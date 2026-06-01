# Pitfalls — ESP32-S3 STEP Acquisition

| Pitfall | Warning signs | Prevention | Phase |
|---------|---------------|------------|-------|
| MIPI camera on S3 | Compile OK but no CSI pins | Use P4 or DVP fallback first | 0 |
| SD blocks streaming | TCP gaps, watchdog | Dedicated sd_task, large buffers | 3 |
| ESP-NOW vs Wi-Fi coexistence | Packet loss | Set Wi-Fi protocol params, limit ESP-NOW payload | 4 |
| Open Ephys header mismatch | Host parse errors | Unit test header/payload sizes vs STEP client | 1 |
| IMU scale wrong | Gait metrics drift | Calibrate against known gravity vector | 1 |
| Camera verify false positives | Alerts on static scenes | ROI mask, baseline motion threshold | 5 |
| PSRAM exhaustion | Camera init fail | Reduce fb_count, JPEG size | 5 |
