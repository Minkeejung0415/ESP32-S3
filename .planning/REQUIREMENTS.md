# Requirements: ESP32-S3 STEP Acquisition

**Defined:** 2026-06-01  
**Updated:** 2026-06-01 — Arduino-first; camera deferred v2  
**Core Value:** Time-aligned IMU streaming at Red Pitaya–equivalent rates for STEP analytics.

## v1 Requirements

### Arduino / Firmware Core

- [ ] **ARD-01**: Arduino sketch `arduino/step_node/` builds and runs on XIAO ESP32S3
- [ ] **FW-02**: ICM-20948 sampled at 100 Hz with int16 scaling to host
- [ ] **FW-03**: DIO GPIO readable and exposed as stream channel
- [ ] **FW-04**: TCP server on port 5000 with `REDPITAYA` / `START` handshake
- [ ] **FW-05**: Open Ephys 22-byte LE header + channel-major int16 payload
- [ ] **FW-01**: ESP-IDF project builds (advanced path, optional for v1 sign-off)

### Multi-Node & Storage

- [ ] **SYNC-01**: ESP-NOW master broadcasts frame index + timestamp
- [ ] **SYNC-02**: Slave applies offset and tags samples with master seq
- [ ] **SD-01**: Session blocks written to SD with metadata header
- [ ] **SD-02**: Streaming continues during SD write (no >5% packet drop in 60 s test)

### Host Integration

- [ ] **HOST-01**: Python client parses extended channel map
- [ ] **HOST-02**: README + `docs/arduino-ide-guide.md` cover flash and Open Ephys steps
- [ ] **HOST-03**: Example config mirrors Red Pitaya env vars

## v2 Requirements (deferred)

### Camera & Verification

- **CAM-01**: DVP camera bring-up on XIAO Sense (OV3660 class)
- **CAM-02**: Motion score channel updated each frame
- **VER-01**: Action verification flag (IMU vs camera)
- **CAM-03**: TEVM-AR0234 + S32 lens production path (reference: `docs/camera-feasibility.md` ✓)
- **TEVM-01**: MIPI driver on ESP32-P4 with TEVM-AR0234
- **OPE-01**: Native Open Ephys plugin source ID
- **SIM-01**: OpenSim bridge prototype

## Out of Scope (v1)

| Feature | Reason |
|---------|--------|
| Camera streaming / verification | Deferred to v2; feasibility doc only |
| Red Pitaya firmware rewrite | Separate repo |
| Medical device submission | Research prototype |
| Technexion VizionSDK on ESP32-S3 | Wrong platform for MIPI TEVM |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| ARD-01 | Phase 1 | In Progress |
| FW-02 | Phase 1 | Pending |
| FW-03 | Phase 2 | Pending |
| FW-04 | Phase 1 | Pending |
| FW-05 | Phase 1 | Pending |
| FW-01 | Phase 6 | Pending |
| SYNC-01 | Phase 3 | Pending |
| SYNC-02 | Phase 3 | Pending |
| SD-01 | Phase 4 | Pending |
| SD-02 | Phase 4 | Pending |
| HOST-01 | Phase 5 | Pending |
| HOST-02 | Phase 5 | In Progress |
| HOST-03 | Phase 5 | Pending |
| CAM-* / VER-* | Phase 7 (v2) | Deferred |

**Coverage:**
- v1 requirements: 13 total
- Mapped to phases: 13
- Unmapped: 0 ✓

---
*Last updated: 2026-06-01 after Arduino-first reprioritization*
