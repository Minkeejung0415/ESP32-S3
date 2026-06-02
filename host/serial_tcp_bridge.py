#!/usr/bin/env python3
"""
USB Serial → TCP bridge for Open Ephys (Ephys Socket compatible).

Use when the PC cannot join the ESP32 Wi-Fi (STEP_ESP32 Soft AP or hotspot).
Board streams Open Ephys binary on USB; this script listens on localhost:5000.

Sketch preset (USB_OPEN_EPHYS_MODE in step_node.ino):
  ENABLE_TCP false
  ENABLE_SERIAL_BENCH true
  SERIAL_OUTPUT_BINARY true

Windows example:
  pip install pyserial
  python host/serial_tcp_bridge.py COM5
  set SERIAL_PORT=COM5 && python host/serial_tcp_bridge.py

Open Ephys Ephys Socket: TCP client → 127.0.0.1:5000
"""
from __future__ import annotations

import argparse
import asyncio
import logging
import os
import struct
import sys
import threading
import time
from collections import deque

logger = logging.getLogger(__name__)

HEADER = struct.Struct("<iiHiii")
HEADER_SIZE = HEADER.size
FRAME_PAYLOAD = 8 * 2  # 8 x int16
FRAME_SIZE = HEADER_SIZE + FRAME_PAYLOAD
HANDSHAKE_REPLY = b"8 channels; sample_rate=100; node=esp32s3_arduino\n"
# Open Ephys Ephys Socket: OpenCV Mat depth enum (S16), not literal 16 bits.
OE_BIT_DEPTH_S16 = 3


def open_serial(port: str, baud: int):
    try:
        import serial
    except ImportError:
        print("Install pyserial: pip install pyserial", file=sys.stderr)
        sys.exit(1)
    ser = serial.Serial(port, baud, timeout=0.1)
    ser.reset_input_buffer()
    return ser


def pack_csv_row(fields: list[str]) -> bytes | None:
    """Convert CSV seq,ax,...,cam (9 fields) to one Open Ephys frame."""
    if len(fields) < 9:
        return None
    try:
        _seq = int(fields[0])
        ch = [int(fields[i]) for i in range(1, 9)]
    except ValueError:
        return None
    hdr = HEADER.pack(0, FRAME_PAYLOAD, 16, 2, 8, 1)
    return hdr + struct.pack("<8h", *ch)


def is_valid_header(hdr: tuple) -> bool:
    _off, num_bytes, bit_depth, elem, n_ch, n_per = hdr
    return (
        elem == 2
        and bit_depth == 16
        and n_ch == 8
        and n_per == 1
        and num_bytes == FRAME_PAYLOAD
    )


class SerialFrameSource:
    """Background thread: parse binary (or CSV) frames from USB serial."""

    def __init__(self, port: str, baud: int, csv_mode: bool) -> None:
        self._ser = open_serial(port, baud)
        self._csv_mode = csv_mode
        self._buf = bytearray()
        self._queue: deque[bytes] = deque(maxlen=256)
        self._lock = threading.Lock()
        self._stop = threading.Event()
        self._thread = threading.Thread(target=self._run, daemon=True)

    def start(self) -> None:
        self._thread.start()

    def stop(self) -> None:
        self._stop.set()
        self._thread.join(timeout=2)
        self._ser.close()

    def get_frame(self, timeout: float = 0.5) -> bytes | None:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            with self._lock:
                if self._queue:
                    return self._queue.popleft()
            time.sleep(0.01)
        return None

    def drain(self) -> None:
        with self._lock:
            self._queue.clear()

    def write_line(self, line: str) -> None:
        self._ser.write(line.encode("ascii"))

    def _push_frame(self, frame: bytes) -> None:
        with self._lock:
            self._queue.append(frame)

    def _run(self) -> None:
        while not self._stop.is_set():
            chunk = self._ser.read(4096)
            if chunk:
                self._buf.extend(chunk)
            if self._csv_mode:
                self._parse_csv_lines()
            else:
                self._parse_binary()

    def _parse_csv_lines(self) -> None:
        while b"\n" in self._buf:
            line, _, rest = self._buf.partition(b"\n")
            self._buf = bytearray(rest)
            text = line.decode("utf-8", errors="replace").strip()
            if not text or text.startswith("#") or text.startswith("STEP"):
                continue
            if text.startswith("Format:") or "Wi-Fi" in text or text.startswith("ICM"):
                continue
            if text.startswith("seq,"):
                continue
            parts = text.split(",")
            frame = pack_csv_row(parts)
            if frame:
                self._push_frame(frame)

    def _parse_binary(self) -> None:
        while len(self._buf) >= HEADER_SIZE:
            hdr = HEADER.unpack_from(self._buf, 0)
            if not is_valid_header(hdr):
                del self._buf[0]
                continue
            if len(self._buf) < FRAME_SIZE:
                break
            frame = bytes(self._buf[:FRAME_SIZE])
            del self._buf[:FRAME_SIZE]
            self._push_frame(frame)


async def read_line(reader: asyncio.StreamReader) -> str:
    line = await reader.readline()
    return line.decode("utf-8", errors="replace").strip()


async def handle_client(
    reader: asyncio.StreamReader,
    writer: asyncio.StreamWriter,
    source: SerialFrameSource,
) -> None:
    peer = writer.get_extra_info("peername")
    logger.info("TCP client connected: %s", peer)
    try:
        while True:
            line = await asyncio.wait_for(read_line(reader), timeout=120.0)
            if not line:
                break
            upper = line.upper()
            if upper.startswith("REDPITAYA"):
                logger.info("handshake REDPITAYA")
                source.write_line("REDPITAYA\n")
                writer.write(HANDSHAKE_REPLY)
                await writer.drain()
            elif upper.startswith("START"):
                logger.info("START — forwarding serial stream")
                source.write_line("START\n")
                source.drain()
                await stream_frames(reader, writer, source)
                break
            else:
                logger.debug("ignore command: %s", line)
    except asyncio.TimeoutError:
        logger.warning("client idle timeout")
    except ConnectionResetError:
        pass
    finally:
        writer.close()
        try:
            await writer.wait_closed()
        except Exception:
            pass
        logger.info("TCP client disconnected")


async def stream_frames(
    reader: asyncio.StreamReader,
    writer: asyncio.StreamWriter,
    source: SerialFrameSource,
) -> None:
    loop = asyncio.get_event_loop()
    while not reader.at_eof():
        frame = await loop.run_in_executor(None, source.get_frame, 1.0)
        if frame is None:
            continue
        writer.write(frame)
        await writer.drain()


async def run_server(
    bind: str,
    port: int,
    source: SerialFrameSource,
) -> None:
    server = await asyncio.start_server(
        lambda r, w: handle_client(r, w, source),
        bind,
        port,
    )
    addrs = ", ".join(str(s.getsockname()) for s in server.sockets or [])
    logger.info("Serial→TCP bridge listening on %s (Open Ephys → %s:%d)", addrs, bind, port)
    async with server:
        await server.serve_forever()


def main() -> None:
    p = argparse.ArgumentParser(
        description="Bridge STEP USB serial (Open Ephys binary) to TCP localhost:5000"
    )
    p.add_argument(
        "port",
        nargs="?",
        default=os.environ.get("SERIAL_PORT"),
        help="Serial port (e.g. COM5 on Windows, /dev/ttyACM0 on Linux)",
    )
    p.add_argument("--baud", type=int, default=int(os.environ.get("SERIAL_BAUD", "115200")))
    p.add_argument("--bind", default=os.environ.get("BRIDGE_BIND", "127.0.0.1"))
    p.add_argument("--tcp-port", type=int, default=int(os.environ.get("BRIDGE_PORT", "5000")))
    p.add_argument(
        "--csv",
        action="store_true",
        help="Parse CSV serial (SERIAL_OUTPUT_BINARY false) instead of binary",
    )
    args = p.parse_args()

    if not args.port:
        print("Usage: python host/serial_tcp_bridge.py COM5", file=sys.stderr)
        sys.exit(1)

    logging.basicConfig(level=logging.INFO, format="%(levelname)s: %(message)s")
    print(f"Serial {args.port} @ {args.baud} → TCP {args.bind}:{args.tcp_port}")
    print("Close Serial Monitor before starting. Open Ephys: Ephys Socket → 127.0.0.1:5000")

    source = SerialFrameSource(args.port, args.baud, csv_mode=args.csv)
    source.start()
    try:
        asyncio.run(run_server(args.bind, args.tcp_port, source))
    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        source.stop()


if __name__ == "__main__":
    main()
