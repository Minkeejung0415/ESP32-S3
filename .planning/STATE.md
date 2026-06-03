# STATE — ESP32-S3 STEP Acquisition

**Last updated:** 2026-06-02  
**Milestone:** v1.0  
**Current phase:** Host/OpenSim parity (fusion on-device); Phase 3 ESP-NOW next

## Project Reference

See: `.planning/PROJECT.md`

**Core value:** Time-aligned IMU streaming at Red Pitaya–equivalent rates with lab features (filter, quaternions) without full RP backend parity.  
**Current focus:** Flash v1.4.0; USB bridge + Acquisition Board; OpenSim via `host/esp32_to_opensim_bridge.py`.

## Session Log

- 2026-06-01: GSD init; ESP-IDF scaffold; camera feasibility doc.
- 2026-06-01: Arduino-first; CAM-* deferred v2.
- 2026-06-01: Phase 1–2 complete (ICM, DIO, v1.3.0).
- 2026-06-02: **v1.4.0** — Madgwick fusion on ESP32, 11 ch (quat ch7–10), `FILTER` command, `OK CHANNELS:11` / `STARTED`, OpenSim bridge, serial TCP bridge updated.

## Blockers

None.

## Next Actions

1. Flash v1.4.0 and verify quat ch7–10 in `host/esp32_tcp_client.py`
2. Plugin: map filter button → `FILTER 1`/`0` on TCP (minimal `acqboard.ccp` edit)
3. Phase 3: second board for ESP-NOW when available
