🩵 Phase 1 — MVP (Minimum Viable Product)

Goal: Core functionality — must be stable, efficient, and visually clear.

✅ Core Features

Spectrum Analyzer

Real-time FFT (JUCE dsp::FFT)

Adjustable resolution and smoothing

Log frequency scaling

Averaging modes: instantaneous / short-term / long-term

Stereo Correlation Meter

Real-time correlation (−1 → +1)

Visual feedback: bar or circular indicator

Optional numeric readout

Stereo Width Visualizer

L/R balance and width vector display

Optional “phase dot” cloud visualization (Ozone Imager style)

LUFS Meter

Integrated loudness module using ITU-R BS.1770-4

Real-time short-term (LUFS-S), momentary, and integrated loudness

K-weighted RMS algorithm

Simple horizontal bar or numerical display

Clip / over indicator (if >0 LUFS)

Optional “target zone” overlay (e.g., −14 LUFS streaming standard)

Mono / Stereo A/B Toggle

One-click mono fold-down

Optional timed A/B loop test (stereo ↔ mono every X seconds)

Works pre-meter (affects visual + audio)

Freeze / Snapshot

Hold spectrum, correlation, and LUFS readings

Compare against current playback visually

Lightweight UI

Modular panels: Spectrum / Correlation / LUFS

Resizable layout

Efficient OpenGL rendering

Flat minimal aesthetic (like SPAN + Ozone hybrid)
