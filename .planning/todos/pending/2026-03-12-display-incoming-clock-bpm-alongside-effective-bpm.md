---
created: 2026-03-12T22:05:54.444Z
title: Display incoming clock BPM alongside effective BPM
area: ui
files:
  - src/AnalogLFO.cpp:741-797
---

## Problem

The BPM readout in the waveform display currently shows the **effective BPM** (incoming clock BPM × ratio multiplier). For example, a 120 BPM clock at /1.5 ratio shows "80 BPM". While this has utility, the user also wants to see the **incoming clock BPM** at all times when clocked, so both values are visible simultaneously.

This is beyond the current DISP requirements (DISP-01 through DISP-06) and would need a new requirement and phase or addition to the display overlay system.

## Solution

Add a second BPM readout showing the raw incoming clock BPM (calculated as `60.0 / smoothedPeriod` without the ratio multiplier). Positioning options:
- Show incoming clock BPM in a different corner or adjacent to the effective BPM
- Use a label like "CLK: 120 BPM" vs "EFF: 80 BPM" to distinguish them
- Consider the limited waveform display space (~215×102px) when placing both readouts
