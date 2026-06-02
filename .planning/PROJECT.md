# ESP32-S3 Multi-Node Acquisition (STEP Extension)

## What This Is

Firmware and host tooling to add **Seeed Studio XIAO ESP32-S3** nodes to the existing **STEP** gait / rehabilitation pipeline. Each node acquires **ICM-20948 IMU** and **DIO**, logs to **SD** (later), and streams **Open Ephys–compatible** TCP packets—matching the **Red Pitaya** path (`tcp_client.py`, 100 Hz, 22-byte header + int16 channels).

**Primary integration target:** [Minkeejung0415/Plugin](https://github.com/Minkeejung0415/Plugin) **AcqBoardRedPitaya** acquisition board (`REDPITAYA` / `START` on port 5000), with **OpenSim** via Plugin-repo scripts (`ephys_to_opensim_bridge.py`, `opensim_live_realtime.py`)—not firmware.

**v1 bring-up:** Arduino IDE (`arduino/step_node/`). **Single ESP32 on USB first**, then **Wi-Fi TCP + same Plugin board**, then **ESP-NOW multi-node** (execution deferred; scaffold remains). **Camera is out of scope** for the current milestone.

## Core Value

**Time-aligned, trustworthy IMU motion data** at Red Pitaya–equivalent sample rates so STEP analytics (ZUPT, symmetry, alerts) and the **Plugin acquisition pipeline** run with minimal host changes.

## Requirements

### Validated

- ✓ **ICM-20948 @ 100 Hz (Arduino)** — `arduino/step_node/step_node.ino:253-303,538-545` WHO_AM_I probe, burst read, synthetic fallback
- ✓ **DIO on ch6 (Arduino)** — `step_node.ino:220-251,544-545` debounced level + edge count packed to int16
- ✓ **Open Ephys binary packet layout** — `step_node.ino:97-106,305-312,325-331` 22-byte LE header + 8×int16
- ✓ **USB lab alternate (Ephys Socket)** — `USB_OPEN_EPHYS_MODE` + `host/serial_tcp_bridge.py` → localhost:5000 (no Plugin repo)
- ✓ **CAM-03 reference** — `docs/camera-feasibility.md` (TEVM / S32 vs ESP32-S3)

### Active (milestone order)

1. [ ] **Plugin AcqBoard** — USB and/or Wi-Fi TCP validation with `REDPITAYA`/`START` (Plugin repo patches; checklist `.planning/PLUGIN-INTEGRATION.md`)
2. [ ] **Wi-Fi TCP** hardened for Plugin path (`USB_OPEN_EPHYS_MODE false`, `ENABLE_TCP true`)
3. [ ] **OpenSim** via Plugin pipeline (8-ch ESP32 map)
4. [ ] **SD** session logging without starving stream
5. [ ] Host docs + `esp32_tcp_client.py` parity with STEP env vars

### Deferred (execution)

- [ ] **ESP-NOW** multi-node sync — firmware scaffold only (`ENABLE_ESPNOW false` default); after end-to-end streaming stable
- [ ] **Camera** capture / IMU verification — **current milestone out of scope** (`docs/camera-feasibility.md`, v2)

### Out of Scope (current milestone)

- Camera streaming and action verification — **deferred** (not near-term roadmap)
- TEVM-AR0234 on ESP32-S3 MIPI — hardware mismatch
- Red Pitaya firmware replacement
- OpenSim native plugin inside ESP32 firmware
- **Plugin C++ implementation** in this repo — external Plugin repo only
- Medical certification

## Context

- **STEP / Red Pitaya:** TCP 5000, `REDPITAYA`/`START`, ch0–5 = ax,ay,az,gx,gy,gz @ 100 Hz.
- **Arduino primary:** `docs/arduino-ide-guide.md`; ESP-IDF in `firmware/` for advanced users.
- **Open Ephys (primary):** Plugin **AcqBoardRedPitaya** — see `docs/open-ephys-plugin.md`, `.planning/PLUGIN-INTEGRATION.md`.
- **Open Ephys (alternate lab):** Built-in **Ephys Socket** + `serial_tcp_bridge.py` (USB binary, no `REDPITAYA` on GUI side).
- **OpenSim:** Plugin-repo bridge scripts only.
- **Repository:** https://github.com/Minkeejung0415/ESP32-S3.git

## Constraints

- **Do not break `arduino/step_node.ino` default single-board behavior** — `ENABLE_ESPNOW false`, `ENABLE_SD false` remain defaults.
- **Preset conflict:** `USB_OPEN_EPHYS_MODE` (lines 79–87) overrides `ENABLE_TCP` / bench serial modes. **Plugin on Wi-Fi** needs `USB_OPEN_EPHYS_MODE false` + `ENABLE_TCP true`. **Ephys Socket USB lab** needs `USB_OPEN_EPHYS_MODE true`.
- **Plugin handshake gaps:** ESP32 does not emit `OK CHANNELS:8`, `STARTED`, or `SENSORS:`; samples on TCP not UDP 55001 — Plugin or firmware/bridge must adapt (documented, not fixed in this milestone unless scoped).
- **Second board** required only when ESP-NOW execution starts (deferred).

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Arduino IDE for v1 | User workflow; faster lab iteration | ✓ Primary path |
| **Plugin AcqBoard = primary OE path** | STEP stack + OpenSim scripts live in Plugin repo | In progress — external patches |
| Ephys Socket + USB bridge = alternate lab | Quick GUI test without building Plugin | ✓ Working path |
| Single USB node before Wi-Fi / ESP-NOW | End-to-end streaming first | ✓ Strategy |
| ESP-NOW execution deferred | Scaffold stays; validate after Plugin path | — Pending |
| SD after streaming stable | Lower risk than parallel bring-up | — Pending |
| Camera out of current milestone | User priority IMU + Plugin | ✓ Deferred |
| OpenSim via Plugin repo scripts | Existing STEP stack | — Pending |

## Assumptions (confirmed 2026-06-02)

1. **Phase 1 sign-off (Plugin)** = AcqBoard connects to ESP32 (USB via bridge and/or Wi-Fi TCP), 8 channels @ 100 Hz, DIO on ch6 — not Ephys Socket-only.
2. **Phase 1 lab alternate** = Ephys Socket + `serial_tcp_bridge.py` remains valid for quick checks without Plugin build.
3. **OpenSim** = run Plugin-repo `ephys_to_opensim_*` after AcqBoard stream; 8-ch map documented in `docs/open-ephys-plugin.md`.
4. **SD hardware** = XIAO ESP32-S3 **Sense**, CS GPIO21, `ENABLE_SD true` when Phase SD starts.
5. **ESP-NOW** = one master (`NODE_IS_MASTER true`); slave clock offset — implement after Phases 1–2 Plugin validation.
6. **ICM I2C** = 0x69 default; sketch probes 0x68/0x69.

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition** (via `/gsd-transition`):
1. Requirements invalidated? → Move to Out of Scope with reason
2. Requirements validated? → Move to Validated with phase reference
3. New requirements emerged? → Add to Active
4. Decisions to log? → Add to Key Decisions
5. "What This Is" still accurate? → Update if drifted

---
*Last updated: 2026-06-02 — Plugin AcqBoard primary; ESP-NOW/camera deferred; single-node USB first*
