# Roadmap
## Phase 1 - MVP (Minimum Viable Product)

Goal: Core functionality — must be stable, efficient, and visually clear.

✅ Core Features

1. Spectrum Analyzer

  - Real-time FFT (JUCE dsp::FFT)
  
  - Adjustable resolution and smoothing
  
  - Log frequency scaling

  - Averaging modes: instantaneous / short-term / long-term

2. Stereo Correlation Meter

  - Real-time correlation (−1 → +1)
  
  - Visual feedback: bar or circular indicator
  
  - Optional numeric readout

3. Stereo Width Visualizer

  - L/R balance and width vector display
  
  - Optional “phase dot” cloud visualization (Ozone Imager style)

4. LUFS Meter

  - Integrated loudness module using ITU-R BS.1770-4
  
  - Real-time short-term (LUFS-S), momentary, and integrated loudness
  
  - K-weighted RMS algorithm
  
  - Simple horizontal bar or numerical display
  
  - Clip / over indicator (if >0 LUFS)
  
  - Optional “target zone” overlay (e.g., −14 LUFS streaming standard)

5. Mono / Stereo A/B Toggle

  - One-click mono fold-down
  
  - Optional timed A/B loop test (stereo ↔ mono every X seconds)
  
  - Works pre-meter (affects visual + audio)
  
6. Freeze / Snapshot

  - Hold spectrum, correlation, and LUFS readings
  
  - Compare against current playback visually
  
7. Lightweight UI

  - Modular panels: Spectrum / Correlation / LUFS
  
  - Resizable layout
  
  - Efficient OpenGL rendering
  
  - Flat minimal aesthetic (like SPAN + Ozone hybrid)


## Phase 2 - Advanced Metering & Personalization
1. Multi-Band Correlation
2. Reference Track Overlay
3. RMS + Peak overlay on Spectrum
4. Hotkeys (M, Space, Tab)
5. Custom Color Themes / Layouts
6. Session Presets (Save Layouts)
7. Refined Loudness Target Panel
  - User-selectable presets: Spotify (−14), YouTube (−13), Apple (−16)
  - Overshoot highlighting (color flash when exceeding target)

## Phase 3 - Professional / Expansion Features
1. Mid/Side Spectrum & Correlation
2. External Side-chain / Reference Compare
3. A/B Snapshot Compare (full plugin state)
4. Compact “HUD” Mode
5. Standalone Version
6. DAW-linked Transport Integration
7. Batch Loudness Analysis (offline mode)

---
| Milestone | Deliverable                        | Target |
| --------- | ---------------------------------- | ------ |
| M1        | JUCE setup + FFT base              | Week 1 |
| M2        | Correlation + Stereo panel         | Week 2 |
| M3        | LUFS Meter (Momentary, Short-Term) | Week 3 |
| M4        | Mono A/B + Freeze                  | Week 4 |
| M5        | UI polish + Beta build             | Week 5 |
| M6        | Public Beta (MVP Release)          | Week 6 |
| M7        | Feedback + Phase 2 design          | Week 8 |

-------- Update 25-12-19
Finished the spectrum analyzer (rough)
Finished the correlation meter
Finished the Lufs meter (rough, like jsut a number)
Have a 'stereo width' analyzer? not really what I want. Needs to be like the Ozone UI, right now it's just a number

NEXT:
Mono / Stereo Toggle
A and B Toggle? Honeslty, now thinking about it, don't know why I would have this... but will just add it
Proper stereo width (the Ozone kind)
A UI that is dog water
performance fixes (speed)
Spectrum analyzer 'leveling' really weird looking with lots of info in the lows


UPDATE 23 JAN 26
- added GUI of the correlation and width, the width kind of wack as in it 'sends' outside the parameter box
- the levels for the spectrum analyzer are NOT accureate because when setting the values from -90 to 0, ti says things are hitting at -60 which i know they aren't
