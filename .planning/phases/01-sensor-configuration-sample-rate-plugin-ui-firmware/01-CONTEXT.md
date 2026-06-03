# Phase 1: Sensor configuration & sample rate (Plugin UI ↔ firmware) - Context

**Gathered:** 2026-06-03
**Status:** Ready for planning
**Mode:** Autonomous (smart discuss — recommendations accepted)

<domain>
## Phase Boundary

Plugin and firmware agree on `FREQ:` and `CFG` so sample rate and ICM full-scale presets change device behavior and are reflected in the REDPITAYA handshake (`sample_rate=`).

</domain>

<decisions>
## Implementation Decisions

### Command surface
- Mirror Red Pitaya text commands: `FREQ:50|100|200`, `CFG 0 ACC|GYR <0-3>`, optional `CFG 0 SRATE <Hz>`
- Allowed sample rates: **50–200 Hz** (default 100)
- Presets align with Plugin `kAccSensitivity` / `kGyrSensitivity` tables

### Transport
- Wi-Fi TCP: replies on same socket as binary stream
- USB + `serial_tcp_bridge.py --plugin`: forward `FREQ`/`CFG`/`FILTER` to serial during acquisition

### Plugin
- Parse `sample_rate=` from ESP32 handshake; send `FREQ:` before `START`; `updateSampleFrequency` writes `FREQ:` for ESP32 nodes (no silent 100 Hz lock)

### Claude's Discretion
- USB bridge may keep static handshake text when firmware does not echo text in binary USB mode

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `step_node.ino` `handleLine()`, ICM bank-2 registers
- Plugin `acqboard.ccp` RP command paths and preset tables

### Integration Points
- `arduino/step_node/step_node.ino`, `C:\Users\justi\Plugin\acqboard.ccp`, `host/serial_tcp_bridge.py`

</code_context>

<specifics>
## Specific Ideas

Match Plugin review MD-04: sample-rate UI must not be a no-op on ESP32.

</specifics>

<deferred>
## Deferred Ideas

Multi-sensor `CFG` indices beyond sensor 0.

</deferred>
