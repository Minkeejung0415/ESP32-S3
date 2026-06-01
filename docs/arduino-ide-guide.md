# Arduino IDE — STEP ESP32-S3 Node

Primary bring-up path for v1: **IMU + DIO + ESP-NOW + SD + Open Ephys TCP** on **Seeed XIAO ESP32S3**. Camera and action verification are **deferred to v2** (see [camera-feasibility.md](camera-feasibility.md)).

## 1. Install Arduino IDE

1. Install [Arduino IDE 2.x](https://www.arduino.cc/en/software).
2. **File → Preferences → Additional boards manager URLs**, add:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. **Tools → Board → Boards Manager** → install **esp32** by Espressif (≥ 3.0 recommended).
4. **Tools → Board → esp32** → select **XIAO_ESP32S3** (Seeed XIAO ESP32S3).

If **XIAO_ESP32S3** is missing, add Seeed board package:
```
https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json
```
Then install **Seeed ESP32 Boards** and pick **XIAO_ESP32S3**.

## 2. Open the sketch

```
arduino/step_node/step_node.ino
```

Edit the config block at the top:

| Define | Purpose |
|--------|---------|
| `WIFI_SSID` / `WIFI_PASS` | Lab Wi-Fi |
| `PIN_I2C_SDA` / `PIN_I2C_SCL` | ICM-20948 I2C (default D4/D5 → GPIO 5/6) |
| `PIN_DIO` | Digital input (default D0 → GPIO 1) |
| `ICM20948_ADDR` | `0x69` or `0x68` if AD0 grounded |
| `NODE_IS_MASTER` | `true` = ESP-NOW sync master |
| `ENABLE_SD` | `true` on Sense board with SD wired |
| `ENABLE_TCP` | Open Ephys TCP server on port 5000 |
| `ENABLE_SERIAL_BENCH` | `true` = CSV on Serial, no TCP (desk test) |

## 3. Libraries (Board Manager / Library Manager)

Built-in with ESP32 core (no extra install):

- **WiFi**, **WiFiClient**, **WiFiServer**
- **esp_now**
- **Wire** (ICM-20948 I2C)
- **SD**, **SPI** (when `ENABLE_SD`)

Optional (not required for v1 sketch):

- **Adafruit ICM20948** — replace minimal I2C in sketch for full register map

## 4. Pin map — Seeed XIAO ESP32S3

| Signal | Default GPIO | XIAO pad | Notes |
|--------|--------------|----------|-------|
| I2C SDA | 5 | D4 | ICM-20948 |
| I2C SCL | 6 | D5 | ICM-20948 |
| DIO in | 1 | D0 | Pull-up input |
| SD CS | 21 | — | Sense expansion; verify schematic |
| 3V3 / GND | — | — | Common ground with IMU |

Confirm against [Seeed XIAO ESP32S3 wiki](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/) if your carrier differs.

## 5. Flash

1. Connect USB-C, select correct **Port**.
2. **Tools → USB CDC On Boot → Enabled** (serial monitor).
3. **Upload**.
4. **Serial Monitor @ 115200** — expect `WiFi OK IP=...` and `TCP listen :5000`.

## 6. Red Pitaya–compatible TCP (Open Ephys)

Matches STEP / Red Pitaya behavior:

| Step | Action |
|------|--------|
| Connect | TCP to node IP, port **5000** |
| Handshake | Send `REDPITAYA\n` → response lists 8 channels @ 100 Hz |
| Start | Send `START\n` |
| Payload | 22-byte little-endian header + **8 × int16** channel-major |

Channel map:

| Ch | Content |
|----|---------|
| 0–5 | ICM-20948 ax, ay, az, gx, gy, gz (int16; scale on host) |
| 6 | DIO level |
| 7 | Reserved (0 in v1; camera v2) |

Host test:

```bash
pip install numpy
set ESP32_NODE_HOST=<node-ip>
python host/esp32_tcp_client.py
```

**Open Ephys:** Ephys Socket plugin → TCP client → same IP/port and framing.

## 7. Serial bench mode (no Wi-Fi)

Set in sketch:

```cpp
#define ENABLE_SERIAL_BENCH true
#define ENABLE_TCP false
```

Upload, open Serial Monitor. CSV line per sample:

```
seq,ax,ay,az,gx,gy,gz,dio,cam
```

Plot in Python or compare to Red Pitaya `tcp_client.py` scaling env vars (`ICM_ACCEL_SCALE`, `ICM_GYRO_SCALE`).

## 8. ESP-NOW multi-node

1. Flash **master** with `NODE_IS_MASTER true`, **slaves** with `false`.
2. All nodes on same Wi-Fi channel (ESP-NOW init after `WiFi.begin`).
3. Master broadcasts `{seq, time_us}` each sample; slaves log offset (extend sketch for production sync).

## 9. SD logging

1. Set `ENABLE_SD true`.
2. Session append file: `/step_session.bin` (seq + 16 bytes channels per frame).
3. Copy SD to PC for offline analysis.

## 10. ESP-IDF path (advanced)

Same architecture in `firmware/` for teams using **idf.py** (camera hooks, FreeRTOS tasks). Arduino is recommended for lab v1 bring-up.

## Troubleshooting

| Issue | Fix |
|-------|-----|
| ICM not found | Check wiring, I2C address 0x68/0x69, run with synthetic fallback |
| No TCP client | Firewall; ping node IP; confirm `ENABLE_TCP` and not serial bench |
| ESP-NOW no sync | Same RF channel; Wi-Fi must be started before `esp_now_init` |
| SD fail | Sense CS pin, FAT32 card, `ENABLE_SD` |

---
*v1 guide — camera deferred; see `.planning/ROADMAP.md`*
