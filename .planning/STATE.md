# STATE — ESP32-S3 STEP Acquisition

**Last updated:** 2026-06-03  
**Milestone:** v1.0 → v1.4 integration  
**Current phase:** Host + Plugin + OpenSim integration (firmware v1.4 re-merge pending on `main`)

## Project Reference

See: `.planning/PROJECT.md`, **docs/integration-checklist.md**

**Core value:** Time-aligned IMU @ 100 Hz with on-device fusion (filter + quaternions) for STEP + Plugin + OpenSim.  
**Current focus:** Complete checklist in `docs/integration-checklist.md`; merge PR #3 bridge fix; re-flash v1.4 firmware.

## Session Log

- 2026-06-01: GSD init; ESP-IDF scaffold; Phase 1–2 (ICM, DIO).
- 2026-06-02–03: v1.4 fusion branch (11 ch, Madgwick, gateway, plugin patches); partial merge to `main`.
- 2026-06-03: **USB bridge fix** — `bit_depth=16` parsing (PR #3).
- 2026-06-03: **Integration checklist** — full pending work documented (`docs/integration-checklist.md`).

## Blockers

- `main` firmware still v1.3.0 (8 ch, no fusion) — lab must flash v1.4 or wait for re-merge.
- Plugin repo changes are local to each PC (patch or gateway path).

## Next Actions

1. Merge PR #3 (`serial_tcp_bridge.py` bit_depth fix).
2. Flash v1.4 (`step_node.ino` + `imu_fusion.h`, `USB_OPEN_EPHYS_MODE true`).
3. Follow **Phase A–C** in `docs/integration-checklist.md`.
4. Plugin: gateway + `hosts.txt` **or** `patch_plugin_esp32.py`.
5. OpenSim: `esp32_to_opensim_bridge.py` + rotation calibration.

## Preset reminder

| Goal | Firmware | Host | Open Ephys |
|------|----------|------|------------|
| Plugin USB | `USB_OPEN_EPHYS_MODE true` | `serial_tcp_bridge.py COMx --plugin` | `127.0.0.1:5000` |
| Plugin USB (no C++ patch) | same | `rp_compat_gateway.py COMx` + hosts | `rp-f0f85a.local` |
| Ephys Socket Wi-Fi | `ENABLE_TCP true` | none | board IP:5000 |
