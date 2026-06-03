# STATE — ESP32-S3 STEP Acquisition

**Last updated:** 2026-06-03  
**Milestone:** v2.0 — sensor config, quaternion filter, OpenSim  
**Status:** **Complete (code)** — hardware / OpenSim HIL sign-off pending  
**Firmware:** `step_node.ino` v1.4.0

## Project Reference

See: `.planning/PROJECT.md`, `.planning/PLUGIN-INTEGRATION.md`, `.planning/reviews/plugin-esp32-REVIEW.md`

**Core value:** Orientation-aware IMU for Plugin + OpenSim — CFG/FREQ/filter on firmware; quaternions on wire when `FILTER ON`.

## Completed (autonomous run 2026-06-03)

| Phase | Outcome |
|-------|---------|
| 1 | `FREQ:50-200`, `CFG 0 ACC|GYR`, Plugin rate forward, USB bridge command relay |
| 2 | Mahony AHRS; ch3-5/ch7 Q15 quat when filter on |
| 3 | Plugin ESP32 `sendOpenSimQuaternionPacket` when filter + OpenSim enabled |
| 4 | Docs/command matrix; SD/ESP-NOW still deferred |

## Key files

- `arduino/step_node/step_node.ino`
- `host/serial_tcp_bridge.py`, `host/esp32_tcp_client.py`
- `C:\Users\justi\Plugin\acqboard.ccp` (external)
- `docs/open-ephys-plugin.md`, `docs/arduino-ide-guide.md`

## Blockers / user actions

1. **Flash** v1.4.0 and run Wi-Fi or USB `--plugin` HIL for FREQ/CFG/filter.
2. **Plugin:** rebuild with patched `acqboard.ccp`; enable **Filter ON** before OpenSim Live.
3. **OpenSim E2E:** confirm UDP orientation (not identity) during motion.
4. **Plugin repo:** update `ephys_to_opensim_bridge.py` when using host-only OpenSim path (file not in ESP32-S3 repo).

## Preset reminder

| Goal | `USB_OPEN_EPHYS_MODE` | `ENABLE_TCP` |
|------|----------------------|--------------|
| Plugin Wi-Fi | `false` | `true` |
| Plugin USB bridge | `true` | off + `serial_tcp_bridge.py --plugin` |
