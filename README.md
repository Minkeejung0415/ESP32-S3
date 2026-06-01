# ESP32-S3 STEP Acquisition Nodes

Wireless **Seeed XIAO ESP32-S3** nodes for the STEP gait monitoring stack: **ICM-20948** IMU, **DIO**, **ESP-NOW** sync, **SD card** logging, and **Open Ephys–compatible** TCP streaming aligned with the existing **Red Pitaya** path.

## Quick start (Arduino IDE — recommended for v1)

1. Open **`arduino/step_node/step_node.ino`**
2. Follow **[docs/arduino-ide-guide.md](docs/arduino-ide-guide.md)** — board package, Wi-Fi, flash, TCP test
3. Run **`python host/esp32_tcp_client.py`** against the node IP (port 5000)

Camera and IMU-vs-camera verification are **v2**; v1 focuses on IMU + TCP + sync + SD.

## Hardware

| Component | v1 target | Notes |
|-----------|-----------|-------|
| MCU | Seeed XIAO ESP32-S3 | Arduino sketch in `arduino/` |
| IMU | ICM-20948 (I2C) | 100 Hz, ch0–5 |
| DIO | GPIO input | ch6 |
| Sync | ESP-NOW | Master/slave in sketch |
| SD | Sense expansion (optional) | `ENABLE_SD` |
| Camera | **Deferred v2** | Reference: [docs/camera-feasibility.md](docs/camera-feasibility.md) |

## Red Pitaya parity

| Parameter | Value |
|-----------|-------|
| TCP port | 5000 |
| Handshake | `REDPITAYA` → `START` |
| Packet | 22-byte LE header + int16 channel-major |
| Rate | 100 Hz |
| Channels 0–5 | ax, ay, az, gx, gy, gz |
| Channel 6 | DIO |
| Channel 7 | Reserved (0 in v1) |

## Advanced: ESP-IDF

Same modules in **`firmware/`** for `idf.py` builds (FreeRTOS, optional camera stubs). Use after Arduino bring-up is stable.

```bash
cd firmware
idf.py set-target esp32s3
idf.py build flash monitor
```

## Host / Open Ephys / OpenSim

1. Set `ESP32_NODE_HOST` and `ESP32_NODE_PORT=5000`
2. **`host/esp32_tcp_client.py`** — 8-channel parser (STEP-compatible)
3. **Open Ephys:** Ephys Socket plugin, TCP client mode
4. **OpenSim:** host bridge from TCP or SD export (v2)

## Planning (GSD)

- `.planning/PROJECT.md` — goals
- `.planning/REQUIREMENTS.md` — v1 vs v2
- `.planning/ROADMAP.md` — phases (camera = Phase 7 / v2)

## Repository

https://github.com/Minkeejung0415/ESP32-S3.git
