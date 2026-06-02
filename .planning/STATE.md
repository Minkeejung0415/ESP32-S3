# STATE — ESP32-S3 STEP Acquisition

**Last updated:** 2026-06-02  
**Milestone:** v1.0  
**Current phase:** 1 — **Plugin AcqBoard validation** (sensors + Ephys Socket USB alternate done)  
**Next after Phase 1:** Phase 2 Wi-Fi TCP + same Plugin board

## Project Reference

See: `.planning/PROJECT.md`, `.planning/PLUGIN-INTEGRATION.md`

**Core value:** Time-aligned IMU @ 100 Hz for STEP + **Plugin acquisition board** (primary).  
**Current focus:** Single ESP32 — prove **REDPITAYA/START** path with Plugin repo; keep USB **Ephys Socket + bridge** as lab alternate. **ESP-NOW and camera not in flight.**

## Session Log

- 2026-06-01: GSD init; ESP-IDF scaffold; camera feasibility doc.
- 2026-06-01: Arduino-first; CAM deferred.
- 2026-06-01: Phase 1 sensors — ICM, DIO, USB binary bench.
- 2026-06-02: Brownfield refresh.
- 2026-06-02: **Plugin ESP32 path** — patched `C:\Users\justi\Plugin` (commit `e298679`): dual Red Pitaya / ESP32-S3 in `AcqBoardRedPitaya`; review `.planning/reviews/plugin-esp32-REVIEW.md`.

## Blockers

- ~~**Plugin AcqBoard:** C++ patches in [Minkeejung0415/Plugin](https://github.com/Minkeejung0415/Plugin)~~ → **Implemented** in local clone `C:\Users\justi\Plugin` @ `e298679` (2026-06-02). Lab HIL still required on RP + ESP32.
- **ESP-NOW:** second board + deferred until streaming sign-off.
- **Camera:** explicitly out of scope for current milestone.

## Next Actions (operator)

1. **Flash for Plugin Wi-Fi test:** `USB_OPEN_EPHYS_MODE false`, `ENABLE_TCP true`, Wi-Fi credentials → note IP from Serial Monitor.
2. **Verify handshake:** `set ESP32_NODE_HOST=<ip>` then `python host\esp32_tcp_client.py`.
3. **Plugin repo:** build Open Ephys with Plugin sources from `C:\Users\justi\Plugin` (commit `e298679`); set `ESP32_NODE_HOST` or editor **Node IP**; review `.planning/reviews/plugin-esp32-REVIEW.md`.
4. **Alternate (no Plugin build):** `USB_OPEN_EPHYS_MODE true` + `python host\serial_tcp_bridge.py COMx` → Ephys Socket @ 127.0.0.1:5000.
5. When Plugin acquires: **`/gsd:plan-phase 2`** Wi-Fi hardening; then Phase 3 OpenSim scripts in Plugin repo.

## Preset reminder

| Goal | `USB_OPEN_EPHYS_MODE` | `ENABLE_TCP` | Open Ephys |
|------|----------------------|--------------|------------|
| **Plugin (primary)** | `false` | `true` | Custom source → node IP:5000 |
| **Ephys Socket (alternate)** | `true` | off (USB binary) | Ephys Socket → 127.0.0.1:5000 via bridge |
| 4-wire CSV bench | Manual lines 14–20 | — | No Open Ephys |

**Gap:** Plugin may require `STARTED`/`SENSORS` and/or UDP 55001; ESP32/firmware/bridge differ — resolve in Plugin (preferred) per doc.
