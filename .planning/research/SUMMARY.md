# Research Summary — v2.0 (sensor config, quaternion, OpenSim)

**Synthesized:** 2026-06-03 (inline from codebase + Plugin review; no parallel researcher agents)

## Stack (unchanged)

- **MCU:** XIAO ESP32-S3 (Arduino primary)
- **IMU:** ICM-20948 I2C @ 400 kHz
- **Host:** Python 3 (`pyserial`, `asyncio`) — `host/serial_tcp_bridge.py`, `host/esp32_tcp_client.py`
- **Acquisition:** Minkeejung0415/Plugin AcqBoardRedPitaya
- **Motion:** OpenSim 4.x via Plugin `ephys_to_opensim_bridge.py` / `opensim_live_realtime.py`

## Table stakes (must match Red Pitaya for lab trust)

| Capability | Red Pitaya | ESP32 today |
|------------|------------|-------------|
| `FREQ:` sample rate | Yes | **No** — fixed `SAMPLE_HZ 100` |
| `CFG:` sensor presets | Yes | **No** |
| `FILTER:` / fusion | Yes (firmware fusion) | **No** |
| Quaternion in stream / OpenSim | UDP quat v2 | **ch7=0**; Plugin ESP32 skips `sendOpenSimQuaternionPacket` |

## Architecture recommendation

1. **Phase 1:** Text command handler on ESP32 + Plugin ESP32 branch command forwarding (same TCP socket as samples).
2. **Phase 2:** On-device AHRS (Madgwick/Mahony) — avoids duplicating fusion in Plugin and matches RP “firmware fusion” model.
3. **Phase 3:** Pack quaternions for OpenSim in Plugin ESP32 `run()` loop (mirror RP UDP helper) or extend bridge to parse new channel layout.

## Watch out for

- **Channel map change** breaks existing 8-ch recordings — version the layout (`node=esp32s3_arduino` string bump).
- **Sample rate change** affects Open Ephys buffer sizing and Plugin `updateSampleFrequency` clamp.
- **OpenSim frame convention** (wxyz vs xyzw, sensor vs world) must match RP scripts or models look “wrong” even with data present.
- **USB bridge** must forward config commands serial↔TCP when using `--plugin`.

## Confidence

| Topic | Level | Note |
|-------|-------|------|
| Gap diagnosis (no quat) | High | Direct code + HI-02 review |
| On-device AHRS feasibility | Medium | ESP32 CPU at 100 Hz typical for Madgwick |
| Plugin CFG/FREQ on ESP32 branch | Medium | Requires C++ changes in Plugin repo |

---
*Feeds REQUIREMENTS.md and ROADMAP.md Phase 1–3*
