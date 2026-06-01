# STATE — ESP32-S3 STEP Acquisition

**Last updated:** 2026-06-01  
**Milestone:** v1.0  
**Current phase:** 1 (Arduino IMU + TCP)

## Project Reference

See: `.planning/PROJECT.md`

**Core value:** Time-aligned IMU streaming at Red Pitaya–equivalent rates.  
**Current focus:** Single-board Arduino bench (`ENABLE_ESPNOW false` default); camera v2.

## Session Log

- 2026-06-01: GSD init; ESP-IDF scaffold; camera feasibility doc.
- 2026-06-01: Arduino-first; CAM-* deferred v2.
- 2026-06-01: **Single-node default** — ESP-NOW gated off; one board sufficient for v1 test.

## Blockers

None.

## Next Actions

1. Flash one XIAO ESP32S3; TCP or serial bench per guide
2. Calibrate ICM20948 burst read in sketch
3. ESP-NOW two-board test only when second board is ready
