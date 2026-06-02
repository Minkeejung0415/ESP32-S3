# Plugin ESP32-S3 integration review

**Date:** 2026-06-02  
**Repo:** `C:\Users\justi\Plugin` (Minkeejung0415/Plugin)  
**Scope:** AcqBoardRedPitaya dual-path (Red Pitaya + ESP32-S3 STEP node)

## Verdict: **PASS** (with lab verification pending)

Implementation adds ESP32 support via `isEsp32Node` flag in existing `AcqBoardRedPitaya` without removing Red Pitaya code paths. Hardware-in-the-loop tests on both targets are still required before production sign-off.

---

## Findings

### HIGH — none

No blocking defects identified in static review.

### MEDIUM

| ID | Finding | Mitigation |
|----|---------|------------|
| M1 | **OpenSim quaternion path inactive on ESP32** — firmware has no fusion quat slots; `launchOpenSimLive` / UDP v2 quat forwarding will not produce meaningful motion without host-side fusion. | Use `OPENSIM_ESP32_8CH=1` in bridge for single-IMU map; defer full OpenSim until Phase 3 or add fusion. |
| M2 | **Detection order** — `detectBoard()` probes `rp-*.local` before configurable ESP32 host. Labs with both devices online will always bind Red Pitaya first. | Expected; disconnect RP or use dedicated PC if ESP32-only lab. |
| M3 | **Sample rate UI on ESP32** — editor still allows editing Hz label; `updateSampleFrequency` no-ops at 100 Hz for ESP32. | Cosmetic; consider greying out rate field when `getIsEsp32Node()`. |

### LOW

| ID | Finding | Notes |
|----|---------|-------|
| L1 | `connectCommandSocketToBoard()` connects TCP without re-handshake (pre-existing RP behavior). | Acceptable; `startAcquisition` reopens session. |
| L2 | ESP32 TCP reader drops one byte on bad `elem` field (resync). | Matches framing test pattern; monitor for log spam on noisy links. |
| L3 | Duplicate header files `Acqboardredpitaya.h` and `devices/redpitaya/AcqBoardRedPitaya.h` both updated. | Follow existing repo convention. |

---

## Regression risk checklist — Red Pitaya

| Check | Risk | Status |
|-------|------|--------|
| Handshake requires `OK` + `CHANNELS:N` | Low | **Preserved** — ESP32 branch only when `OK` absent and esp32 pattern matches |
| `START` → `STARTED` / `SENSORS:` parsing | Low | **Preserved** — ESP32 returns early before FREQ/STARTED block |
| UDP bind port **55001** in `run()` | Low | **Preserved** — ESP32 uses separate early return; RP path unchanged |
| `FREQ:`, `FILTER`, `CFG`, `RECORD` commands | Low | **Preserved** — only sent on non-ESP32 path |
| Per-sensor scale + quaternion OpenSim UDP | Low | **Preserved** — only in UDP `run()` loop |
| Default hosts `rp-f0f85a.local`, `rp-f0cd35.local` | Low | **Preserved** — tried first in `detectBoard()` |
| OpenSim bridge scripts intact | Low | **Preserved** — optional `OPENSIM_ESP32_8CH` flag only |

---

## Test plan

### Red Pitaya (regression)

1. Build Open Ephys GUI with patched Plugin source copied into acquisition-board plugin tree.
2. Connect to real Red Pitaya (`rp-*.local`).
3. Verify Serial/GUI log: `OK CHANNELS:N` handshake.
4. Start acquisition → expect `STARTED` (+ optional `SENSORS:`).
5. Confirm UDP 55001 data in GUI (channels match sensor layout).
6. Toggle filter, sample rate, sensor CFG — confirm TCP commands still work.
7. Run `diagnose_oe_udp.py` or existing UDP test scripts if available.

### ESP32-S3 STEP node

1. Flash firmware: `USB_OPEN_EPHYS_MODE false`, `ENABLE_TCP true`; note Wi-Fi IP.
2. Pre-verify: `set ESP32_NODE_HOST=<ip>` → `python host\esp32_tcp_client.py` (ESP32-S3 repo).
3. Set `ESP32_NODE_HOST` env **or** enter IP in Plugin editor **Node IP** field → retry detection.
4. Start acquisition → log should show `Detected ESP32-S3 node` / `TCP binary, 8 ch @ 100 Hz`.
5. GUI: 8 channels named AccX…GyrZ, DIO, Reserved @ ~100 Hz.
6. DIO: toggle wired input; ch6 (DIO) bit0 should follow.
7. Stop/start twice — no stale framing (TCP session reopen).

### OpenSim (optional)

1. Red Pitaya: existing `Gen Motion` / `OpenSim Live` flow unchanged.
2. ESP32: set `OPENSIM_ESP32_8CH=1` before bridge; expect single-sensor map only.

---

## Files changed (Plugin repo)

| File | Change |
|------|--------|
| `acqboard.ccp` | Handshake branch, ESP32 `startAcquisition`, TCP `run()` loop, host env, `retryDetection` |
| `devices/redpitaya/AcqBoardRedPitaya.h` | `isEsp32Node`, `configurableNodeHost`, API |
| `Acqboardredpitaya.h` | Mirror header API |
| `devicethread.cpp` | ESP32 channel names |
| `device editor.cpp` / `.h` | Node IP field, save/load, re-detect |
| `ephys_to_opensim_bridge.py` | `OPENSIM_ESP32_8CH` sensor map flag |

---

## How ESP32 is selected

1. **Auto:** After Red Pitaya hosts fail, try `configurableNodeHost` then `ESP32_NODE_HOST` env; handshake reply containing `esp32s3`, `node=esp32`, or `8 channels` + `sample_rate` sets `isEsp32Node=true`.
2. **Manual:** Enter IP in editor **Node IP** → triggers `retryDetection()`.
3. **Red Pitaya:** Reply contains `OK` → `isEsp32Node=false` (unchanged behavior).

---

## Build note

Plugin `.ccp` / editor sources must be copied into the Open Ephys **Acquisition Board** plugin project in the GUI repo and rebuilt (see Plugin `README.md`). This workspace holds reference sources only.
