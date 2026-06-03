# Phase 3: OpenSim quaternions - Context

**Gathered:** 2026-06-03

<domain>
OpenSim UDP v2 receives non-zero quaternions from ESP32 Plugin path when filter enabled.

</domain>

<decisions>
- Plugin ESP32 `run()` calls `sendOpenSimQuaternionPacket()` when `openSimEnabled && filterEnabled`
- Quaternion from ch7,3,4,5 Q15 via existing `quaternionFromScaledQ15`
- Host bench: `ESP32_FILTER_ON` in `esp32_tcp_client.py`

</decisions>

<deferred>
## Deferred
- `ephys_to_opensim_bridge.py` lives in Plugin repo (not in ESP32-S3 tree); update there when bridge file exists

</deferred>
