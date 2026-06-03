#!/usr/bin/env python3
"""
Apply the 3 minimal Open Ephys Plugin patches for ESP32-S3 (local Plugin clone).

  1. Prepend 127.0.0.1 to kRedPitayaHosts
  2. Read samples from TCP when numAdcChannels == 11 (ESP32 mode)
  3. Skip blocking SENSORS parse after STARTED when ESP32 mode

Usage:
  python scripts/patch_plugin_esp32.py --plugin-dir C:\\path\\to\\Plugin
  python scripts/patch_plugin_esp32.py --plugin-dir ~/Plugin --dry-run

Searches acqboard.ccp (and .cpp). Creates .bak backups.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ESP32_FLAG = "bool esp32TcpStream = false;"
ESP32_FLAG_DETECT = (
    "if (response.contains(\"CHANNELS:\"))\n"
    "    {\n"
    "        int n = parseChannelsFromResponse(response);\n"
    "        if (n == 11)\n"
    "            esp32TcpStream = true;\n"
    "    }"
)

TCP_RUN_BLOCK = r'''
    if (esp32TcpStream && commandSocket != nullptr)
    {
        const int frameBytes = headerSize + numAdcChannels * 2;
        Array<uint8> buffer;
        buffer.resize(frameBytes);

        while (!threadShouldExit())
        {
            if (!readTcpFrame(commandSocket, buffer.getData(), frameBytes))
                continue;

            memcpy(thisSample, buffer.getData() + headerSize, numAdcChannels * 2);
            incrementSampleCounter();
            broadcastSample();
        }
        return;
    }
'''

SENSORS_SKIP = r'''
    if (esp32TcpStream)
    {
        // ESP32: STARTED only; fixed 11-channel layout (no SENSORS line).
        numAdcChannels = 11;
        return;
    }
'''


def find_acqboard(root: Path) -> Path | None:
    for name in ("acqboard.ccp", "acqboard.cpp", "AcqBoardRedPitaya.cpp"):
        hits = list(root.rglob(name))
        if hits:
            return hits[0]
    return None


def patch_hosts(text: str) -> tuple[str, bool]:
    if '"127.0.0.1"' in text and "kRedPitayaHosts" in text:
        return text, False
    pat = re.compile(
        r"(static\s+const\s+char\*\s+kRedPitayaHosts\s*\[\]\s*=\s*\{)",
        re.MULTILINE,
    )
    if not pat.search(text):
        pat = re.compile(r"(kRedPitayaHosts\s*\[\]\s*=\s*\{)")
    m = pat.search(text)
    if not m:
        return text, False
    return pat.sub(r'\1\n        "127.0.0.1",', text, count=1), True


def patch_esp32_flag_member(text: str) -> tuple[str, bool]:
    if "esp32TcpStream" in text:
        return text, False
    m = re.search(r"class\s+AcqBoardRedPitaya[^{]*\{", text)
    if not m:
        return text, False
    insert_at = m.end()
    return text[:insert_at] + "\n    " + ESP32_FLAG + text[insert_at:], True


def patch_detect_11ch(text: str) -> tuple[str, bool]:
    if "esp32TcpStream = true" in text:
        return text, False
    for needle in (
        "deviceFound = true",
        "return true;",
        "numAdcChannels =",
    ):
        idx = text.find("performDetectionHandshake")
        if idx < 0:
            continue
        chunk = text[idx : idx + 8000]
        if "CHANNELS:" in chunk and "OK" in chunk:
            m = re.search(r"(numAdcChannels\s*=\s*\d+;)", chunk)
            if m:
                pos = idx + chunk.find(m.group(0)) + len(m.group(0))
                return text[:pos] + "\n    if (response.contains(\"CHANNELS:11\"))\n        esp32TcpStream = true;\n" + text[pos:], True
    return text, False


def patch_sensors_skip(text: str) -> tuple[str, bool]:
    if "ESP32: STARTED only" in text:
        return text, False
    m = re.search(r"void\s+AcqBoardRedPitaya::startAcquisition\s*\([^)]*\)\s*\{", text)
    if not m:
        return text, False
    return text[: m.end()] + SENSORS_SKIP + text[m.end() :], True


def patch_run_tcp(text: str) -> tuple[str, bool]:
    if "esp32TcpStream && commandSocket" in text:
        return text, False
    m = re.search(r"void\s+AcqBoardRedPitaya::run\s*\([^)]*\)\s*\{", text)
    if not m:
        return text, False
    return text[: m.end()] + TCP_RUN_BLOCK + text[m.end() :], True


def patch_file(path: Path, dry_run: bool) -> list[str]:
    text = path.read_text(encoding="utf-8", errors="replace")
    original = text
    applied: list[str] = []

    for name, fn in (
        ("host 127.0.0.1", patch_hosts),
        ("esp32TcpStream member", patch_esp32_flag_member),
        ("detect 11 channels", patch_detect_11ch),
        ("skip SENSORS wait", patch_sensors_skip),
        ("TCP run() path", patch_run_tcp),
    ):
        text, ok = fn(text)
        if ok:
            applied.append(name)

    if text == original:
        return applied

    if dry_run:
        print(f"[dry-run] would patch {path}: {applied}")
        return applied

    backup = path.with_suffix(path.suffix + ".bak")
    if not backup.exists():
        backup.write_text(original, encoding="utf-8")
    path.write_text(text, encoding="utf-8")
    print(f"Patched {path}: {applied}")
    return applied


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--plugin-dir", type=Path, required=True, help="Root of Minkeejung0415/Plugin clone")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    root = args.plugin_dir.resolve()
    if not root.is_dir():
        print(f"Not a directory: {root}", file=sys.stderr)
        sys.exit(1)

    acq = find_acqboard(root)
    if not acq:
        print("Could not find acqboard.ccp under", root, file=sys.stderr)
        print("Copy plugin-patches/MANUAL.md edits by hand.", file=sys.stderr)
        sys.exit(1)

    applied = patch_file(acq, args.dry_run)
    if not applied:
        print("No automatic patches applied — see plugin-patches/MANUAL.md")
        sys.exit(2)

    print("Done. Rebuild the Open Ephys Plugin (Visual Studio / CMake).")
    print("Or use host/rp_compat_gateway.py with plugin-patches/hosts.txt (no rebuild).")


if __name__ == "__main__":
    main()
