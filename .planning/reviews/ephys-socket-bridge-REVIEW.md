---
phase: ephys-socket-bridge
reviewed: 2026-06-02T00:00:00Z
depth: deep
files_reviewed: 4
files_reviewed_list:
  - host/serial_tcp_bridge.py
  - host/esp32_tcp_client.py
  - arduino/step_node/step_node.ino
  - firmware/main/open_ephys_stream.c
findings:
  critical: 1
  warning: 2
  info: 1
  total: 4
status: issues_found
---

# Ephys Socket Bridge — Code Review Report

**Reviewed:** 2026-06-02  
**Depth:** deep (cross-module protocol trace)  
**Files Reviewed:** 4  
**Status:** issues_found (1 critical fixed in `host/serial_tcp_bridge.py`)

## Summary

Open Ephys **Ephys Socket** connects as a TCP client and immediately reads a **22-byte binary header** from the server. The USB serial bridge (`host/serial_tcp_bridge.py`) previously blocked in a text **REDPITAYA/START** handshake loop and sent **no binary data** until `START` arrived. Ephys Socket never sends those commands, so the GUI could not read a header within ~500 ms, showed **"Ephys Socket: Could not read stream."**, closed the socket, and the bridge logged connect/disconnect pairs on ephemeral localhost ports.

## Critical Issues

### CR-01: Protocol mismatch — bridge waits for text handshake; Ephys Socket expects immediate binary stream

**File:** `host/serial_tcp_bridge.py:168-183` (before fix)  
**Issue:** `handle_client()` looped on `read_line()` waiting for `REDPITAYA`/`START`. Open Ephys `SocketThread::connectSocket()` (`open-ephys-plugins/ephys-socket` `SocketThread.cpp`) connects and reads `HEADER_SIZE` (22) bytes with no outbound text. Deadlock: GUI reads (gets 0 bytes), bridge reads (waits for `\n`), GUI fails after 5×100 ms retries and disconnects.  
**Fix:** Applied — after a 300 ms optional wait for text commands, default to **Ephys Socket mode** and call `stream_frames()` immediately. Red Pitaya path retained for `esp32_tcp_client.py`.

## Warnings

### WR-01: First-frame latency can still fail connect if USB serial is not streaming

**File:** `host/serial_tcp_bridge.py:206-208`, `arduino/step_node/step_node.ino:48,522-524`  
**Issue:** Ephys Socket allows only ~500 ms to receive the first 22-byte header. Arduino `BOOT_CSV_DELAY_MS` (5000 ms) suppresses serial output after boot; if Open Ephys connects before frames arrive, the same error can occur.  
**Fix:** Applied longer first-frame wait (5 s) and warning log. User must wait for boot delay, enable `USB_OPEN_EPHYS_MODE` (`SERIAL_OUTPUT_BINARY true`), and close Serial Monitor.

### WR-02: Documentation implies Ephys Socket uses REDPITAYA/START on TCP

**File:** `docs/arduino-ide-guide.md:263`, `docs/open-ephys-plugin.md:65`  
**Issue:** Text says handshake is "handled by the bridge" for Ephys Socket, but the built-in plugin does not send those commands — only the bridge→serial forward path and `esp32_tcp_client.py` use them.  
**Fix:** Clarify in docs that Ephys Socket requires **immediate binary** on TCP; handshake is optional for test client / direct ESP32 TCP only.

## Info

### IN-01: Misleading log interpretation (53453/53455 are client ports)

**File:** N/A (operational)  
**Issue:** Ephemeral ports in `('127.0.0.1', 53453)` are the **GUI client's** source ports, not the listen port. Server remains `127.0.0.1:5000`.  
**Fix:** None required — use GUI panel port **5000** as configured.

---

_Reviewed: 2026-06-02_  
_Reviewer: Claude (gsd-code-reviewer)_  
_Depth: deep_
