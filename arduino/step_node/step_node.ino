/*
 * STEP ESP32-S3 node — Arduino IDE entry point
 * Seeed XIAO ESP32S3: ICM20948 + DIO + SD + Open Ephys TCP (Red Pitaya parity)
 * Optional ESP-NOW multi-node sync (off by default — one board is enough for v1).
 *
 * Guide: docs/arduino-ide-guide.md
 */

// ---------- User config (before includes that depend on flags) ----------
#define ENABLE_ESPNOW false   // true = multi-node sync; v1 bench needs only one board

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

// ---------- User config ----------
#define WIFI_SSID "STEP_ESP32"
#define WIFI_PASS "changeme"

#define TCP_PORT 5000
#define SAMPLE_HZ 100
#define NUM_CHANNELS 8

#define PIN_I2C_SDA 5   // XIAO D4
#define PIN_I2C_SCL 6   // XIAO D5
#define PIN_DIO 1       // XIAO D0 — change as wired
#define ICM20948_ADDR 0x69

#define NODE_IS_MASTER true   // only used when ENABLE_ESPNOW is true
#define ENABLE_SD false
#define ENABLE_TCP true
#define ENABLE_SERIAL_BENCH false  // true = CSV on Serial @115200, skip TCP

// SD (XIAO ESP32S3 Sense expansion — adjust if your wiring differs)
#define PIN_SD_CS 21

// ---------- Open Ephys header (22 bytes LE) ----------
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

uint32_t seq = 0;
int16_t channels[NUM_CHANNELS];
bool icm_ok = false;

typedef struct {
  uint32_t seq;
  int64_t time_us;
} SyncPacket;

#if ENABLE_ESPNOW
void onEspNowRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  (void)info;
  if (len >= (int)sizeof(SyncPacket)) {
    const SyncPacket *p = (const SyncPacket *)data;
    (void)p;
  }
}

void onEspNowSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  (void)info;
  (void)status;
}
#endif

bool icmWrite(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(ICM20948_ADDR);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

bool icmRead(uint8_t reg, uint8_t *val) {
  Wire.beginTransmission(ICM20948_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(ICM20948_ADDR, (uint8_t)1) != 1) return false;
  *val = Wire.read();
  return true;
}

bool initIcm20948() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
  uint8_t who = 0;
  if (!icmRead(0x00, &who)) return false;
  if (who != 0xEA) {
    Serial.printf("ICM20948 WHO_AM_I=0x%02X (expected 0xEA)\n", who);
  }
  icmWrite(0x06, 0x01);
  return true;
}

void readImu(int16_t out[6]) {
  if (!icm_ok) {
    float t = millis() * 0.01f;
    out[0] = (int16_t)(1000 * sinf(t));
    out[1] = (int16_t)(500 * cosf(t));
    out[2] = 16384;
    out[3] = out[4] = out[5] = 0;
    return;
  }
  /* TODO: burst read accel/gyro registers — placeholder zeros until calibrated */
  for (int i = 0; i < 6; i++) out[i] = 0;
}

int16_t readDio() {
  return (int16_t)digitalRead(PIN_DIO);
}

void sendEspNowSync() {
#if ENABLE_ESPNOW
  if (!NODE_IS_MASTER) return;
  SyncPacket pkt = {seq, (int64_t)esp_timer_get_time()};
  uint8_t bcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_send(bcast, (uint8_t *)&pkt, sizeof(pkt));
#endif
}

void packAndSendTcp() {
  if (!client || !client.connected() || !streaming) return;

  OeHeader hdr = {};
  hdr.offset = 0;
  hdr.num_channels = NUM_CHANNELS;
  hdr.samples_per_channel = 1;
  hdr.element_size = 2;
  hdr.bit_depth = 16;
  hdr.num_bytes = NUM_CHANNELS * 1 * 2;

  client.write((uint8_t *)&hdr, sizeof(hdr));
  client.write((uint8_t *)channels, sizeof(channels));
}

void logSd() {
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

void handleLine(const String &line) {
  if (line.startsWith("REDPITAYA")) {
    client.print("8 channels; sample_rate=100; node=esp32s3_arduino\n");
  } else if (line.startsWith("START")) {
    streaming = true;
    Serial.println("TCP streaming START");
  }
}

void setupWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("Connecting to %s", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\nWiFi OK IP=%s\n", WiFi.localIP().toString().c_str());
}

void setupEspNow() {
#if ENABLE_ESPNOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  esp_now_register_recv_cb(onEspNowRecv);
  esp_now_register_send_cb(onEspNowSent);
  esp_now_peer_info_t peer = {};
  memset(&peer.peer_addr, 0xFF, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
  Serial.println("ESP-NOW enabled (multi-node)");
#else
  Serial.println("ESP-NOW disabled — single-node mode");
#endif
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("STEP node (Arduino) starting");

  pinMode(PIN_DIO, INPUT_PULLUP);
  icm_ok = initIcm20948();
  Serial.printf("ICM20948: %s\n", icm_ok ? "OK" : "synthetic fallback");

  setupWifi();
  setupEspNow();

#if ENABLE_SD
  if (SD.begin(PIN_SD_CS)) Serial.println("SD ready");
  else Serial.println("SD init failed");
#endif

#if ENABLE_TCP && !ENABLE_SERIAL_BENCH
  server.begin();
  Serial.printf("TCP listen :%d\n", TCP_PORT);
#endif
}

void loop() {
#if ENABLE_TCP && !ENABLE_SERIAL_BENCH
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
#endif

  static uint32_t last_us = 0;
  uint32_t now = micros();
  if (now - last_us < (1000000UL / SAMPLE_HZ)) return;
  last_us = now;

  readImu(channels);
  channels[6] = readDio();
  channels[7] = 0;  // camera deferred v2

  sendEspNowSync();
  packAndSendTcp();
  logSd();

#if ENABLE_SERIAL_BENCH
  Serial.printf("%lu,%d,%d,%d,%d,%d,%d,%d,%d\n",
                (unsigned long)seq, channels[0], channels[1], channels[2],
                channels[3], channels[4], channels[5], channels[6], channels[7]);
#endif

  seq++;
}
