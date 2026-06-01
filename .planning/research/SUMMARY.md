# Research Summary

**Stack:** ESP-IDF on Seeed XIAO ESP32-S3; ICM-20948 I2C; ESP-NOW sync; Wi-Fi TCP Open Ephys; FATFS SD.

**Camera:** TEVM-AR0234 (MIPI, global shutter, S32 lens variant in lab) is **not** plug-compatible with XIAO ESP32-S3 DVP. Use OV3660/OV5640 for S3 bring-up; plan ESP32-P4 or external gateway for TEVM.

**Table stakes:** 100 Hz IMU, TCP port 5000, Red Pitaya handshake, 22-byte Open Ephys header.

**Watch out for:** SD/TCP contention, ESP-NOW + Wi-Fi coexistence, extended channel map breaking STEP client without env flag.
