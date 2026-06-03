---
status: awaiting_human_verify
trigger: "Sample rate in Open Ephys Plugin UI does not affect actual acquisition progression/rate for ESP32 path — still broken after 29adb29/e934948."
created: 2026-06-03T00:00:00Z
updated: 2026-06-03T12:00:00Z
---

## Current Focus

reasoning_checkpoint:
  hypothesis: "Plugin HW rate label defaulted to 1000 and was never synced from detection; startAcquisition clamped ESP32 to 200 Hz while users edited the label thinking it was already 100 Hz. Timestamp progression used 1/boardSampleRate per frame regardless of actual frame interval, so changing only metadata looked like a noop when hardware/label were out of sync."
  confirming_evidence:
    - "sampleRateLabel default was 1000 for all boards; ESP32 startAcquisition uses jlimit(50,200, boardSampleRate) → 200 Hz if user never edited label"
    - "performDetectionHandshake set boardSampleRate from sample_rate= but UI label unchanged"
    - "labelTextChanged used jlimit(1,2000) for setSampleRate but updateSampleFrequency ESP32 uses jlimit(50,200) — mismatch possible"
    - "Prior bridge FREQ pre-START fix is on disk (29adb29) but insufficient alone for perceived progression"
  falsification_test: "After fix: set label 100→50, start USB acquisition; console shows FREQ:50; frame count per wall second ~50; timeline scroll rate halves"
  fix_rationale: "Sync label default to 100 for Red Pitaya; clamp label↔board consistently for ESP32; refresh label after detect; ESP32 timestamps use measured inter-frame dt; firmware resets pacing on FREQ"
  blind_spots: "User may run old bridge process without restart; Wi-Fi direct path not re-tested in this session"

hypothesis: confirmed (compound: label/sync + timestamp model)
next_action: user verifies 100→50 Hz on USB path with rebuilt Plugin and reflashed firmware

## Symptoms

expected: Changing sample rate in Plugin UI changes device loop rate, buffer fill, and timestamp spacing (g_sample_hz == boardSampleRate)
actual: Sample rate change has no effect on acquisition progression
errors: none reported
reproduction: USB path Plugin → serial_tcp_bridge.py --plugin → step_node.ino; change rate 100→50
started: reported 2026-06-03

## Eliminated

- hypothesis: bridge still drops pre-START FREQ on current disk
  evidence: serial_tcp_bridge.py handle_plugin_handshake relays FREQ/CFG/FILTER (commit 29adb29 present)
  timestamp: 2026-06-03

- hypothesis: firmware loop ignores g_sample_hz changes mid-stream
  evidence: loop uses g_sample_hz each iteration; added g_sample_last_us reset on FREQ for immediate pacing
  timestamp: 2026-06-03

## Evidence

- timestamp: 2026-06-03
  checked: device editor.cpp sampleRateLabel default
  found: was "1000" for Red Pitaya; syncRedPitayaBoardSampleRateFromLabel pushed 1000 → startAcquisition clamped to 200 Hz
  implication: UI edits from wrong baseline; user changing 100→50 may not match label text

- timestamp: 2026-06-03
  checked: acqboard.ccp run() ESP32 timestamp increment
  found: elapsedSeconds += 1/boardSampleRate per TCP frame regardless of inter-arrival time
  implication: timeline progression decoupled from actual hardware frame rate

- timestamp: 2026-06-03
  checked: labelTextChanged vs updateSampleFrequency ESP32 clamps
  found: setSampleRate accepted 1–2000; FREQ used 50–200 only in updateSampleFrequency
  implication: boardSampleRate could disagree with serial FREQ

## Resolution

root_cause: Compound failure — (1) HW rate label not aligned with detected boardSampleRate (default 1000 → effective 200 Hz on ESP32 start), (2) Plugin timestamp progression advanced by configured 1/boardSampleRate per frame instead of actual frame interval so hardware rate changes were invisible on the timeline when metadata was stale. Prior bridge-only fix was necessary but not sufficient.
fix: Plugin — Red Pitaya default label 100, ESP32 clamp in sync/labelTextChanged, label sync after retryDetection, multi-line detection handshake, wall-clock inter-frame dt for ESP32 timestamps. Firmware — reset g_sample_last_us on FREQ/CFG SRATE. Bridge — strip duplicate newlines on forward.
verification: code review; py_compile serial_tcp_bridge.py
files_changed:
  - ../Plugin/device editor.cpp
  - ../Plugin/device editor.h
  - ../Plugin/acqboard.ccp
  - arduino/step_node/step_node.ino
  - host/serial_tcp_bridge.py
