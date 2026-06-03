# Phase 2: Quaternion orientation filter - Context

**Gathered:** 2026-06-03
**Status:** Ready for planning
**Mode:** Autonomous

<domain>
## Phase Boundary

On-device Mahony AHRS produces unit quaternion on the 8-channel wire when `FILTER ON`.

</domain>

<decisions>
## Implementation Decisions

### Wire layout (locked)
- **FILTER OFF:** ch0-2 accel, ch3-5 gyro, ch6 DIO, ch7=0
- **FILTER ON:** ch0-2 accel, ch3-5 qx,qy,qz (Q15), ch6 DIO, ch7 qw (Q15)
- Gyro not exported on wire when filter enabled (OpenSim-oriented)

### Algorithm
- Mahony IMU (6-DOF), Kp=2, Ki=0.005, runs at `g_sample_hz`

### Commands
- `FILTER ON` resets integrator; `FILTER OFF` restores gyro slots

</decisions>

<code_context>
## Integration Points
- `packChannelsFromImu()` in `step_node.ino`
- Plugin scaling branch when `filterEnabled`

</code_context>

<deferred>
## Deferred Ideas
- ICM DMP fusion; 12-channel expansion

</deferred>
