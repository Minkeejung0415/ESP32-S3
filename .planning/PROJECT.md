# ESP32-S3 Multi-Node Acquisition (STEP Extension)

## What This Is

Firmware and host tooling to add **Seeed Studio XIAO ESP32-S3** nodes to the existing **STEP** gait / rehabilitation pipeline. Each node acquires **ICM-20948 IMU** and **DIO**, synchronizes with **ESP-NOW**, logs to **SD**, and streams **Open Ephys–compatible** TCP packets—matching the **Red Pitaya** path (`tcp_client.py`, 100 Hz, 22-byte header + int16 channels).

**v1 bring-up uses Arduino IDE** (`arduino/step_node/`). **Camera / action verification is v2**; TEVM-AR0234 feasibility is documented only.

## Core Value

**Time-aligned, trustworthy IMU motion data** at Red Pitaya–equivalent sample rates so STEP analytics (ZUPT, symmetry, alerts) run unchanged or with minimal host changes.

## Requirements

### Validated

- ✓ **CAM-03 reference** — `docs/camera-feasibility.md` (TEVM / S32 vs ESP32-S3)

### Active

- [ ] Arduino sketch on Seeed XIAO ESP32-S3
- [ ] ICM-20948 at ≥100 Hz
- [ ] DIO on channel map
- [ ] ESP-NOW multi-node sync
- [ ] Open Ephys TCP (port 5000)
- [ ] SD session logging
- [ ] Host Python client + Arduino guide

### Out of Scope (v1)

- Camera capture and IMU-vs-camera verification — **deferred v2**
- TEVM-AR0234 on ESP32-S3 MIPI — hardware mismatch; see feasibility doc
- Red Pitaya firmware replacement
- OpenSim native plugin in firmware
- Medical certification

## Context

- **STEP / Red Pitaya:** TCP 5000, `REDPITAYA`/`START`, ch0–5 = ax,ay,az,gx,gy,gz @ 100 Hz.
- **Arduino primary:** `docs/arduino-ide-guide.md`; ESP-IDF in `firmware/` for advanced users.
- **Repository:** https://github.com/Minkeejung0415/ESP32-S3.git

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Arduino IDE for v1 | User workflow; faster lab iteration | — Pending |
| Camera deferred to v2 | Lower priority vs IMU/TCP/sync | ✓ Documented |
| Open Ephys TCP parity | Reuse STEP host | — Pending |
| ESP-NOW sync | Multi-node without mesh complexity | — Pending |

## Evolution

See standard GSD evolution block in prior version.

---
*Last updated: 2026-06-01 after Arduino-first reprioritization*
