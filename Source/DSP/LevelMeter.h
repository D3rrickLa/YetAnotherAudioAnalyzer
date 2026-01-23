/*
  ==============================================================================

    LevelMeter.h
    Created: 14 Nov 2025 4:07:26pm
    Author:  Gen3r

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <atomic>

// Simple approximate LUFS meter: K-weighting (HP + shelf), block-based mean-power,
// absolute gate, single-pass relative gating approximation, exposes integrated LUFS
// and last-block LUFS for immediate GUI display.
//
// This implementation is intentionally pragmatic and single-pass (not the strict two-pass BS.1770).
class LevelMeter
{
public:
    LevelMeter();
    ~LevelMeter() = default;

    void prepare(double sampleRate, int channels);
    void reset();

    // Process a buffer range (audio thread)
    void processBuffer(const juce::AudioBuffer<float>& buffer, int startSample, int numSamples);

    float getIntegratedLufs() const noexcept;
    bool hasIntegratedLufs() const noexcept;

    // immediate fallback so GUI shows something quickly
    float getLastBlockLufs() const noexcept;

    std::atomic<float> lastBlockRmsL{ 0.0f };
    std::atomic<float> lastBlockRmsR{ 0.0f };

private:
    void finalizeBlock();

    // converts power to LUFS (small epsilon to avoid log(0))
    static double powerToLufs(double power);

    double sampleRate = 44100.0;
    int numChannels = 2;

    // block accumulation
    int blockSize = 1024; // in samples
    int blockCounter = 0;
    double blockEnergy = 0.0;

    // accumulated gated energy/time (for integrated)
    double accumulatedEnergy = 0.0;
    double accumulatedTime = 0.0;

    // filters for K-weighting per channel
    std::vector<juce::IIRFilter> hpFilters;
    std::vector<juce::IIRFilter> shelfFilters;

    // gating params (tunable)
    double absoluteGate = -70.0; // LUFS
    // atomic shared outputs
    std::atomic<float> integratedLufs;
    std::atomic<bool> integratedValid;
    std::atomic<float> lastBlockLufs;

};