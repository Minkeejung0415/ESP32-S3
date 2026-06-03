# Phase 2 Plan

1. Implement Mahony AHRS in firmware
2. `FILTER ON|OFF` in `handleLine`
3. Document channel map in arduino-ide-guide + open-ephys-plugin
4. Plugin: scale ch3-5/ch7 as Q15 when `filterEnabled`

## Verification
- Static: |q|≈1 after settle (host decode with `ESP32_FILTER_ON=1`)
- 90° rotation test: manual HIL
