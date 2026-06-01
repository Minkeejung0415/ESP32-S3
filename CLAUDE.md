# ESP32-S3 STEP — Agent Guide

## Project

ESP-IDF firmware on Seeed XIAO ESP32-S3 for STEP data acquisition (IMU + DIO + camera verify), ESP-NOW sync, SD log, Open Ephys TCP. Extends Red Pitaya workflow — do not break 6-channel STEP clients without `ESP32_NUM_CHANNELS`.

## Layout

- `arduino/step_node/` — **Primary v1** Arduino sketch
- `docs/arduino-ide-guide.md` — flash & TCP setup
- `firmware/` — ESP-IDF (advanced)

## Conventions

- Sample rate: 100 Hz (`OE_STREAM_SAMPLE_HZ`)
- TCP port: 5000
- ICM20948 I2C: SDA=4, SCL=5 (adjust in `icm20948.c` for your board)
- TEVM MIPI is **ESP32-P4**, not S3 — see feasibility doc before promising MIPI on XIAO

## GSD

After changes, update `.planning/STATE.md` and phase status in `ROADMAP.md`.
