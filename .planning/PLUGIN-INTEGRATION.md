# Plugin repo integration checklist (external)

**Repo:** [Minkeejung0415/Plugin](https://github.com/Minkeejung0415/Plugin) — **not** implemented in ESP32-S3 workspace.  
**Primary Open Ephys path:** custom **AcqBoardRedPitaya** source plugin + ESP32 TCP `REDPITAYA` / `START`.  
**OpenSim path:** `ephys_to_opensim_bridge.py`, `opensim_live_realtime.py` in Plugin repo (after AcqBoard streams).

Detail and gap analysis: [docs/open-ephys-plugin.md](../docs/open-ephys-plugin.md).

---

## Milestone sign-off order (this project)

| Step | ESP32-S3 repo | Plugin repo | Host test |
|------|---------------|-------------|-----------|
| **1** Single-node USB lab stream | `USB_OPEN_EPHYS_MODE true` + `serial_tcp_bridge.py` | Optional (Ephys Socket alternate only) | Open Ephys Ephys Socket @ 127.0.0.1:5000 |
| **1b** Plugin AcqBoard on USB | `USB_OPEN_EPHYS_MODE true` + `serial_tcp_bridge.py COMx --plugin` | Build GUI with Plugin (`217425a` + `e298679`) | Plugin **Node IP `127.0.0.1`**:5000; 8 ch @ 100 Hz |
| **2** Plugin AcqBoard on Wi-Fi | `USB_OPEN_EPHYS_MODE false`, `ENABLE_TCP true`, Wi-Fi STA | Same Plugin patches + configurable ESP32 IP | `python host/esp32_tcp_client.py` then Plugin → node IP:5000 |
| **3** OpenSim | — | Run `ephys_to_opensim_bridge.py` / `opensim_live_realtime.py` with ESP32 8-ch map | Open Ephys recording → bridge → OpenSim UDP :5000 |
| **4** SD | `ENABLE_SD true` (Sense) | — | No Plugin change |
| **Later** ESP-NOW | `ENABLE_ESPNOW true` (after streaming stable) | — | Multi-board sync validation |

---

**Gap:** ~~Plugin may require `STARTED`/`SENSORS` and/or UDP 55001~~ → **Resolved in Plugin** (`e298679`): ESP32 uses TCP binary stream; RP path unchanged.

---

## Plugin repo — implementation status (2026-06-02)

**Local path:** `C:\Users\justi\Plugin`  
**Commit:** `e298679` — `feat(acqboard): ESP32 TCP path without breaking Red Pitaya`  
**Review:** [.planning/reviews/plugin-esp32-REVIEW.md](reviews/plugin-esp32-REVIEW.md)

- [x] **`performDetectionHandshake()`** — accept ESP32 reply (`8 channels` / `sample_rate=100` / `node=esp32s3`) without requiring `OK CHANNELS:N` only
- [x] **Host list** — configurable ESP32 IP (`Node IP` editor field + `ESP32_NODE_HOST` env); `rp-*.local` tried first
- [x] **`startAcquisition()`** — ESP32 branch skips `STARTED` / `SENSORS:`; fixed 8-ch ICM20948 layout
- [x] **`run()` transport** — **4a:** ESP32 reads binary from TCP after `START` (mirrors `host/esp32_tcp_client.py`); Red Pitaya stays UDP 55001
- [x] **Scaling** — ch0–5 ICM int16 (±2g / ±250°/s presets); ch6 DIO bit0; ch7 = 0
- [x] **Optional UI** — `Node IP` field + `retryDetection()` (no separate subclass)
- [x] **OpenSim scripts** — `OPENSIM_ESP32_8CH=1` maps single `torso_imu` (quat path N/A on ESP32 firmware)

---

## Plugin repo — required changes (checklist)

Do **not** duplicate C++ here; track in Plugin repo issues/PRs.

- [x] **`performDetectionHandshake()`** — accept ESP32 reply (`8 channels` / `sample_rate=100`) without requiring `OK CHANNELS:N` only
- [x] **Host list** — configurable ESP32 IP (not only `rp-*.local`)
- [x] **`startAcquisition()`** — tolerate missing `STARTED` / `SENSORS:` lines; fixed 8-ch ESP32 layout
- [x] **`run()` transport** — **4a:** read binary from TCP after `START` (match `host/esp32_tcp_client.py`), **or** **4b:** add UDP 55001 to ESP32 firmware
- [x] **Scaling** — ch0–5 ICM int16; ch6 DIO bit0; ch7 = 0 (no quaternion tail unless firmware adds fusion)
- [x] **Optional** — `AcqBoardEsp32S3` subclass + editor IP field → implemented as `isEsp32Node` flag + Node IP field
- [x] **OpenSim scripts** — confirm channel map matches fixed 8-ch ESP32 (not Red Pitaya quaternion slots)

---

## ESP32-S3 firmware / host — preset matrix

| Goal | `USB_OPEN_EPHYS_MODE` | `ENABLE_TCP` | Plugin AcqBoard |
|------|----------------------|--------------|-----------------|
| Ephys Socket lab (alternate) | `true` | off (USB serial binary) | Not used — bridge → localhost:5000 |
| Plugin on Wi-Fi | `false` | `true` | Node IP:5000; send `REDPITAYA`/`START` |
| Plugin on USB via bridge | `true` | `serial_tcp_bridge.py COMx --plugin` | Node IP **127.0.0.1**; bridge emits handshake + TCP binary |

**Known gaps (Wi‑Fi firmware path only):**

- ESP32 TCP firmware does not send `OK CHANNELS:8`, `STARTED`, or `SENSORS:` (Plugin `e298679` tolerates ESP32 reply).
- Plugin expects **UDP 55001** for Red Pitaya; ESP32 (and USB bridge) stream on **same TCP socket** after `START`.

**USB bridge mitigation (step 1b):** `host/serial_tcp_bridge.py --plugin` — full text handshake on localhost + serial binary. Ephys Socket lab path: same script **without** `--plugin`.

---

## Verification commands

```powershell
# Wi-Fi — firmware handshake (before Plugin GUI)
set ESP32_NODE_HOST=<node-ip>
python host\esp32_tcp_client.py

# USB — Ephys Socket alternate (no Plugin)
python host\serial_tcp_bridge.py COM5
# Open Ephys: Ephys Socket → 127.0.0.1:5000

# USB — Plugin AcqBoard (no Wi-Fi)
python host\serial_tcp_bridge.py COM5 --plugin
# Open Ephys Plugin: Node IP 127.0.0.1:5000
set ESP32_NODE_HOST=127.0.0.1
python host\esp32_tcp_client.py
```

Plugin GUI: build from Plugin repo, select Red Pitaya / ESP32 board. Wi‑Fi: node IP from Serial Monitor. USB bridge: **Node IP `127.0.0.1`** with `--plugin` running.

---

*Last updated: 2026-06-02 — Plugin patches applied locally @ e298679; HIL verification pending.*
