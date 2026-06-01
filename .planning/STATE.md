# STATE — ESP32-S3 STEP Acquisition

**Last updated:** 2026-06-01  
**Milestone:** v1.0  
**Current phase:** 1 (Arduino IMU + TCP)

## Project Reference

See: `.planning/PROJECT.md`

**Core value:** Time-aligned IMU streaming at Red Pitaya–equivalent rates.  
**Current focus:** Arduino sketch + guide; camera deferred to v2 Phase 7.

## Session Log

- 2026-06-01: GSD init; ESP-IDF scaffold; camera feasibility doc.
- 2026-06-01: **Reprioritized** — Arduino IDE primary; CAM-* moved to v2; added `arduino/step_node/` and `docs/arduino-ide-guide.md`.

## Blockers

None.

## Next Actions

1. Flash Arduino sketch on lab XIAO ESP32S3; calibrate ICM20948 registers
2. TCP soak test with `host/esp32_tcp_client.py`
3. ESP-NOW two-board sync test
4. Enable SD on Sense hardware
