# Open Ephys integration — ESP32-S3 vs Plugin repo

This document answers whether [Minkeejung0415/Plugin](https://github.com/Minkeejung0415/Plugin) must be edited for the ESP32-S3 STEP node, and how that relates to the built-in **Ephys Socket** plugin.

Firmware reference: `arduino/step_node/step_node.ino` v1.3.0 · Host test: `host/esp32_tcp_client.py`

---

## Short answer

| Open Ephys path | Edit Plugin repo? |
|-----------------|-------------------|
| **Built-in Ephys Socket** (TCP client → node IP:5000) | **No** — use stock Open Ephys GUI; set `ENABLE_TCP true` on ESP32 |
| **Custom Red Pitaya source plugin** (`AcqBoardRedPitaya` in Plugin repo) | **Yes — minimal to moderate changes** (handshake, START, transport, host, scaling) |

For USB-only bench work, neither plugin is required — use Serial Monitor or `host/serial_bench_reader.py`.

---

## What is in Minkeejung0415/Plugin?

Not a standalone “Ephys Socket” plugin. It is a **custom Open Ephys GUI Source plugin** (DeviceThread + acquisition boards) with:

| Component | Role |
|-----------|------|
| `devicethread.cpp` | Detects OpalKelly → ONI → **Red Pitaya** → simulation |
| `acqboard.ccp` + `Acqboardredpitaya.h` | **AcqBoardRedPitaya** — TCP control + **UDP data** from Red Pitaya |
| `RedPitaya_justin.c` | Reference Red Pitaya firmware (multi-sensor, fusion, SD record) |
| `ephys_to_opensim_bridge.py`, `opensim_live_realtime.py` | OpenSim UDP bridge (localhost:5000) |

There is **no** separate Ephys Socket implementation in that repo; streaming logic is Red-Pitaya-specific.

---

## Protocol comparison

| Item | Red Pitaya (`RedPitaya_justin.c` + Plugin) | ESP32-S3 STEP (`step_node.ino`) |
|------|--------------------------------------------|----------------------------------|
| TCP port | **5000** (control only) | **5000** (control **and** sample stream) |
| Data transport | **UDP 55001** (`sendto` per sample) | **Same TCP socket** after `START` |
| `REDPITAYA\n` reply | `OK CHANNELS:<N>\n` | `8 channels; sample_rate=100; node=esp32s3_arduino\n` |
| `START\n` reply | `STARTED BIN:… CSV:…\n` then `SENSORS:0,ICM20948;…\n` | Sets streaming; **no STARTED / SENSORS lines** |
| Sample rate | Configurable (`FREQ:`), default 100 Hz | Fixed **100 Hz** |
| Channels | Dynamic (sensors × raw + quat + analog) | Fixed **8** int16 |
| Packet header | 22-byte LE `iiHiii` | **Same** 22-byte layout |
| Payload | int16 channel-major | int16 channel-major |
| ch0–5 | IMU (scaled in plugin by sensor preset) | ICM20948 ax, ay, az, gx, gy, gz (raw int16) |
| ch6 | Part of sensor layout / DIO varies | DIO packed (level + edge count) |
| ch7 | Reserved / fusion / analog | 0 in v1 |
| Host discovery | Hardcoded `rp-f0f85a.local`, `rp-f0cd35.local` | Wi-Fi IP from Serial Monitor |

**What already matches:** port 5000, `REDPITAYA` / `START` command names, 22-byte Open Ephys header, int16 channel-major samples, 100 Hz target.

**What does not match the custom Plugin:** handshake text, STARTED/SENSORS lines, UDP vs TCP streaming, fixed 8-channel map, ESP32 IP vs `.local` Red Pitaya hosts, Plugin scaling expects Red Pitaya sensor slots + quaternion tail.

---

## Path A — No Plugin repo changes (recommended for first TCP test)

Use the **built-in Open Ephys Ephys Socket** plugin:

1. On ESP32: `#define ENABLE_TCP true`, `#define ENABLE_SERIAL_BENCH false`, set Wi-Fi credentials.
2. Note node IP from Serial Monitor.
3. In Open Ephys GUI: add **Ephys Socket** → TCP client → `<node-ip>:5000`.
4. **Connect only** — built-in Ephys Socket is a TCP client that immediately reads **22-byte binary packet headers**; it does **not** send `REDPITAYA` / `START`. ESP32 firmware TCP mode still expects that text handshake today; for Open Ephys use **USB + `serial_tcp_bridge.py`** (see [arduino-ide-guide.md](arduino-ide-guide.md)) or adapt firmware to stream binary on connect.
5. Expect **8 channels @ 100 Hz**; scale ax–gz on the host (raw int16 ÷ sensitivity — see `host/esp32_tcp_client.py` env `ICM_ACCEL_SCALE` / `ICM_GYRO_SCALE`).

Verify with Python first:

```powershell
set ESP32_NODE_HOST=<node-ip>
python host\esp32_tcp_client.py
```

**Plugin repo:** no edits.

---

## Path B — Custom Red Pitaya plugin (Plugin repo changes required)

To use **AcqBoardRedPitaya** from [Minkeejung0415/Plugin](https://github.com/Minkeejung0415/Plugin) with ESP32-S3 **as-is**, the plugin will fail at detection and/or streaming. Required changes (firmware parity is an alternative — not covered here):

### 1. `acqboard.ccp` — `performDetectionHandshake()`

**Today:** Requires response containing `"OK"` and parses `CHANNELS:N`.

**Change:** Also accept ESP32 reply, e.g. parse `8 channels` or `sample_rate=100`, set `numAdcChannels = 8`, `deviceFound = true` without requiring `"OK"`.

### 2. `acqboard.ccp` — `kRedPitayaHosts[]` / connect path

**Today:** Only `rp-f0f85a.local`, `rp-f0cd35.local`.

**Change:** Add configurable host (editor text field or env) for ESP32 IP, e.g. `192.168.x.x`, or try mDNS `esp32s3.local` if you add it on the board.

### 3. `acqboard.ccp` — `startAcquisition()`

**Today:** Blocks until `STARTED` / `STARTED BIN:…` and reads `SENSORS:…` line for channel layout and OpenSim mapping.

**Change:** For ESP32 mode: after `START\n`, if no `STARTED` within timeout, assume **fixed 8-channel ESP32 layout**; set `streamSensorNames = { "ICM20948" }` or synthetic single-sensor map; skip SENSORS parse.

### 4. `acqboard.ccp` — `run()`

**Today:** Binds **UDP 55001**, reads `headerSize + numAdcChannels*2` per datagram.

**Change (pick one):**

- **Option 4a (plugin-side):** ESP32 mode reads binary packets from **TCP `commandSocket`** after START (mirror `host/esp32_tcp_client.py` loop).
- **Option 4b (firmware-side):** Add UDP 55001 streaming to ESP32 to match Red Pitaya — then plugin `run()` stays UDP-only.

### 5. `acqboard.ccp` — scaling / channel names

**Today:** Per-sensor ACC/GYR presets from Red Pitaya `CFG` commands; quaternion slots after raw IMU.

**Change:** For 8-ch ESP32 map, fixed scale factors for ch0–5 (ICM20948 ±2g / ±250°/s defaults or match sketch), ch6 = DIO (bit0 level), ch7 = 0; disable quaternion OpenSim path unless fusion added on ESP32.

### 6. `devices/redpitaya/AcqBoardRedPitaya.h` + `devicethread.cpp`

Optional: rename or add `AcqBoardEsp32S3` subclass; register in `detectBoard()` after Red Pitaya probe fails but TCP handshake to user IP succeeds.

### 7. `device editor.cpp`

Optional: IP/host field and “ESP32 fixed 8 ch” toggle in UI.

**Do not need to change:** `RedPitaya_justin.c` (Red Pitaya only), OpenSim bridge scripts (unless you want ESP32 quaternion layout).

---

## Alternative: align ESP32 firmware to Red Pitaya plugin

Instead of editing the Plugin, you could extend `step_node.ino` to:

- Reply `OK CHANNELS:8\n` to `REDPITAYA`
- Reply `STARTED\n` + `SENSORS:0,ICM20948\n` to `START`
- Stream samples on **UDP 55001** (keep TCP for commands)

That would be firmware work in **ESP32-S3**, not Plugin repo. The custom plugin would then need only **host IP** configuration and **8-channel scaling** tweaks.

---

## See also

- [arduino-ide-guide.md](arduino-ide-guide.md) — TCP / serial bench setup
- [wiring-diagram.md](wiring-diagram.md) — ICM dual-silk wiring
- Plugin change log: `docs/2026-04-27-change-documentation.md` in Plugin repo (UDP 55001, SENSORS line)
