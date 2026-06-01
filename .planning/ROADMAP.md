# Roadmap — ESP32-S3 STEP Acquisition

**Milestone:** v1.0 — Red Pitaya–parity wireless nodes (Arduino-first)  
**Phases:** 6 active v1 + camera deferred to v2

## v1 Phases

### Phase 1: Arduino IMU + Open Ephys TCP Parity
**Goal:** 100 Hz ICM-20948 streaming on port 5000 via `arduino/step_node/`.  
**Mode:** mvp  
**Requirements:** ARD-01, FW-02, FW-04, FW-05  
**Success Criteria:**
1. Sketch uploads on XIAO ESP32S3
2. Host receives 100 Hz ch0–5 with REDPITAYA/START handshake
3. Serial bench mode CSV optional for desk test

### Phase 2: DIO Channel Extension
**Goal:** DIO on ch6.  
**Requirements:** FW-03  
**Success Criteria:**
1. Toggle visible on ch6 within 20 ms
2. 6-channel STEP clients still work if ch6–7 ignored

### Phase 3: ESP-NOW Multi-Node Sync
**Goal:** Time-align multiple nodes.  
**Requirements:** SYNC-01, SYNC-02  
**Success Criteria:**
1. Two boards |Δt| &lt; 2 ms mean over 1000 frames
2. Slave recovers after master reboot within 5 s

### Phase 4: SD Card Logging
**Goal:** Local persistence without starving TCP.  
**Requirements:** SD-01, SD-02  
**Success Criteria:**
1. Session file readable on PC
2. TCP drop &lt; 5% during 60 s logged capture

### Phase 5: Host Bridge & Documentation
**Goal:** OpenEphys/OpenSim docs + Python client.  
**Requirements:** HOST-01, HOST-02, HOST-03  
**Success Criteria:**
1. `host/esp32_tcp_client.py` runs 60 s without error
2. `docs/arduino-ide-guide.md` complete
3. GitHub repo current

### Phase 6: ESP-IDF parity (optional advanced)
**Goal:** Keep `firmware/` aligned with Arduino behavior.  
**Requirements:** FW-01  
**Success Criteria:**
1. `idf.py build` succeeds
2. TCP packet layout matches Arduino sketch

## v2 — Camera & action verification (deferred)

### Phase 7: Camera motion + verification
**Goal:** DVP or ESP32-P4 MIPI path; IMU vs camera cross-check.  
**Requirements:** CAM-01, CAM-02, VER-01, CAM-03 (reference doc done)  
**Status:** Deferred — see `docs/camera-feasibility.md`

## Progress

| Phase | Status |
|-------|--------|
| 1 | **Complete** — ICM20948 @ 0x68/0x69, USB boot diagnostics v1.2+ |
| 2 | **Complete** — DIO debounce + edge count on ch6 (v1.3.0) |
| 3 | Not Started |
| 4 | Not Started |
| 5 | In Progress — host client + docs |
| 6 | Scaffold only |
| 7 (v2) | Deferred |
