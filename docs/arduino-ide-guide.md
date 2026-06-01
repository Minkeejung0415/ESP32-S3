# Arduino IDE — STEP ESP32-S3 Node

Primary bring-up path for v1: **one Seeed XIAO ESP32S3** — IMU + DIO + Open Ephys TCP. **ESP-NOW is optional** (off by default). Camera deferred to v2 ([camera-feasibility.md](camera-feasibility.md)).

**Full wiring diagram:** [wiring-diagram.md](wiring-diagram.md) (Mermaid + ASCII, v1.3.0 pin table)

## Single board quick test

Use **one** XIAO ESP32S3 only — no second node, no ESP-NOW setup.

1. Open `arduino/step_node/step_node.ino`; confirm **`ENABLE_ESPNOW false`** (default).
2. Set **`WIFI_SSID`** / **`WIFI_PASS`**; upload to **XIAO_ESP32S3** (USB CDC On Boot enabled).
3. Serial Monitor @ 115200 → note **`WiFi OK IP=...`** and **`ESP-NOW disabled — single-node mode`**.
4. **TCP test:** `set ESP32_NODE_HOST=<that IP>` then `python host/esp32_tcp_client.py` (port 5000).
5. **Or serial bench:** set **`ENABLE_SERIAL_BENCH true`** and **`ENABLE_TCP false`**, re-upload; watch CSV lines on Serial — no Wi-Fi or host required.

When you add a second board later, set **`ENABLE_ESPNOW true`** on both (master + slave) — see §8.

## Test over USB (no Wi-Fi)

Use when the board is **USB-connected to the PC only** — no router, no TCP.

1. In `arduino/step_node/step_node.ino` set:
   ```cpp
   #define ENABLE_TCP false
   #define ENABLE_SERIAL_BENCH true
   #define ENABLE_ESPNOW false
   ```
2. **Tools → USB CDC On Boot → Enabled**; upload; pick the board’s **COM port**.
3. Optional **`SERIAL_OUTPUT_BINARY true`** — same 22-byte Open Ephys header + 8× int16 as TCP, sent over Serial (for host parsers).
4. Default (**`SERIAL_OUTPUT_BINARY false`**) — one CSV line per sample @ 100 Hz:
   ```
   seq,ax,ay,az,gx,gy,gz,dio,cam
   ```
5. **Windows host script** (COM port from Device Manager):
   ```powershell
   pip install pyserial
   python host/serial_bench_reader.py COM5
   ```
   Or: `set SERIAL_PORT=COM5` then `python host/serial_bench_reader.py`

Serial Monitor @ 115200 also works for a quick eyeball test. Boot log should show **`Wi-Fi skipped — USB serial bench mode`**.

## 4-wire ICM20948 + USB to PC

Typical lab setup: **XIAO ESP32-S3 Sense** on USB to the PC, **ICM-20948 on four wires only** (no DIO, SD, camera, or second board).

### Wiring

See **[wiring-diagram.md](wiring-diagram.md)** for Mermaid/ASCII diagrams and the full v1.3.0 pin table.

| ICM-20948 | Connect to XIAO Sense |
|-----------|------------------------|
| VCC | **3V3** (3.3 V — not 5 V) |
| GND | **GND** |
| SDA | **D4** (GPIO **5**, default I2C data) |
| SCL | **D5** (GPIO **6**, default I2C clock) |

The sketch uses `PIN_I2C_SDA 5` and `PIN_I2C_SCL 6`, which match the [Seeed pin map](https://wiki.seeedstudio.com/xiao_esp32s3_pin_multiplexing/) for D4/D5. In Arduino IDE, select board **XIAO_ESP32S3** (not a generic ESP32-S3 dev module).

**I2C address:** `ICM20948_ADDR 0x69` if AD0 is high (often default on breakouts); use **`0x68`** if AD0 is tied to GND.

### Recommended defines (USB-only)

Change from Wi-Fi/TCP config — PC is on USB, not on the AP:

```cpp
#define ENABLE_TCP false
#define ENABLE_SERIAL_BENCH true
#define ENABLE_ESPNOW false
#define ENABLE_SD false
#define SERIAL_OUTPUT_BINARY false   // CSV lines; true = Open Ephys binary on Serial
```

Leave `PIN_I2C_*` at 5/6 unless your wiring differs.

### Arduino IDE steps

1. **Tools → Board → XIAO_ESP32S3**
2. **Tools → USB CDC On Boot → Enabled**
3. **Tools → Port →** your COM port (e.g. COM5 in Device Manager)
4. Upload `arduino/step_node/step_node.ino`

### Serial Monitor — what success looks like

After reset @ **115200 baud**, you should see lines like:

```
STEP node (Arduino) starting
ICM20948: OK
Wi-Fi skipped — USB serial bench mode
ESP-NOW disabled — single-node mode
Serial bench active @115200
Format: CSV seq,ax,ay,az,gx,gy,gz,dio,cam
```

- **`ICM20948: OK at I2C 0x69 WHO_AM_I=0xEA`** — real chip; CSV should show changing ax/ay/az (not fixed sinewave)
- **`ICM20948: synthetic fallback`** — no I2C ACK: check VCC/GND, SDA/SCL on **D4/D5**, address 0x68 vs 0x69, and 3.3 V only.

After **5 s pause**, first data line:

```
# STEP boot complete icm=OK addr=0x69
0,<real ax>,<real ay>,...
```

**DIO (ch6):** `PIN_DIO` defaults to **D0 (GPIO 1)** with internal pull-up. Packed int16: **bit 0 = level** (1 idle/high, 0 pressed to GND); **bits 1–15 = debounced edge count**. If nothing is wired, ch6 sits at 1 (or 0 if floating) — harmless for IMU-only tests.

## Phase 2: Test DIO over USB

No Wi-Fi or TCP required — default sketch uses USB serial bench mode (`ENABLE_TCP false`, `ENABLE_SERIAL_BENCH true`).

### Wiring

| Signal | XIAO pad | Notes |
|--------|----------|-------|
| DIO | **D0** (GPIO 1) | `PIN_DIO` in sketch; change `#define` if wired elsewhere |
| Button | D0 → **GND** | Momentary switch; internal pull-up — press pulls low |

Optional: add a 100 nF cap D0–GND for noisy benches (not required for desk test).

### Boot check

After reset @ 115200, look for:

```
DIO: GPIO1 (pad D0) pull-up — initial level=1 (1=idle, 0=GND)
```

Idle (button open): level **1**. Pressed: level **0**.

### 3-step USB test

1. **Upload** `arduino/step_node/step_node.ino` with USB CDC enabled; open Serial Monitor @ 115200 or run `python host/serial_bench_reader.py COM5` (close Monitor if port busy).
2. **Wire** a momentary button from **D0 to GND**. Wait for CSV after the 5 s boot pause (`# STEP boot complete icm=OK ... dio_ch6=level|edges`).
3. **Press/release** the button — within **~20 ms** (one CSV row @ 100 Hz) **ch6** (8th CSV column) should show:
   - **Level:** odd values (1, 3, 5…) = high; even values (0, 2, 4…) = low — decode with `ch6 & 1`
   - **Edges:** each debounced transition increments the upper bits — `(ch6 >> 1)` counts presses+releases

Example CSV row (button pressed once):

```
12,1234,5678,...,0,0
```

Here `dio=0` → level 0 (pressed). After release, `dio=1` (or higher if edges accumulated).

**Pass:** ch6 level toggles 1↔0 with button; edge count increases on each debounced transition. **Fail:** ch6 stuck — check D0 wiring and that `PIN_DIO` matches your pad.

### Windows — read samples on the PC

```powershell
pip install pyserial
python host/serial_bench_reader.py COM5
```

Replace `COM5` with your port. Close Serial Monitor first if the port is busy.

### Serial Monitor shows only numbers (no boot text)

**Cause:** USB CDC often connects *after* the board has already printed boot lines, or CSV scrolls text away immediately.

**Fix (built into firmware v1.2+):**
1. **Tools → USB CDC On Boot → Enabled**; baud **115200**
2. Press **Reset** on the board *after* opening Serial Monitor (or re-upload)
3. Wait **5 seconds** — CSV is paused so diagnostics stay visible
4. You should see the `BOOT DIAGNOSTICS` banner, I2C scan, and `ICM20948: OK` or `synthetic fallback`

First CSV line is prefixed: `# STEP boot complete icm=OK|FALLBACK`

If you still see sinewave + `az=16384` only → **FALLBACK** (I2C not talking to real chip). Check wiring checklist in [4-wire section](#4-wire-icm20948--usb-to-pc). Optional: install **Adafruit ICM20948** library and swap driver if raw registers fail on your breakout.

## Open Wi-Fi (no password)

For a **lab access point with no password**:

```cpp
#define ENABLE_TCP true
#define ENABLE_SERIAL_BENCH false
#define WIFI_SSID "MyOpenAP"
#define WIFI_PASS ""
```

The sketch calls `WiFi.begin(WIFI_SSID)` when the password string is empty. Then TCP test as usual: `python host/esp32_tcp_client.py` with `ESP32_NODE_HOST` set to the node IP.

**Caveats:** no encryption; anyone on the AP can see traffic — lab/isolated network only. Do not use on production or public Wi-Fi.

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
| `WIFI_SSID` / `WIFI_PASS` | Lab Wi-Fi; **`WIFI_PASS ""`** = open network |
| `PIN_I2C_SDA` / `PIN_I2C_SCL` | ICM-20948 I2C (default D4/D5 → GPIO 5/6) |
| `PIN_DIO` | Digital input (default D0 → GPIO 1) |
| `ICM20948_ADDR` | `0x69` or `0x68` if AD0 grounded |
| **`ENABLE_ESPNOW`** | **`false` = single board (default); `true` = multi-node sync** |
| `NODE_IS_MASTER` | Only when `ENABLE_ESPNOW true` |
| `ENABLE_SD` | `true` on Sense board with SD wired |
| `ENABLE_TCP` | Open Ephys TCP server on port 5000 (`false` for USB-only) |
| `ENABLE_SERIAL_BENCH` | `true` = stream on USB Serial @115200 |
| `SERIAL_OUTPUT_BINARY` | `true` = Open Ephys binary on Serial; `false` = CSV |

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
| 6 | DIO: bit0 level, bits1–15 edge count |
| 7 | Reserved (0 in v1; camera v2) |

Host test:

```bash
pip install numpy
set ESP32_NODE_HOST=<node-ip>
python host/esp32_tcp_client.py
```

**Open Ephys:** Ephys Socket plugin → TCP client → same IP/port and framing.

## 7. Serial bench mode (no Wi-Fi)

See **Test over USB (no Wi-Fi)** above. Legacy summary:

```cpp
#define ENABLE_TCP false
#define ENABLE_SERIAL_BENCH true
```

Use `host/serial_bench_reader.py COMx` on Windows or Serial Monitor @ 115200.

## 8. ESP-NOW multi-node (optional)

Requires **`ENABLE_ESPNOW true`** on every board. Skip this section for single-board v1 tests.

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
| No TCP client | Firewall; ping node IP; open Wi-Fi uses `WIFI_PASS ""` |
| USB bench empty | `ENABLE_TCP false`, `ENABLE_SERIAL_BENCH true`; use `serial_bench_reader.py` |
| ESP-NOW no sync | Same RF channel; Wi-Fi must be started before `esp_now_init` |
| SD fail | Sense CS pin, FAT32 card, `ENABLE_SD` |

---
*v1 guide — camera deferred; see `.planning/ROADMAP.md`*
