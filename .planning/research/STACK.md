# Stack Research — ESP32-S3 STEP Acquisition

**Confidence:** High for IMU/TCP/ESP-NOW; Medium for TEVM-AR0234 on ESP32-S3.

## Recommended Stack

| Layer | Choice | Version / Notes |
|-------|--------|-----------------|
| Framework | ESP-IDF | ≥5.2 (5.3+ for esp32-camera improvements) |
| Board | Seeed XIAO ESP32-S3 / Sense | `seeed_xiao_esp32s3` or custom board JSON |
| IMU | ICM-20948 | I2C @ 400 kHz; `esp-idf-lib` or custom driver; 100–200 Hz |
| Camera (bring-up) | OV3660/OV5640 DVP | `esp32-camera` on Sense expansion |
| Camera (target) | TEVM-AR0234 MIPI | **ESP32-P4** + `esp-video-components` / VizionSDK; or NXP/i.MX gateway → ESP32 |
| Multi-node sync | ESP-NOW | Master broadcasts `sync_frame_t` (seq, timestamp_us) |
| Streaming | Wi-Fi TCP | Open Ephys 22-byte header + int16 channel-major |
| Local log | SDMMC / SPI SD | FATFS, ring buffer writer task |
| Host bridge | Python 3.10+ | Extend STEP `tcp_client.py` for extra channels |
| Build | `idf.py` | `firmware/` project root |

## What NOT to Use (on ESP32-S3)

- **Direct MIPI TEVM on XIAO ESP32-S3** — SoC has LCD_CAM DVP only; MIPI needs ESP32-P4 or external bridge.
- **Arduino-only stack for production** — OK for spikes; ESP-IDF required for SD + Wi-Fi + camera concurrent tasks.
- **BLE for sync** — Higher latency than ESP-NOW for sub-ms alignment.

## Red Pitaya Parity Constants

- Sample rate: 100 Hz default (`RED_PITAYA_SAMPLE_HZ`)
- TCP port: 5000
- Handshake: `REDPITAYA\n` → `START\n`
- Header: `<iiHiii` (22 bytes LE)
- Channels: int16, channel-major (C order)

## Extended Channel Map (v1 proposal)

| Ch | Content |
|----|---------|
| 0–5 | ICM20948 ax, ay, az, gx, gy, gz (scaled int16) |
| 6 | DIO level or edge counter low word |
| 7 | Camera motion score / verification flag |
| 8–9 | ESP-NOW master timestamp fragments (optional) |
