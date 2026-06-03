---
status: awaiting_human_verify
trigger: "Sample rate in Open Ephys Plugin UI does not affect actual acquisition progression/rate for ESP32 path."
created: 2026-06-03T00:00:00Z
updated: 2026-06-03T00:00:00Z
---

## Current Focus

reasoning_checkpoint:
  hypothesis: "USB bridge handle_plugin_handshake drops FREQ: between REDPITAYA and START, so firmware g_sample_hz stays at default 100 Hz while Plugin may show a different boardSampleRate"
  confirming_evidence:
    - "serial_tcp_bridge.py:348 logged non-START lines as ignored; Plugin sends FREQ before START (acqboard.ccp:670-677)"
    - "send_plugin_handshake_replies hardcoded sample_rate=100"
    - "DeviceEditor synced label after startAcquisition, so label could be ignored for initial FREQ targetHz"
  falsification_test: "If forwarding FREQ pre-START does not change measured inter-sample interval on device, hypothesis is wrong"
  fix_rationale: "Forward FREQ/CFG/FILTER pre-START; track rate for handshake; sync label before start; re-send FREQ after START when relay active"
  blind_spots: "Not hardware-verified in this session; Wi-Fi direct path was already OK"

hypothesis: confirmed — bridge dropped pre-START FREQ
next_action: user verifies 100→50 Hz on USB path

## Symptoms

expected: Changing sample rate in Plugin UI changes device loop rate, buffer fill, and timestamp spacing (g_sample_hz == boardSampleRate)
actual: Sample rate change has no effect on acquisition progression
errors: none reported
reproduction: USB path Plugin → serial_tcp_bridge.py --plugin → step_node.ino; change rate 100→50
started: reported 2026-06-03

## Eliminated

## Evidence

- timestamp: 2026-06-03
  checked: Plugin acqboard.ccp startAcquisition ESP32 path lines 670-677
  found: Plugin sends FREQ: before START on each acquisition
  implication: rate command exists on Plugin side

- timestamp: 2026-06-03
  checked: host/serial_tcp_bridge.py handle_plugin_handshake lines 329-348
  found: loop only handles START; other lines logged as "ignored command while waiting for START"
  implication: FREQ never forwarded to USB serial before stream starts

- timestamp: 2026-06-03
  checked: send_plugin_handshake_replies line 289
  found: hardcoded sample_rate=100 in TCP handshake
  implication: detection always reports 100 Hz regardless of prior FREQ

- timestamp: 2026-06-03
  checked: device editor.cpp startAcquisition lines 929-934
  found: syncRedPitayaBoardSampleRateFromLabel() runs AFTER memoryUsage->startAcquisition()
  implication: label value may not be applied to settings.boardSampleRate before FREQ in startAcquisition

- timestamp: 2026-06-03
  checked: step_node.ino loop line 922, handleLine FREQ branch 551-566
  found: firmware honors g_sample_hz when FREQ received on Serial
  implication: firmware not root cause if FREQ never arrives

## Resolution

root_cause: serial_tcp_bridge.py handle_plugin_handshake ignored FREQ:/CFG/FILTER between REDPITAYA and START (only START was forwarded to USB serial). Handshake always advertised sample_rate=100. DeviceEditor applied label sample rate after acquisition had already started.
fix: Bridge forwards relayable commands pre-START and tracks _plugin_sample_rate_hz for handshake; DeviceEditor syncs label before startAcquisition; Plugin re-sends FREQ after START (relay active).
verification: py_compile serial_tcp_bridge.py OK; code review of command path
files_changed:
  - host/serial_tcp_bridge.py
  - ../Plugin/device editor.cpp
  - ../Plugin/acqboard.ccp
