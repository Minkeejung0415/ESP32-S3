---
status: human_needed
phase: 1
score: 4/4 must-haves (code)
---

# Phase 1 Verification

## Automated

- [x] Host Python modules compile
- [x] Firmware command handlers present
- [x] Plugin ESP32 FREQ path implemented
- [x] Docs list TCP commands

## Human verification

1. Flash `step_node.ino`, connect Wi-Fi TCP, send `REDPITAYA` → confirm `sample_rate=` matches `FREQ:` setting
2. Send `CFG 0 ACC 2` and verify accel LSB scale changes in Plugin or `esp32_tcp_client.py`
3. Plugin UI sample rate change → confirm `FREQ:` reaches device (Serial log or logic analyzer)
