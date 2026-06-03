# ESP32 fusion, filter mode, and OpenSim (v1.4+)

Firmware **v1.4.0** runs **Madgwick AHRS on the ESP32** before samples are streamed. You do **not** need Red Pitaya UDP or full `SENSORS:` layout parity for orientation or filter behavior.

## Channel map (11 channels)

| Ch | Filter OFF | Filter ON |
|----|------------|-----------|
| 0–2 | Raw accel ax, ay, az | Gravity-removed linear accel (g × 16384) |
| 3–5 | Raw gyro gx, gy, gz | Same |
| 6 | DIO (level + edge count) | Same |
| 7–10 | Quaternion qw, qx, qy, qz (int16 ÷ 32767) | Same |

Gyro and quaternion channels are unchanged when filter is on; only **ch0–2** switch to filtered linear acceleration (Red Pitaya–style “filter button on ch0” behavior).

## TCP / serial commands

| Command | Effect |
|---------|--------|
| `REDPITAYA` | `OK CHANNELS:11` + info line |
| `START` | Begin streaming; replies `STARTED` |
| `FILTER` or `FILTER 1` | Enable filter on ch0–2 |
| `FILTER 0` | Raw accel on ch0–2 |

USB bench: send `FILTER 1` on the same serial port (115200) while streaming.

## USB → PC → Open Ephys (your usual path)

1. Flash `step_node.ino` with **USB_OPEN_EPHYS_MODE** preset (`SERIAL_OUTPUT_BINARY true`).
2. `python host/serial_tcp_bridge.py COMx`
3. Open Ephys **custom Acquisition Board**: TCP client → `127.0.0.1:5000`, handshake `REDPITAYA` / `START`.
4. Wire the plugin **filter button** to send `FILTER 1` / `FILTER 0` on the command socket (or toggle from Serial Monitor for testing).

Minimal plugin change (your local `acqboard.ccp`): on filter toggle, `send("FILTER 1\n")` instead of only remapping host-side raw data.

## OpenSim

```powershell
python host\serial_tcp_bridge.py COM3
set ESP32_NODE_HOST=127.0.0.1
python host\esp32_to_opensim_bridge.py
```

Adjust `OPENSIM_UDP_HOST` / `OPENSIM_UDP_PORT` to match your OpenSim listener (default `127.0.0.1:9876`). Quaternion layout matches ch7–10 on the wire.

## Backward compatibility

- Old **8-channel** recordings: set `ESP32_NUM_CHANNELS=8` on host tools.
- `SAMPLE_HZ` in the sketch is the device packet rate; Open Ephys UI sample rate can differ.

## What we did *not* implement (by design)

- UDP port 55001 (optional later; TCP/USB is enough for your lab)
- Full Red Pitaya `SENSORS:` / multi-board discovery
- Ephys Socket–specific paths
