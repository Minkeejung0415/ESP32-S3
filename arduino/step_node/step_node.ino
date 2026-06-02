/*
 * STEP ESP32-S3 node — Arduino IDE entry point
 * Seeed XIAO ESP32S3: ICM20948 + DIO + SD + Open Ephys TCP (Red Pitaya parity)
 *
 * Guide: docs/arduino-ide-guide.md
 *
 * --- Wi-Fi connect timeout fallback ---
 * If STA join fails (30 s), firmware starts Soft AP: SSID STEP_ESP32, pass step1234.
 * On your PC: join Wi-Fi "STEP_ESP32" (password step1234), then Open Ephys / TCP host 192.168.4.1:5000.
 *
 * --- Phone hotspot: use 2.4 GHz band only (ESP32-S3 does not join 5 GHz-only APs). ---
 * Edit WIFI_SSID / WIFI_PASS below (was ubcvisitor open campus — change for your hotspot).
 *
 * --- WIRING_4WIRE_ICM + USB to PC (copy-paste preset) ---
 * #define ENABLE_TCP false
 * #define ENABLE_SERIAL_BENCH true
 * #define ENABLE_ESPNOW false
 * #define ENABLE_SD false
 * #define ICM20948_ADDR 0x69
 * --- end preset ---
 *
 * --- USB_OPEN_EPHYS_MODE (no Wi-Fi — PC cannot join STEP_ESP32) ---
 * Run on PC: python host/serial_tcp_bridge.py COM5
 * Open Ephys Ephys Socket → 127.0.0.1:5000
 * #define ENABLE_TCP false
 * #define ENABLE_SERIAL_BENCH true
 * #define SERIAL_OUTPUT_BINARY true
 * #define ENABLE_ESPNOW false
 * #define ENABLE_SD false
 * --- end USB_OPEN_EPHYS_MODE ---
 */

#define ENABLE_ESPNOW false

#include <WiFi.h>
#include <WiFiClient.h>
#if ENABLE_ESPNOW
#include <esp_now.h>
#endif
#include <esp_timer.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <math.h>
#include <string.h>

#define FIRMWARE_VERSION "1.3.0"
#define BOOT_CSV_DELAY_MS 5000
#define REPEAT_STATUS_SEC 10
#define BOOT_DIAGNOSTICS true

#define DIO_DEBOUNCE_MS 15   // stable toggle within ~20 ms @ 100 Hz

// STA: join your phone/lab hotspot (2.4 GHz). Empty WIFI_PASS = open network (WiFi.begin SSID only).
#define WIFI_SSID "YOUR_HOTSPOT"
#define WIFI_PASS "yourpassword"

// Soft AP fallback after STA timeout (automatic — do not need to edit unless renaming lab AP)
#define WIFI_AP_SSID "STEP_ESP32"
#define WIFI_AP_PASS "step1234"
#define WIFI_STA_TIMEOUT_MS 30000

#define TCP_PORT 5000
#define SAMPLE_HZ 100
#define NUM_CHANNELS 8

#define PIN_I2C_SDA 5   // XIAO D4 / GPIO5
#define PIN_I2C_SCL 6   // XIAO D5 / GPIO6
#define PIN_DIO 1       // XIAO D0 / GPIO1 — change via #define if wired elsewhere
#define ICM20948_ADDR 0x69

#define NODE_IS_MASTER true
#define ENABLE_SD false
#define ENABLE_TCP true
#define ENABLE_SERIAL_BENCH false
#define SERIAL_OUTPUT_BINARY false
#define PIN_SD_CS 21

#define ICM_REG_BANK_SEL 0x7F
#define ICM_WHO_AM_I 0x00
#define ICM_PWR_MGMT_1 0x06
#define ICM_ACCEL_XOUT_H 0x2D
#define ICM20948_WHOAMI_VAL 0xEA

#pragma pack(push, 1)
struct OeHeader {
  int32_t offset;
  int32_t num_bytes;
  uint16_t bit_depth;
  int32_t element_size;
  int32_t num_channels;
  int32_t samples_per_channel;
};
#pragma pack(pop)

WiFiServer server(TCP_PORT);
WiFiClient client;
bool streaming = false;
bool wifi_up = false;
bool wifi_soft_ap = false;

uint32_t seq = 0;
int16_t channels[NUM_CHANNELS];
bool icm_ok = false;
uint8_t icm_addr = ICM20948_ADDR;
uint32_t boot_ms = 0;
bool csv_banner_sent = false;
uint32_t last_status_ms = 0;

// DIO ch6: bit0 = level (1 idle/high, 0 pressed to GND); bits1-15 = debounced edge count
struct {
  bool stable_high;
  bool pending_raw;
  uint32_t pending_since_ms;
  uint16_t edge_count;
} dio_state = {true, true, 0, 0};

static bool useWifi() { return ENABLE_TCP || ENABLE_ESPNOW; }

typedef struct {
  uint32_t seq;
  int64_t time_us;
} SyncPacket;

#if ENABLE_ESPNOW
void onEspNowRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  (void)info; (void)data; (void)len;
}
void onEspNowSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  (void)info; (void)status;
}
#endif

static bool i2cProbe(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

static bool icmWriteAddr(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

static bool icmReadAddr(uint8_t addr, uint8_t reg, uint8_t *val) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(addr, (uint8_t)1) != 1) return false;
  *val = Wire.read();
  return true;
}

static void icmSelectBank(uint8_t addr, uint8_t bank) {
  icmWriteAddr(addr, ICM_REG_BANK_SEL, bank & 0x30);
}

static bool icmReadWhoAmI(uint8_t addr, uint8_t *who) {
  icmSelectBank(addr, 0);
  return icmReadAddr(addr, ICM_WHO_AM_I, who);
}

static bool icmWrite(uint8_t reg, uint8_t val) {
  return icmWriteAddr(icm_addr, reg, val);
}

static bool icmReadReg(uint8_t reg, uint8_t *val) {
  return icmReadAddr(icm_addr, reg, val);
}

static void printBootDiagnostics() {
#if BOOT_DIAGNOSTICS
  Serial.println();
  Serial.println("========================================");
  Serial.println("  STEP ESP32-S3 NODE — BOOT DIAGNOSTICS");
  Serial.println("========================================");
  Serial.printf("Firmware: %s\n", FIRMWARE_VERSION);
  Serial.printf("Board target: XIAO_ESP32S3 (Sense)\n");
  Serial.printf("I2C SDA: GPIO%d (pad D4)  SCL: GPIO%d (pad D5)\n", PIN_I2C_SDA, PIN_I2C_SCL);
  Serial.printf("ICM20948 config addr: 0x%02X (AD0 high=0x69, low=0x68)\n", ICM20948_ADDR);
  Serial.printf("Sample rate: %d Hz  channels: %d\n", SAMPLE_HZ, NUM_CHANNELS);
  Serial.println("--- I2C scan 0x68-0x6B ---");
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
  delay(50);
  int found = 0;
  for (uint8_t a = 0x68; a <= 0x6B; a++) {
    if (i2cProbe(a)) {
      Serial.printf("  device at 0x%02X\n", a);
      found++;
    }
  }
  if (found == 0) Serial.println("  (no devices — check VCC/GND/SDA/SCL on D4/D5)");
  Serial.println("--- ICM20948 WHO_AM_I (expect 0xEA) ---");
  for (uint8_t a : {0x68, 0x69}) {
    uint8_t who = 0;
    if (icmReadWhoAmI(a, &who)) {
      Serial.printf("  0x%02X -> WHO_AM_I 0x%02X %s\n", a, who,
                    who == ICM20948_WHOAMI_VAL ? "OK" : "unexpected");
    } else {
      Serial.printf("  0x%02X -> no ACK\n", a);
    }
  }
  Serial.println("========================================");
#endif
}

static void initDio() {
  pinMode(PIN_DIO, INPUT_PULLUP);
  bool level = digitalRead(PIN_DIO);
  dio_state.stable_high = level;
  dio_state.pending_raw = level;
  dio_state.pending_since_ms = millis();
  Serial.printf("DIO: GPIO%d (pad D0) pull-up — initial level=%d (1=idle, 0=GND)\n",
                PIN_DIO, level ? 1 : 0);
  Serial.println("DIO ch6: bit0=level, bits1-15=edge_count (Open Ephys int16)");
}

static void updateDio() {
  bool raw = digitalRead(PIN_DIO);
  uint32_t now = millis();
  if (raw != dio_state.pending_raw) {
    dio_state.pending_raw = raw;
    dio_state.pending_since_ms = now;
  }
  if ((now - dio_state.pending_since_ms) >= (uint32_t)DIO_DEBOUNCE_MS &&
      dio_state.pending_raw != dio_state.stable_high) {
    dio_state.stable_high = dio_state.pending_raw;
    if (dio_state.edge_count < 0x7FFF) {
      dio_state.edge_count++;
    }
  }
}

static int16_t packDioCh6() {
  uint16_t packed = (dio_state.stable_high ? 1u : 0u) |
                    ((uint32_t)(dio_state.edge_count & 0x7FFFu) << 1);
  return (int16_t)packed;
}

static bool initIcm20948() {
  const uint8_t candidates[] = {ICM20948_ADDR, 0x68, 0x69};
  for (uint8_t a : candidates) {
    uint8_t who = 0;
    if (!icmReadWhoAmI(a, &who)) continue;
    if (who != ICM20948_WHOAMI_VAL) {
      Serial.printf("ICM20948 at 0x%02X WHO_AM_I=0x%02X (expected 0xEA)\n", a, who);
      continue;
    }
    icm_addr = a;
    icmSelectBank(icm_addr, 0);
    icmWriteAddr(icm_addr, ICM_PWR_MGMT_1, 0x01);
    delay(100);
    Serial.printf("ICM20948: OK at I2C 0x%02X WHO_AM_I=0xEA\n", icm_addr);
    return true;
  }
  Serial.println("ICM20948: synthetic fallback — no chip at 0x68/0x69 with WHO_AM_I 0xEA");
  return false;
}

static bool readImuRaw(int16_t out[6]) {
  icmSelectBank(icm_addr, 0);
  Wire.beginTransmission(icm_addr);
  Wire.write(ICM_ACCEL_XOUT_H);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(icm_addr, (uint8_t)14) != 14) return false;

  auto read16be = []() {
    int16_t v = (int16_t)(Wire.read() << 8);
    v |= Wire.read();
    return v;
  };
  out[0] = read16be();
  out[1] = read16be();
  out[2] = read16be();
  (void)read16be();
  out[3] = read16be();
  out[4] = read16be();
  out[5] = read16be();
  return true;
}

static void readImu(int16_t out[6]) {
  if (icm_ok && readImuRaw(out)) return;

  float t = millis() * 0.01f;
  out[0] = (int16_t)(1000 * sinf(t));
  out[1] = (int16_t)(500 * cosf(t));
  out[2] = 16384;
  out[3] = out[4] = out[5] = 0;
}

static void fillOeHeader(OeHeader *hdr) {
  hdr->offset = 0;
  hdr->num_channels = NUM_CHANNELS;
  hdr->samples_per_channel = 1;
  hdr->element_size = 2;
  hdr->bit_depth = 3;  // Open Ephys Ephys Socket: OpenCV S16 enum (not 16 bits)
  hdr->num_bytes = NUM_CHANNELS * 1 * 2;
}

#if ENABLE_ESPNOW
static void sendEspNowSync() {
  if (!NODE_IS_MASTER || !wifi_up) return;
  SyncPacket pkt = {seq, (int64_t)esp_timer_get_time()};
  uint8_t bcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_send(bcast, (uint8_t *)&pkt, sizeof(pkt));
}
#else
static void sendEspNowSync() {}
#endif

static void packAndSendTcp() {
  if (!client || !client.connected() || !streaming) return;
  OeHeader hdr;
  fillOeHeader(&hdr);
  client.write((uint8_t *)&hdr, sizeof(hdr));
  client.write((uint8_t *)channels, sizeof(channels));
}

static void sendSerialBench() {
#if ENABLE_SERIAL_BENCH
  if (!csv_banner_sent) {
    Serial.printf("# STEP boot complete icm=%s addr=0x%02X dio_ch6=level|edges\n",
                  icm_ok ? "OK" : "FALLBACK", icm_addr);
    csv_banner_sent = true;
  }
#if SERIAL_OUTPUT_BINARY
  OeHeader hdr;
  fillOeHeader(&hdr);
  Serial.write((uint8_t *)&hdr, sizeof(hdr));
  Serial.write((uint8_t *)channels, sizeof(channels));
#else
  Serial.printf("%lu,%d,%d,%d,%d,%d,%d,%d,%d\n",
                (unsigned long)seq, channels[0], channels[1], channels[2],
                channels[3], channels[4], channels[5], channels[6], channels[7]);
#endif
#endif
}

static void logSd() {
#if ENABLE_SD
  if (!SD.begin(PIN_SD_CS)) return;
  File f = SD.open("/step_session.bin", FILE_APPEND);
  if (f) {
    f.write((uint8_t *)&seq, sizeof(seq));
    f.write((uint8_t *)channels, sizeof(channels));
    f.close();
  }
#endif
}

static void handleLine(const String &line) {
  if (line.startsWith("REDPITAYA")) {
    client.print("8 channels; sample_rate=100; node=esp32s3_arduino\n");
  } else if (line.startsWith("START")) {
    streaming = true;
    Serial.println("TCP streaming START");
  }
}

static const char *wifiStatusString(wl_status_t status) {
  switch (status) {
    case WL_IDLE_STATUS: return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL: return "WL_NO_SSID_AVAIL (SSID not found / wrong name / 5 GHz only?)";
    case WL_SCAN_COMPLETED: return "WL_SCAN_COMPLETED";
    case WL_CONNECTED: return "WL_CONNECTED";
    case WL_CONNECT_FAILED: return "WL_CONNECT_FAILED (wrong password?)";
    case WL_CONNECTION_LOST: return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED: return "WL_DISCONNECTED";
    default: return "unknown";
  }
}

static void printWifiFailureHelp(wl_status_t status) {
  Serial.printf("Wi-Fi status=%d (%s)\n", (int)status, wifiStatusString(status));
  Serial.println("STA tips: 2.4 GHz hotspot band; correct SSID/password; PC and ESP32 same network;");
  Serial.println("  iPhone: Settings -> Personal Hotspot -> Maximize Compatibility ON");
}

static bool startSoftApFallback() {
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
  if (!ok) {
    Serial.println("Soft AP start failed");
    return false;
  }
  wifi_up = true;
  wifi_soft_ap = true;
  IPAddress apIp = WiFi.softAPIP();
  Serial.printf("WiFi OK AP IP=%s  SSID=%s  pass=%s\n",
                apIp.toString().c_str(), WIFI_AP_SSID, WIFI_AP_PASS);
  Serial.println("PC: join Wi-Fi STEP_ESP32, then TCP/Open Ephys host 192.168.4.1 port 5000");
  return true;
}

static void setupWifi() {
  if (!useWifi()) {
    Serial.println("Wi-Fi skipped — USB serial bench mode");
    return;
  }

  wifi_soft_ap = false;
  WiFi.mode(WIFI_STA);
  if (strlen(WIFI_PASS) == 0) {
    Serial.printf("Connecting to open network %s (2.4 GHz)\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID);
  } else {
    Serial.printf("Connecting to %s (2.4 GHz)\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  }

  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - t0 > (uint32_t)WIFI_STA_TIMEOUT_MS) {
      Serial.println();
      wl_status_t st = WiFi.status();
      printWifiFailureHelp(st);
      Serial.printf("STA failed (status=%d) — starting Soft AP %s\n", (int)st, WIFI_AP_SSID);
      startSoftApFallback();
      return;
    }
  }

  wifi_up = true;
  wifi_soft_ap = false;
  Serial.printf("\nWiFi OK IP=%s  RSSI=%d dBm\n",
                WiFi.localIP().toString().c_str(), WiFi.RSSI());
}

static void setupEspNow() {
#if ENABLE_ESPNOW
  if (!wifi_up) {
    Serial.println("ESP-NOW skipped (Wi-Fi not connected)");
    return;
  }
  esp_now_init();
  esp_now_register_recv_cb(onEspNowRecv);
  esp_now_register_send_cb(onEspNowSent);
  esp_now_peer_info_t peer = {};
  memset(&peer.peer_addr, 0xFF, 6);
  peer.encrypt = false;
  esp_now_add_peer(&peer);
  Serial.println("ESP-NOW enabled (multi-node)");
#else
  Serial.println("ESP-NOW disabled — single-node mode");
#endif
}

static void maybeRepeatFallbackStatus() {
#if REPEAT_STATUS_SEC > 0
  if (icm_ok) return;
  if (millis() - last_status_ms < (uint32_t)REPEAT_STATUS_SEC * 1000UL) return;
  last_status_ms = millis();
  Serial.println("ICM20948: synthetic fallback — check 3V3, GND, SDA->D4, SCL->D5, addr 0x68/0x69");
#endif
}

void setup() {
  Serial.begin(115200);
  delay(3000);
  while (!Serial && millis() < 5000) {
    delay(10);
  }

  Serial.println();
  Serial.println("STEP node (Arduino) starting");

  initDio();

  printBootDiagnostics();
  icm_ok = initIcm20948();

  setupWifi();
  setupEspNow();

#if ENABLE_SD
  Serial.println(SD.begin(PIN_SD_CS) ? "SD ready" : "SD init failed");
#endif

#if ENABLE_TCP && !ENABLE_SERIAL_BENCH
  if (wifi_up) {
    server.begin();
    Serial.printf("TCP listen :%d\n", TCP_PORT);
  }
#elif ENABLE_SERIAL_BENCH
  Serial.println("Serial bench active @115200");
  Serial.println(SERIAL_OUTPUT_BINARY
                     ? "Format: Open Ephys binary on Serial"
                     : "Format: CSV seq,ax,ay,az,gx,gy,gz,dio,cam");
#endif

  boot_ms = millis();
  Serial.printf("CSV/stream paused %d ms — read diagnostics above\n", BOOT_CSV_DELAY_MS);
  last_status_ms = millis();
}

void loop() {
#if ENABLE_TCP && !ENABLE_SERIAL_BENCH
  if (wifi_up) {
    if (!client || !client.connected()) {
      client = server.available();
      if (client) {
        streaming = false;
        Serial.println("Client connected");
      }
    }
    while (client && client.available()) {
      String line = client.readStringUntil('\n');
      line.trim();
      if (line.length()) handleLine(line);
    }
  }
#endif

  maybeRepeatFallbackStatus();

  if (millis() - boot_ms < (uint32_t)BOOT_CSV_DELAY_MS) {
    return;
  }

  static uint32_t last_us = 0;
  uint32_t now = micros();
  if (now - last_us < (1000000UL / SAMPLE_HZ)) return;
  last_us = now;

  readImu(channels);
  updateDio();
  channels[6] = packDioCh6();
  channels[7] = 0;

  sendEspNowSync();
  packAndSendTcp();
  sendSerialBench();
  logSd();

  seq++;
}
