# Requirements: ESP32-S3 STEP Acquisition

**Defined:** 2026-06-01  
**Updated:** 2026-06-02 — Plugin AcqBoard primary; ESP-NOW/camera execution deferred  
**Core Value:** Time-aligned IMU streaming at Red Pitaya–equivalent rates for STEP + Plugin pipeline.

## v1 Requirements

### SENSORS — ICM-20948

- [x] **SENS-ICM-01**: Arduino sketch probes I2C 0x68/0x69, WHO_AM_I 0xEA, powers ICM — `step_node.ino:253-271`
- [x] **SENS-ICM-02**: 100 Hz sample loop reads accel+gyro int16 BE — `step_node.ino:273-303,538-543`
- [x] **SENS-ICM-03**: Synthetic IMU fallback when chip absent — `step_node.ino:295-303`
- [ ] **SENS-ICM-04**: ESP-IDF burst read (currently TODO) — `firmware/main/icm20948.c:77-79`

### SENSORS — DIO

- [x] **SENS-DIO-01**: GPIO1 pull-up input with 15 ms debounce — `step_node.ino:220-245,53`
- [x] **SENS-DIO-02**: ch6 packs level (bit0) + edge count (bits1–15) — `step_node.ino:247-251,545`
- [x] **SENS-DIO-03**: ESP-IDF DIO parity — `firmware/main/dio_input.c:14-56`

### SENSORS — Camera (**current milestone: deferred**)

- [ ] **SENS-CAM-01**: DVP camera bring-up — **deferred v2**; not in v1.0 milestone
- [x] **SENS-CAM-02**: TEVM-AR0234 feasibility documented — `docs/camera-feasibility.md`

### STREAM-OE — Open Ephys (packet + transport)

- [x] **STREAM-OE-01**: 22-byte LE Open Ephys header + 8×int16 payload — `step_node.ino:97-106,305-312`
- [x] **STREAM-OE-02**: REDPITAYA / START TCP handshake (Wi-Fi mode) — `step_node.ino:365-371,514-528`
- [x] **STREAM-OE-03**: USB serial binary + `host/serial_tcp_bridge.py` → 127.0.0.1:5000 — **Ephys Socket alternate**
- [x] **STREAM-OE-04**: ESP-IDF TCP server parity — `firmware/main/open_ephys_stream.c:24-68,118-145`
- [ ] **STREAM-OE-05**: Wi-Fi TCP stable for Plugin / `esp32_tcp_client.py` — Phase 2

### STREAM-PLUGIN — Plugin repo AcqBoard (**primary**)

- [ ] **STREAM-PLUGIN-01**: Plugin AcqBoard acquires ESP32 8 ch @ 100 Hz after `REDPITAYA`/`START` — external [Minkeejung0415/Plugin](https://github.com/Minkeejung0415/Plugin); checklist `.planning/PLUGIN-INTEGRATION.md`
- [ ] **STREAM-PLUGIN-02**: Handshake/transport gaps closed (Plugin TCP read **or** firmware `OK`/`STARTED`/`SENSORS` **or** UDP 55001) — `docs/open-ephys-plugin.md`
- [ ] **STREAM-PLUGIN-03**: USB path documented when bridge vs direct Wi-Fi is required for Plugin

### STREAM-SIM — OpenSim (Plugin repo)

- [ ] **SIM-01**: OpenSim bridge runbook tied to Plugin `ephys_to_opensim_bridge.py` — external repo
- [ ] **SIM-02**: 8-channel ESP32 scale map for OpenSim UDP path — `docs/open-ephys-plugin.md`
- [ ] **SIM-03**: End-to-end Open Ephys (Plugin source) → OpenSim — Phase 3

### SYNC — ESP-NOW (**execution deferred**)

Firmware scaffold retained; validation **after** Phase 1–2 streaming.

- [ ] **SYNC-01**: Master broadcasts seq + timestamp — `step_node.ino:314-320` (`ENABLE_ESPNOW false` default)
- [ ] **SYNC-02**: Slave receives and stores master time — recv stub Arduino `step_node.ino:137-139`
- [ ] **SYNC-03**: Slave applies `clock_offset_us` to sample timestamps — not implemented
- [ ] **SYNC-04**: Two-board validation |Δt| < 2 ms — Phase 6

### SD — Session logging (Phase 4)

- [ ] **SD-01**: SD init on Sense CS GPIO21 — `ENABLE_SD false` default
- [ ] **SD-02**: Session file with metadata header
- [ ] **SD-03**: Stream continues during SD write (<5% drop / 60 s)

### Host / Arduino core

- [x] **ARD-01**: Arduino sketch builds for XIAO ESP32S3 — `step_node.ino` v1.3.0
- [x] **HOST-01**: `host/esp32_tcp_client.py` parses 8-ch Open Ephys frames
- [x] **HOST-02**: `docs/arduino-ide-guide.md` covers USB Ephys Socket + presets
- [ ] **HOST-03**: Example STEP env config committed — Phase 5

### Firmware advanced

- [ ] **FW-01**: ESP-IDF project full parity — Phase 7 optional

## v2 Requirements (deferred)

- **CAM-01**: OV3660 DVP motion score on ch7
- **VER-01**: IMU vs camera action verification
- **TEVM-01**: MIPI on ESP32-P4 + TEVM-AR0234
- **OPE-01**: Native Open Ephys Plugin C++ — lives in Plugin repo, not ESP32-S3

## Out of Scope (current milestone)

| Feature | Reason |
|---------|--------|
| **Camera** streaming / verification | User decision — deferred v2; not in v1.0 roadmap |
| Plugin C++ in ESP32-S3 repo | External Plugin repo only |
| OpenSim in firmware | Host-side Plugin scripts |
| Red Pitaya firmware rewrite | Separate repo |
| ESP-NOW multi-node **execution** | After single-node Plugin path stable |
| Medical device submission | Research prototype |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| SENS-ICM-01..03 | 1 | Complete |
| SENS-ICM-04 | 7 | Pending |
| SENS-DIO-01..02 | 1 | Complete |
| SENS-DIO-03 | 7 | Pending |
| SENS-CAM-01 | v2 | **Deferred (milestone)** |
| SENS-CAM-02 | 0 | Complete |
| STREAM-OE-01..04 | 1 | Complete |
| STREAM-OE-05 | 2 | Pending |
| STREAM-PLUGIN-01..03 | 1–2 | Pending |
| SIM-01..03 | 3 | Pending |
| SYNC-01..04 | 6 | **Deferred** |
| SD-01..03 | 4 | Pending |
| ARD-01, HOST-01..02 | 1 | Complete |
| HOST-03 | 5 | Pending |
| FW-01 | 7 | Pending |

**Coverage:** v1 requirements mapped to phases; camera and ESP-NOW execution explicitly deferred.

---
*Last updated: 2026-06-02*
