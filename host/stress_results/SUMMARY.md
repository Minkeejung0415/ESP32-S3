# Stress sweep summary

Port: `COM3` baud 115200

| Hz | mean_hz | dup_seq | gap_seq | pass |
|----|---------|---------|---------|------|
| 50 | 50.6 | 0 | 0 | PASS |
| 100 | 101.4 | 0 | 0 | PASS |
| 150 | 152.1 | 0 | 0 | PASS |
| 200 | 202.8 | 0 | 0 | PASS |
| 250 | 253.5 | 0 | 0 | PASS |
| 300 | 304.2 | 0 | 0 | PASS |
| 400 | 405.8 | 0 | 0 | PASS |
| 500 | 507.0 | 0 | 0 | PASS |
| 750 | 760.2 | 0 | 0 | PASS |
| 1000 | 1013.1 | 0 | 0 | PASS |
| 1500 | 1320.8 | 0 | 0 | FAIL |
| 2000 | 1322.5 | 0 | 5415 | FAIL |

Pass rule: `mean_hz` must be within **95%–115%** of requested Hz (e.g. 1500 Hz @ 1320 Hz → **FAIL**; USB/loop ceiling ~1.3 kHz on this path).

Highest passing Hz: **1000**
Recommended cap (80% of highest pass): **800 Hz**
