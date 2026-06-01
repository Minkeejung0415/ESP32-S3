# ESP32-S3 STEP Acquisition Nodes

Wireless **Seeed XIAO ESP32-S3** node for the STEP gait stack: **ICM-20948** IMU, **DIO**, **Open Ephys TCP** — one board is enough for v1 bench.

## Single board quick test (start here)

1. Open **`arduino/step_node/step_node.ino`** — leave **`ENABLE_ESPNOW false`** (default).
2. Set **`WIFI_SSID`** / **`WIFI_PASS`**, flash **XIAO_ESP32S3** ([full guide](docs/arduino-ide-guide.md)).
3. Serial Monitor → **`WiFi OK IP=...`**.
4. **TCP:** `set ESP32_NODE_HOST=<IP>` → `python host/esp32_tcp_client.py`
5. **Or no Wi-Fi:** `ENABLE_SERIAL_BENCH true`, `ENABLE_TCP false` → CSV on Serial @ 115200.

Multi-node **ESP-NOW** is optional later (`ENABLE_ESPNOW true`). Camera is **v2**.

## Hardware

| Component | v1 target | Notes |
|-----------|-----------|-------|
| MCU | Seeed XIAO ESP32-S3 | One board for bench |
| IMU | ICM-20948 (I2C) | 100 Hz, ch0–5 |
| DIO | GPIO input | ch6 |
| Sync | ESP-NOW (optional) | Off by default |
| SD | Sense expansion (optional) | `ENABLE_SD` |
| Camera | **Deferred v2** | [camera-feasibility.md](docs/camera-feasibility.md) |

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

Same modules in **`firmware/`** — menuconfig **STEP_ENABLE_ESPNOW** defaults off for single-node.

```bash
cd firmware
idf.py set-target esp32s3
idf.py build flash monitor
```

## Host / Open Ephys / OpenSim

1. Set `ESP32_NODE_HOST` and `ESP32_NODE_PORT=5000`
2. **`host/esp32_tcp_client.py`**
3. **Open Ephys:** Ephys Socket plugin, TCP client mode

## Planning (GSD)

- `.planning/PROJECT.md` — goals
- `.planning/REQUIREMENTS.md` — v1 vs v2
- `.planning/ROADMAP.md` — phases

## Repository

https://github.com/Minkeejung0415/ESP32-S3.git
