# Phase 1 Plan

**Goal:** CFG/FREQ change ESP32 behavior and Plugin forwards them.

## Tasks

1. Firmware: runtime `g_sample_hz`, `handleLine` for `FREQ:`, `CFG`, dynamic REDPITAYA line
2. Firmware: `icmApplyRangePresets()` for ICM20948 bank-2 FS bits
3. Plugin: handshake parse `sample_rate=`; ESP32 `startAcquisition` sends `FREQ:`; `updateSampleFrequency` for ESP32
4. Host: `serial_tcp_bridge.py` relay Plugin commands during stream
5. Docs: `docs/open-ephys-plugin.md`, `docs/arduino-ide-guide.md` command matrix

## Verification

- `python -m py_compile host/serial_tcp_bridge.py host/esp32_tcp_client.py`
- Manual: `FREQ:50` / `CFG 0 ACC 1` via TCP or bridge (hardware)
