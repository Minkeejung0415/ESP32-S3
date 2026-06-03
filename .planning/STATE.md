# STATE — ESP32-S3 STEP Acquisition

**Last updated:** 2026-06-03  
**Milestone:** v1.0  
**Current phase:** Plugin AcqBoard + ESP32 v1.4 fusion on main

## Project Reference

See: `.planning/PROJECT.md`, `.planning/PLUGIN-INTEGRATION.md`

**Core value:** Time-aligned IMU @ 100 Hz for STEP + Plugin acquisition board; on-device fusion (filter + quaternions).  
**Current focus:** USB `serial_tcp_bridge.py --plugin` or `rp_compat_gateway.py`; flash v1.4.1.

## Session Log

- 2026-06-01: GSD init; ESP-IDF scaffold; camera feasibility doc.
- 2026-06-01: Arduino-first; Phase 1–2 (ICM, DIO).
- 2026-06-02: Plugin ESP32 path in local clone; USB bridge `--plugin` mode.
- 2026-06-02–03: **v1.4.x** — Madgwick fusion, 11 ch, `FILTER_PERMANENT`, `rp_compat_gateway.py`, merged to `main`.

## Blockers

- Lab HIL on Plugin + ESP32 still required.
- ESP-NOW / camera deferred.

## Next Actions

1. Flash v1.4.1 with `USB_OPEN_EPHYS_MODE` or Wi-Fi + Plugin IP.
2. `host\run_usb_plugin_bridge.ps1 COMx` or `host\rp_compat_gateway.py COMx`.
3. OpenSim: `host\esp32_to_opensim_bridge.py` (quat ch7–10).

## Preset reminder

| Goal | `USB_OPEN_EPHYS_MODE` | `ENABLE_TCP` | Open Ephys |
|------|----------------------|--------------|------------|
| Plugin (primary) | `false` | `true` | Custom source → node IP:5000 |
| Plugin USB | `true` | `false` | Acq Board → 127.0.0.1 via bridge |
| Ephys Socket | `true` | `false` | Ephys Socket → bridge (no `--plugin`) |
