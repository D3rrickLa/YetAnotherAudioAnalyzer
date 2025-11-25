/*
  ==============================================================================

    LevelMeter.h
    Created: 14 Nov 2025 4:07:26pm
    Author:  Gen3r

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
// Lightweight, thread-safe LUFS (Integrated) meter implementation for JUCE.
// - Uses 400 ms analysis blocks
// - Applies a K-weighting approximation (HP + high-shelf) using JUCE IIR filters
// - Implements absolute (-70 LUFS) and relative (-10 LU) gates per EBU R128 / ITU-R BS.1770
// - Accumulates energy only from blocks that pass gating
// - Exposes public API suitable for calling from audio thread and reading from GUI thread
class LevelMeter
{
public:
    LevelMeter() = default;
    ~LevelMeter() = default;
        
    void test();

    // Call once before processing (audio thread) with sample rate and channels.
    void prepare(double sampleRate, int numChannels = 2);


    // Reset internal state (call on start/stop/seek etc.)
    void reset();


    // Process a buffer of interleaved or deinterleaved audio. For convenience, we accept
    // a juce::AudioBuffer<float>& and will assume up to 'numChannels' channels are present.
    void processBuffer(const juce::AudioBuffer<float>& buffer, int startSample = 0, int numSamples = -1);


    // Return the current integrated LUFS value. This is atomic-safe for GUI thread usage.
    // If no gated energy has been accumulated yet, returns NaN.
    float getIntegratedLufs() const noexcept;


    // Return whether integrated value is valid (i.e. we've accumulated gated audio)
    bool hasIntegratedLufs() const noexcept;

    double epsValue();

private:
    void finalizeBlock(); // called when 400 ms block completes


    // K-weighting filters (per-channel)
    std::vector<juce::IIRFilter> hpFilters; // high-pass
    std::vector<juce::IIRFilter> shelfFilters; // high-shelf


    double sampleRate = 48000.0;
    int numChannels = 2;


    // Block accumulation (400 ms)
    const double blockDurationSeconds = 0.400; // 400 ms
    int blockSize = 0; // computed from sampleRate
    int blockCounter = 0;
    double blockEnergy = 0.0; // sum of per-sample channel-weighted power


    // Gated integrated accumulation
    double accumulatedEnergy = 0.0;
    double accumulatedTime = 0.0;


    // Last computed integrated LUFS (atomic-safe for reads)
    std::atomic<float> integratedLufs{ std::numeric_limits<float>::quiet_NaN() };
    std::atomic<bool> integratedValid{ false };


    // Constants
    const double absoluteGate = -70.0; // LUFS
    // Note: relative gate computed per-block as ungatedBlockLoudness - 10 LU


    // Small epsilon to avoid log of zero
    static constexpr double EPS = 1e-12;

};
