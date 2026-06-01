# Architecture — ESP32-S3 Multi-Node Acquisition

## Components

```
┌─────────────────┐     ESP-NOW      ┌─────────────────┐
│  Node A (Master)│◄────────────────►│  Node B (Slave) │
│  ICM20948       │                  │  ICM20948       │
│  DIO + Camera   │                  │  DIO + Camera   │
│  SD + Wi-Fi TCP │                  │  SD (optional)  │
└────────┬────────┘                  └─────────────────┘
         │ TCP :5000 (Open Ephys packets)
         ▼
┌─────────────────┐      queue       ┌─────────────────┐
│  Host PC        │ ───────────────► │ STEP Python     │
│  tcp_client.py  │                  │ gait_analysis   │
└─────────────────┘                  └─────────────────┘
```

## Tasks (FreeRTOS)

1. **imu_task** — DRDY or timer @ 100 Hz, push to ring buffer
2. **dio_task** — GPIO ISR → debounced events
3. **camera_task** — Frame grab @ 10–30 Hz (verification rate), compute motion metric
4. **verify_task** — Cross-check IMU state vs camera score
5. **sync_task** — ESP-NOW send/recv, apply clock offset
6. **stream_task** — TCP server, packetize Open Ephys frames
7. **sd_task** — Async flush from ring buffer

## Data Flow

IMU/DIO/camera → `sample_frame_t` (timestamp_us, seq, channels[]) → stream queue + SD queue → TCP/SD consumers.

## Camera Feasibility Branches

| Branch | Hardware | When |
|--------|----------|------|
| A | XIAO Sense + OV3660 DVP | Immediate firmware bring-up |
| B | ESP32-P4 + TEVM-AR0234 MIPI | Target global-shutter path |
| C | TEVM on NXP EVK → Ethernet/USB → host | If lab already has TEVM NXP kit |

## Build Order

1. IMU + TCP stream (Red Pitaya parity)
2. DIO + channel extension
3. SD logging
4. ESP-NOW sync
5. DVP camera motion score
6. TEVM-AR0234 feasibility (parallel hardware spike)
