---
phase: plugin-esp32-integration
reviewed: 2026-06-02T00:00:00Z
depth: deep
commit: e298679c03ae7c6833c6401f5ad2f1e33ef5f0ae
parent: 166583f
files_reviewed: 7
files_reviewed_list:
  - acqboard.ccp
  - devices/redpitaya/AcqBoardRedPitaya.h
  - Acqboardredpitaya.h
  - devicethread.cpp
  - device editor.cpp
  - device editor.h
  - ephys_to_opensim_bridge.py
findings:
  critical: 1
  warning: 4
  info: 3
  total: 8
status: issues_found
verdict: BLOCK
---

# Plugin ESP32-S3 integration — Code Review

**Reviewed:** 2026-06-02  
**Commit:** `e298679` — feat(acqboard): ESP32 TCP path without breaking Red Pitaya  
**Parent:** `166583f`  
**Repo:** `C:\Users\justi\Plugin`  
**Depth:** deep (cross-file + firmware/protocol cross-check vs `ESP32-S3` `step_node.ino`, `host/esp32_tcp_client.py`)

## Verdict: **FLAG** (BL-01 fixed in Plugin `217425a`; HI-02 OpenSim-on-ESP32 path still open)

**Fixes applied:** `kMaxHostsToTry` + deduped hosts in `connectCommandSocketToBoard()`; ESP32 TCP `run()` breaks on read ≤0.

Red Pitaya regression risk is **low** in static review: the UDP `run()` path is gated behind `if (!isEsp32Node)` and the RP handshake/STARTED/SENSORS logic is unchanged when `response.contains("OK")`. One **new** stack buffer overflow in `connectCommandSocketToBoard()` must be fixed before ship; without it, labs using `ESP32_NODE_HOST` + editor Node IP + default `rp-*.local` fallbacks can hit undefined behavior on every `startAcquisition()`.

---

## Summary

The dual-path design (`isEsp32Node` + early return in `run()`) is sound and matches firmware (`REDPITAYA` text reply, `START` → TCP binary, 22-byte LE header, 8×int16). Header field offsets (`num_bytes` @+4, `element_size` @+10) align with Python `struct "<iiHiii"` and `OeHeader` in `step_node.ino`. Red Pitaya behavior is preserved on the non-ESP32 branch.

**Blockers:** `hostsToTry[]` is too small after adding `configurableNodeHost` and `ESP32_NODE_HOST` to the connect retry list.

**High:** ESP32 TCP `run()` does not detect peer disconnect; OpenSim UDP forwarding from the plugin is absent on the ESP32 path.

**Lab sign-off still required** for RP UDP 55001, STARTED/SENSORS, and ESP32 TCP streaming on hardware.

---

## Narrative Findings (AI reviewer)

### BLOCKER

#### BL-01: `hostsToTry` stack buffer overflow in `connectCommandSocketToBoard()`

**File:** `acqboard.ccp:214-234`  
**Issue:** Array size is `kNumRedPitayaHosts + 2` (= **4**), but up to **5** distinct hosts can be pushed: `activeRedPitayaHost`, `configurableNodeHost`, `envEsp32NodeHost()`, plus two `kRedPitayaHosts` entries when all differ. Parent commit used `+ 1` with at most 3 entries; this commit added two host slots without resizing the array.  
**Impact:** Undefined behavior on stack during `startAcquisition()` → reconnect; wrong host selection or crash.  
**Fix:** Size the buffer for all sources, e.g. `String hostsToTry[kNumRedPitayaHosts + 4];`, or use `juce::StringArray hostsToTry` and `add()` without a fixed cap.

```cpp
// Example: safe upper bound
String hostsToTry[kNumRedPitayaHosts + 4];
```

---

### HIGH

#### HI-01: ESP32 TCP `run()` ignores socket disconnect (hang until manual stop)

**File:** `acqboard.ccp:1091-1098, 1081-1164`  
**Issue:** When `read()` returns `<= 0` (peer closed, Wi-Fi drop), the loop keeps polling `waitUntilReady` and never sets `threadShouldExit` or clears `deviceFound`. Acquisition appears “running” with flatlined or frozen buffers.  
**Fix:** On `nRead <= 0` after a successful connection, log, break out of the outer loop, and/or call `signalThreadShouldExit()`; surface status to the editor.

#### HI-02: In-plugin OpenSim UDP path not invoked for ESP32

**File:** `acqboard.ccp:1074-1166` (ESP32 branch) vs `1270-1312` (RP UDP branch)  
**Issue:** `launchOpenSimMotion()` / `launchOpenSimLive()` set `openSimEnabled`, but the ESP32 TCP branch never calls `sendOpenSimQuaternionPacket()`. Red Pitaya quaternion → UDP v2 forwarding is unchanged; ESP32 users only get bridge help if they run `ephys_to_opensim_bridge.py` with `OPENSIM_ESP32_8CH=1` and a separate scaled-IMU feed (not from plugin UDP).  
**Fix:** Document as unsupported for v1, or add ESP32 branch that packs ch0–5 scaled floats (and optional AHRS) into the existing UDP helpers.

#### HI-03: Project load sets Node IP but does not re-detect board

**File:** `device editor.cpp:1222-1228`  
**Issue:** `loadVisualizerEditorParameters` calls `setNodeHost()` but not `retryDetection()`. After reload, GUI may still reflect a prior RP/ESP32 detection while `NodeHost` XML points at ESP32.  
**Fix:** After load, if `NodeHost` non-empty and acquisition inactive, call `retryDetection()` (or defer until editor shown).

---

### MEDIUM

#### MD-01: ESP32 `startAcquisition()` opens a fresh TCP socket and sends only `START\n`

**File:** `acqboard.ccp:457-487`  
**Issue:** New session does not re-send `REDPITAYA\n`. Acceptable for current `step_node.ino` (`START` alone sets `streaming = true`), but diverges from `esp32_tcp_client.py` and docs; future firmware that gates binary on handshake would break.  
**Fix:** Mirror Python: send `REDPITAYA\n`, read/discard one line, then `START\n` (or document firmware contract).

#### MD-02: Handshake heuristic may false-positive on non-ESP32 text devices

**File:** `acqboard.ccp:31-36, 197-207`  
**Issue:** `isEsp32HandshakeResponse()` matches any line containing both `"8 channels"` and `"sample_rate"` without `esp32s3` / `node=esp32`.  
**Fix:** Require `node=esp32` or `esp32s3` substring (firmware always includes `node=esp32s3_arduino`).

#### MD-03: Detection order — Red Pitaya always wins when both online

**File:** `acqboard.ccp:265-310`  
**Issue:** `detectBoard()` probes `rp-f0f85a.local` / `rp-f0cd35.local` before `configurableNodeHost` / `ESP32_NODE_HOST`. Expected for dual-device labs; ESP32-only labs must disconnect RP or set Node IP and use **retry** after RP probe fails.  
**Fix:** None required if documented; optional “prefer Node IP first” toggle.

#### MD-04: Sample-rate editor still editable on ESP32 (no-op at 100 Hz)

**File:** `acqboard.ccp:698-703`, `device editor.cpp` (sample rate label)  
**Issue:** `updateSampleFrequency` clamps ESP32 to 100 Hz but UI does not grey out the field.  
**Fix:** When `getIsEsp32Node()`, disable sample rate editor or show “100 (fixed)”.

---

### LOW

#### LO-01: ESP32 framing resync drops one byte per bad `element_size`

**File:** `acqboard.ccp:1107-1111`  
**Issue:** Same strategy as many framing parsers; noisy links may spam logs. Acceptable for v1.

#### LO-02: Duplicate headers `Acqboardredpitaya.h` / `devices/redpitaya/AcqBoardRedPitaya.h`

**File:** both headers  
**Issue:** Both updated in commit; drift risk is pre-existing repo convention. Keep in sync on future edits.

#### LO-03: `ephys_to_opensim_bridge.py` — `OPENSIM_ESP32_8CH` only affects sensor name map

**File:** `ephys_to_opensim_bridge.py:79-81`  
**Issue:** Flag does not change UDP packet parsing; ESP32 still needs IMU float packets or AHRS path. Correct for scope; operators must set env before launch.

---

## Regression checklist — Red Pitaya (`isEsp32Node == false`)

| Check | Risk | Static review |
|-------|------|----------------|
| Handshake requires `OK` + `CHANNELS:N` | Low | **Preserved** — ESP32 branch only when `OK` absent and `isEsp32HandshakeResponse()` |
| `START` → `STARTED` / `SENSORS:` parsing | Low | **Preserved** — ESP32 returns before FREQ/STARTED block |
| UDP bind port **55001** in `run()` | Low | **Preserved** — ESP32 early-return; RP path unchanged |
| `FREQ:`, `FILTER`, `CFG`, `RECORD` on TCP | Low | **Preserved** — non-ESP32 path only |
| Per-sensor scale + quaternion OpenSim UDP v2 | Low | **Preserved** — only in UDP `run()` loop |
| Default hosts `rp-f0f85a.local`, `rp-f0cd35.local` first in `detectBoard()` | Low | **Preserved** |
| OpenSim bridge scripts default behavior | Low | **Preserved** — optional `OPENSIM_ESP32_8CH` only |
| `connectCommandSocketToBoard()` host list | **High (new)** | **Regression risk** — stack overflow when env + editor + RP hosts all used (BL-01) |

---

## Recommended fixes (report only — do not implement here)

1. **BL-01 (required):** Resize `hostsToTry` or use dynamic `StringArray` before merge/build.
2. **HI-01:** Treat TCP `read() <= 0` as disconnect; exit `run()` cleanly.
3. **HI-03:** Call `retryDetection()` after loading `NodeHost` from XML when idle.
4. **MD-01:** Optional `REDPITAYA\n` before `START\n` on ESP32 reconnect for parity with `esp32_tcp_client.py`.
5. **MD-02:** Tighten `isEsp32HandshakeResponse()` to require firmware-specific tokens.

---

## Test plan (hardware)

### Red Pitaya (regression)

1. Build GUI with Plugin sources; connect to `rp-*.local`.
2. Confirm log: `OK CHANNELS:N` and `STARTED` / `SENSORS:`.
3. Confirm UDP 55001 data; filter/rate/CFG commands work.
4. OpenSim Gen Motion / Live — quaternion UDP v2 unchanged.

### ESP32-S3

1. Firmware: `USB_OPEN_EPHYS_MODE false`, `ENABLE_TCP true`; note IP.
2. Pre-check: `python host\esp32_tcp_client.py` (ESP32-S3 repo).
3. Set Node IP or `ESP32_NODE_HOST`; retry detection → `Detected ESP32-S3 node`.
4. Start acquisition → 8 ch @ ~100 Hz; DIO ch6 bit0 toggles.
5. Stop/start twice; disconnect Wi-Fi mid-stream — verify HI-01 behavior after fix.
6. OpenSim: `OPENSIM_ESP32_8CH=1` + bridge; expect single-IMU map, no RP quat tail.

### Stress BL-01

1. Set editor Node IP, `ESP32_NODE_HOST`, and have both `rp-*.local` entries reachable or configured.
2. Start acquisition repeatedly; run under debugger / ASAN if available — verify no stack corruption.

---

## How ESP32 is selected

1. **Auto:** After `rp-*.local` fails, try `configurableNodeHost` then `ESP32_NODE_HOST`; handshake matches `esp32s3` / `node=esp32` / (`8 channels` + `sample_rate`).
2. **Manual:** Editor **Node IP** → `retryDetection()` when not acquiring.
3. **Red Pitaya:** Reply contains `OK` → `isEsp32Node = false`.

---

_Reviewed: 2026-06-02_  
_Reviewer: gsd-code-reviewer (adversarial)_  
_Depth: deep_  
_Supersedes prior PASS note in same file — commit `e298679` review_
