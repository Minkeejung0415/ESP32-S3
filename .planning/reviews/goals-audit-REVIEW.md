# Goals Audit — Code Evidence Review

**Reviewer:** gsd-code-reviewer (inline, /gsd-new-project brownfield)  
**Date:** 2026-06-02  
**Scope:** Four user goals mapped to implementation status with file:line citations.

## Executive Summary

| Goal | Status | Summary |
|------|--------|---------|
| 1. ESP-NOW multi-chip sync | **Partial** | Master send scaffold; disabled by default; slave recv/offset missing |
| 2. Stream to Open Ephys & OpenSim | **Partial** | Open Ephys USB+TCP implemented; OpenSim scripts absent from repo |
| 3. SD card logging | **Partial** | Append-only stub; disabled by default; no session header |
| 4. ICM20948 + DIO + camera | **Partial** | ICM+DIO implemented (Arduino); camera stub/deferred v2 |

---

## Goal 1: Synchronize multiple chips with ESP-NOW

**Verdict: Partial**

| Check | Status | Evidence |
|-------|--------|----------|
| Feature compile-gated off by default | ✓ | `arduino/step_node/step_node.ino:34` `#define ENABLE_ESPNOW false` |
| Master sends seq + time_us | ✓ (when enabled) | `step_node.ino:314-319` `sendEspNowSync()` |
| Master send called each frame | ✓ | `step_node.ino:548` |
| Slave receive handler | ✗ empty | `step_node.ino:137-139` `onEspNowRecv` no-op |
| Slave clock offset applied | ✗ | `SyncPacket` defined `132-135` but never consumed on slave |
| ESP-IDF recv stores master time | Partial | `firmware/main/espnow_sync.c:20-28` stores seq/time only |
| clock_offset_us computed | ✗ | `espnow_sync.h:9` declared; never written in `espnow_sync.c` |
| Kconfig default off | ✓ | `firmware/main/Kconfig.projbuild:11-13` `default n` |
| Two-board validation | ✗ | No test harness or logged proof |

**Remaining work:** Enable path, implement slave recv + offset, tag samples, Phase 2 success criteria.

---

## Goal 2: Stream data to Open Ephys & OpenSim

**Verdict: Partial** (Open Ephys yes; OpenSim missing)

### Open Ephys

| Check | Status | Evidence |
|-------|--------|----------|
| 22-byte LE header | ✓ | `step_node.ino:97-106`, `open_ephys_stream.c:24-48` |
| 100 Hz, 8×int16 | ✓ | `step_node.ino:65-66,310-311,543-546` |
| REDPITAYA/START handshake | ✓ Wi-Fi | `step_node.ino:365-371` |
| USB binary → TCP bridge | ✓ | `step_node.ino:79-87,340-344`; `host/serial_tcp_bridge.py:34-40,387-405` |
| bit_depth=3 (S16 enum) | ✓ | `step_node.ino:310`; bridge normalizes `serial_tcp_bridge.py:99-107` |
| Ephys Socket on Wi-Fi without handshake | ✗ | `docs/open-ephys-plugin.md:64-65` firmware expects text; USB bridge handles OE |
| Host TCP client | ✓ | `host/esp32_tcp_client.py:44-50,52-64` |

### OpenSim

| Check | Status | Evidence |
|-------|--------|----------|
| Bridge script in repo | ✗ | `Glob **/*opensim*` → 0 files |
| Documented external scripts | ✓ reference only | `docs/open-ephys-plugin.md:29` Plugin repo paths |
| 8-ch scale mapping doc | Partial | `docs/open-ephys-plugin.md:114` spec only |
| End-to-end OpenSim test | ✗ | Not present |

**Preset conflict (Open Ephys USB):**

| Preset | Evidence | Issue |
|--------|----------|-------|
| `USB_OPEN_EPHYS_MODE` | `step_node.ino:77-87` | Sets binary serial + no TCP — **correct for OE USB** |
| 4-wire ICM comment preset | `step_node.ino:14-20` | CSV bench, no `SERIAL_OUTPUT_BINARY` — **not Open Ephys compatible** |
| Default | `step_node.ino:77,84-86` | Wi-Fi TCP on; USB = diagnostics only until USB mode set |

Bridge detects wrong mode: `host/serial_tcp_bridge.py:122-126`.

---

## Goal 3: Save data to SD card

**Verdict: Partial**

| Check | Status | Evidence |
|-------|--------|----------|
| Disabled by default | ✓ | `step_node.ino:74` `#define ENABLE_SD false` |
| SD init in setup | Gated | `step_node.ino:493-495` `#if ENABLE_SD` |
| Append seq + channels | Partial | `step_node.ino:353-362` `/step_session.bin` raw bytes, no metadata header |
| CS pin defined | ✓ | `step_node.ino:89` `PIN_SD_CS 21` |
| ESP-IDF logger | Partial | `firmware/main/sd_logger.c:11-27` fopen `/sdcard/step_session.bin`; no mount in file |
| Stream contention test | ✗ | SD-03 not validated |

---

## Goal 4: Collect ICM20948, DIO pin & camera

**Verdict: Partial** (ICM + DIO done on Arduino; camera missing)

### ICM-20948

| Check | Status | Evidence |
|-------|--------|----------|
| I2C init + WHO_AM_I | ✓ | `step_node.ino:253-271,196-214` boot scan |
| 100 Hz read | ✓ | `step_node.ino:273-303,538-543` |
| ch0–5 in stream | ✓ | `step_node.ino:543` `readImu(channels)` |
| ESP-IDF real read | ✗ TODO | `icm20948.c:77-79` burst read not implemented |

### DIO

| Check | Status | Evidence |
|-------|--------|----------|
| GPIO init pull-up | ✓ | `step_node.ino:220-228` GPIO1 / D0 |
| Debounce 15 ms | ✓ | `step_node.ino:53,238-244` |
| Packed ch6 | ✓ | `step_node.ino:247-251,545` |

### Camera

| Check | Status | Evidence |
|-------|--------|----------|
| ch7 in stream | Stub 0 | `step_node.ino:546` `channels[7] = 0` |
| Firmware camera module | Stub | `camera_verify.c:9-17` log stub, no DVP driver |
| Feasibility doc | ✓ | `docs/camera-feasibility.md:11-17` TEVM not on S3 |
| Deferred v2 | ✓ | ROADMAP Phase 7 |

---

## Recommendations

1. **Next execute:** `/gsd:plan-phase 2` (ESP-NOW) — highest gap vs user goal 1.
2. **Do not modify** default `ENABLE_ESPNOW false` / `ENABLE_SD false` without explicit phase plan.
3. **OpenSim:** Import or submodule Plugin-repo scripts before claiming SIM-* complete.
4. **Document** USB_OPEN_EPHYS_MODE vs 4-wire CSV preset prominently in Phase 5 docs pass.

---
*Review complete — no code changes required for audit itself*
