# Roadmap — ESP32-S3 STEP Acquisition

**Milestone:** v1.0 — STEP-compatible streaming (Arduino-first, Plugin AcqBoard primary)  
**Phases:** Plugin USB/TCP → Wi-Fi TCP → OpenSim → SD → docs → ESP-NOW (later) → ESP-IDF optional  
**Execution order:** **(1)** single-node USB + Plugin AcqBoard → **(2)** Wi-Fi TCP + same Plugin → **(3)** OpenSim (Plugin scripts) → **(4)** SD → **(5)** docs → **ESP-NOW later** · **camera out of near-term**

## v1 Phases

### Phase 1: Single-node streaming + Plugin AcqBoard validation
**Goal:** One XIAO board streams ICM + DIO at 100 Hz into the **Plugin repo acquisition board** (`REDPITAYA` / `START`, port 5000). Sensor + binary layout already proven; **milestone exit = Plugin GUI acquires 8 channels**.

**Two host paths (same firmware packets):**

| Path | ESP32 preset | Host | Plugin repo |
|------|--------------|------|-------------|
| **A — Primary (Plugin)** | Wi-Fi: `USB_OPEN_EPHYS_MODE false`, `ENABLE_TCP true` **or** USB + TCP-forward bridge | Node IP:5000 or bridge proxying handshake | Build Plugin; patches per `docs/open-ephys-plugin.md` Path B |
| **B — Alternate (lab)** | `USB_OPEN_EPHYS_MODE true` | `serial_tcp_bridge.py` → 127.0.0.1:5000 | **Not used** — Open Ephys **Ephys Socket** only |

**Requirements:** ARD-01, SENS-ICM-01..03, SENS-DIO-01..02, STREAM-OE-01..03, STREAM-PLUGIN-01, HOST-01..02  
**Success Criteria:**
1. `python host/esp32_tcp_client.py` receives 8 ch @ 100 Hz after `REDPITAYA`/`START` (Wi-Fi path)
2. Plugin AcqBoard connects with checklist in `.planning/PLUGIN-INTEGRATION.md` complete (or documented workaround)
3. DIO on ch6 visible in acquisition
4. Ephys Socket + USB bridge path still works as regression (alternate)
1. Sketch uploads on XIAO ESP32S3
2. Host receives 100 Hz ch0–10 (11 ch) with REDPITAYA/START; quat on ch7–10
3. Serial bench mode CSV optional for desk test

**Status:** Sensors + alternate USB path **complete**; **Plugin AcqBoard sign-off in progress**

**Preset / gaps:** Plugin expects Red Pitaya dialog (`OK CHANNELS`, `STARTED`, `SENSORS`, often UDP 55001). ESP32 uses different reply text and TCP binary only — fix in **Plugin repo** (preferred) or firmware/bridge; see `docs/open-ephys-plugin.md`.

---

### Phase 2: Wi-Fi TCP + Plugin AcqBoard (production-style)
**Goal:** Same Plugin board and channel map over STA Wi-Fi (no USB cable); stable enough for short STEP sessions.  
**Requirements:** STREAM-OE-02, STREAM-OE-05, STREAM-PLUGIN-01, STREAM-PLUGIN-02  
**Depends on:** Phase 1 Plugin path understood (handshake + transport)  
**Success Criteria:**
1. Open Ephys via Plugin sources ESP32 at DHCP IP (documented in runbook)
2. TCP drop < 5% over 60 s @ 100 Hz (`esp32_tcp_client.py` or Plugin)
3. Document USB bridge vs direct Wi-Fi vs Soft AP (`STEP_ESP32`)

---

### Phase 3: OpenSim via Plugin pipeline
**Goal:** ESP32 8-channel map into OpenSim using Plugin-repo `ephys_to_opensim_bridge.py` / `opensim_live_realtime.py`.  
**Requirements:** SIM-01, SIM-02, SIM-03  
**Depends on:** Phase 1–2 (stable AcqBoard stream)  
**Success Criteria:**
1. Plugin scripts run with documented ch0–5 scale, ch6 DIO bit0, ch7 = 0
2. Recorded or live Open Ephys → bridge → OpenSim UDP localhost:5000
3. Runbook in `docs/open-ephys-plugin.md` + `.planning/PLUGIN-INTEGRATION.md`

**Note:** Scripts live in **Plugin repo**, not ESP32-S3.

---

### Phase 4: SD card logging
**Goal:** Local SD on Sense board without starving stream.  
**Requirements:** SD-01, SD-02, SD-03  
**Depends on:** Phase 2 (streaming stable)  
**Success Criteria:**
1. `ENABLE_SD true`; session file with metadata header on PC
2. Stream continues during 60 s capture (<5% drop)

---

### Phase 5: Host bridge & documentation
**Goal:** STEP operator docs, env examples, Plugin checklist linked from README.  
**Requirements:** HOST-03  
**Depends on:** Phases 1–4 findings  
**Success Criteria:**
1. README + `docs/arduino-ide-guide.md` + `docs/open-ephys-plugin.md` agree on primary vs alternate paths
2. `.env.example` or guide for `ICM_*_SCALE`, `ESP32_NODE_HOST`
3. `.planning/PLUGIN-INTEGRATION.md` kept in sync with Plugin PRs

---

### Phase 6: ESP-NOW multi-node sync (deferred execution)
**Goal:** Time-align two+ boards after Plugin/TCP path is production-trusted.  
**Requirements:** SYNC-01..04  
**Depends on:** Phase 2  
**Status:** **Deferred** — `ENABLE_ESPNOW` scaffold only; enable when second board + streaming sign-off done  
**Success Criteria:**
1. Master broadcast + slave `clock_offset_us` on samples
2. Two boards |Δt| < 2 ms over 1000 frames

---

### Phase 7: ESP-IDF parity (optional advanced)
**Goal:** `firmware/` matches Arduino for TCP/ICM/DIO.  
**Requirements:** FW-01, SENS-ICM-04, SENS-DIO-03  
**Success Criteria:** `idf.py build`; TCP layout match; ICM burst read complete

---

## Out of near-term roadmap

### Camera & action verification (v2 / deferred)
**Goal:** DVP on XIAO Sense or ESP32-P4 MIPI; IMU vs camera on ch7.  
**Requirements:** SENS-CAM-01, CAM-01, VER-01 — **not scheduled in v1.0 milestone**  
See `docs/camera-feasibility.md`

---

## Progress

| Phase | Status |
|-------|--------|
| 1 Single-node + Plugin AcqBoard | **In progress** (sensors done; Plugin sign-off pending) |
| 2 Wi-Fi TCP + Plugin | Not Started |
| 3 OpenSim (Plugin scripts) | Not Started |
| 4 SD logging | Not Started |
| 5 Host docs | In Progress |
| 6 ESP-NOW | **Deferred** |
| 7 ESP-IDF parity | Scaffold only |
| Camera (v2) | **Out of milestone** |

## Dependency graph

```
Phase 1 (USB/TCP + Plugin) ──► Phase 2 (Wi-Fi + Plugin)
         │                              │
         └──────────────┬───────────────┘
                        ▼
                 Phase 3 (OpenSim)
                        │
                        ▼
                 Phase 4 (SD) ──► Phase 5 (docs)
Phase 2 ──► Phase 6 (ESP-NOW, later)
Phase 7 (IDF) — optional, parallel after Phase 1
Camera v2 — not on v1.0 critical path
```

---
*Last updated: 2026-06-02 — reordered for Plugin primary; ESP-NOW/camera deferred*
