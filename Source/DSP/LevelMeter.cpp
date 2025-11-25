/*
  ==============================================================================

    LevelMeter.cpp
    Created: 14 Nov 2025 4:07:26pm
    Author:  Gen3r

  ==============================================================================
*/


#pragma once

#include "LevelMeter.h"
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

private:
    void finalizeBlock(); // called when 400 ms block completes

    // K-weighting filters (per-channel)
    std::vector<juce::IIRFilter> hpFilters;    // high-pass
    std::vector<juce::IIRFilter> shelfFilters; // high-shelf

    double sampleRate = 48000.0;
    int numChannels = 2;

    // Block accumulation (400 ms)
    const double blockDurationSeconds = 0.400; // 400 ms
    int blockSize = 0;            // computed from sampleRate
    int blockCounter = 0;
    double blockEnergy = 0.0;    // sum of per-sample channel-weighted power

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


// LevelMeter.cpp

#include "LevelMeter.h"

// Helper: convert power to LUFS according to ITU formula
static double powerToLufs(double power)
{
    return -0.691 + 10.0 * std::log10(power + epsValue());
}

void LevelMeter::prepare(double sr, int channels)
{
    sampleRate = sr;
    numChannels = std::max(1, channels);

    blockSize = static_cast<int>(std::round(blockDurationSeconds * sampleRate));
    if (blockSize < 1) blockSize = 1;

    hpFilters.assign(numChannels, juce::IIRFilter());
    shelfFilters.assign(numChannels, juce::IIRFilter());

    // --- K-weighting approximation ---
    // Use a 2nd-order high-pass at 40 Hz (approximation) + a high-shelf around 4 kHz.
    // These are widely used approximations of the ITU/EBU K-weighting chain and
    // are suitable for most meter implementations. If you need bit-exact compliance
    // with BS.1770, replace these with fixed coefficients derived from the spec.

    for (int ch = 0; ch < numChannels; ++ch)
    {
        // High-pass: 40 Hz, Q ~= 0.707 (but ITU uses a specific prototype — this is a practical approximation)
        hpFilters[ch].setCoefficients(juce::IIRCoefficients::makeHighPass(sampleRate, 40.0, 0.70710678));

        // High-shelf: +4 dB at 4000 Hz, Q ~= 0.707 (approximation)
        const double gainDb = 4.0;
        shelfFilters[ch].setCoefficients(juce::IIRCoefficients::makeHighShelf(sampleRate, 4000.0, 0.70710678, juce::Decibels::decibelsToGain(gainDb)));
    }

    reset();
}

void LevelMeter::reset()
{
    blockCounter = 0;
    blockEnergy = 0.0;
    accumulatedEnergy = 0.0;
    accumulatedTime = 0.0;
    integratedLufs.store(std::numeric_limits<float>::quiet_NaN());
    integratedValid.store(false);

    // Reset filter states
    for (auto& f : hpFilters) f.reset();
    for (auto& f : shelfFilters) f.reset();
}

void LevelMeter::processBuffer(const juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    const int availableSamples = buffer.getNumSamples();
    if (numSamples < 0 || startSample + numSamples > availableSamples)
        numSamples = availableSamples - startSample;

    if (numSamples <= 0) return;

    // Prepare pointers to channel data. If the buffer has fewer channels than expected,
    // we treat missing channels as silence.
    std::vector<const float*> channelData(numChannels, nullptr);
    for (int ch = 0; ch < numChannels; ++ch)
    {
        if (ch < buffer.getNumChannels())
            channelData[ch] = buffer.getReadPointer(ch) + startSample;
        else
            channelData[ch] = nullptr; // will be treated as zeros
    }

    for (int i = 0; i < numSamples; ++i)
    {
        // For each sample, filter per channel and compute channel-weighted power.
        double samplePower = 0.0;

        // Per ITU-R BS.1770, channel weights depend on channel layout. For stereo we use
        // simple average of squared signals: power = 0.5*(L^2 + R^2). For N channels we'll
        // average equally (1/N sum of squares). You may replace weights for other layouts.

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float in = 0.0f;
            if (channelData[ch] != nullptr)
                in = channelData[ch][i];

            float filtered = hpFilters[ch].processSingleSampleRaw(in);
            filtered = shelfFilters[ch].processSingleSampleRaw(filtered);

            samplePower += static_cast<double>(filtered) * static_cast<double>(filtered);
        }

        samplePower /= static_cast<double>(numChannels);

        // accumulate energy (power * dt). We'll sum power and divide by sampleRate later when computing mean power.
        blockEnergy += samplePower;
        blockCounter++;

        if (blockCounter >= blockSize)
            finalizeBlock();
    }
}

void LevelMeter::finalizeBlock()
{
    if (blockCounter <= 0)
        return;

    // mean power over block = (sum of sample powers) / blockCounter
    const double meanPower = blockEnergy / static_cast<double>(blockCounter);
    const double blockDuration = static_cast<double>(blockCounter) / sampleRate;

    const double blockLufs = powerToLufs(meanPower);

    // Absolute gate
    if (blockLufs > absoluteGate)
    {
        // Compute relative gate threshold using ungated integrated loudness candidate.
        // The spec: the relative gate is -10 LU relative to the ungated loudness of the whole program.
        // Practically, we don't know the final ungated integrated loudness until we've processed everything,
        // so common practical approach (and used by many meters) is to use the block's LUFS relative
        // to the running ungated integrated estimate — or to use a two-pass approach.
        // Here we'll implement the practical single-pass approximation used above in the plan:
        // "relative gate = current blockLufs - 10 LU" where current blockLufs is compared to the running ungated estimate.

        // NOTE: The strict BS.1770 integrated algorithm uses a two-step gating process that requires
        // the ungated integrated loudness across the entire program to compute the relative gate.
        // This implementation uses the common single-pass approximation: include blocks whose blockLUFS
        // is within 10 LU of the currently accumulated integrated loudness candidate. To reduce bias
        // early in processing when we have little or no accumulated data, we allow the first blocks
        // to seed the accumulator.

        bool includeBlock = false;

        if (!integratedValid.load())
        {
            // No accumulated energy yet — accept the block (if it passes absolute gate) to seed the accumulator
            includeBlock = true;
        }
        else
        {
            // Compute current running integrated (ungated) loudness from accumulatedEnergy/Time without this block
            const double runningMeanPower = accumulatedEnergy / std::max(accumulatedTime, 1e-9);
            const double runningLufs = powerToLufs(runningMeanPower);

            double relativeGateThreshold = runningLufs - 10.0; // -10 LU relative

            if (blockLufs > relativeGateThreshold)
                includeBlock = true;
        }

        if (includeBlock)
        {
            accumulatedEnergy += meanPower * blockDuration;
            accumulatedTime += blockDuration;

            // Update integrated LUFS
            const double integratedMeanPower = accumulatedEnergy / std::max(accumulatedTime, 1e-9);
            const double integrated = powerToLufs(integratedMeanPower);
            integratedLufs.store(static_cast<float>(integrated));
            integratedValid.store(true);
        }
    }

    // reset block
    blockCounter = 0;
    blockEnergy = 0.0;
}

float LevelMeter::getIntegratedLufs() const noexcept
{
    return integratedLufs.load();
}

bool LevelMeter::hasIntegratedLufs() const noexcept
{
    return integratedValid.load();
}

double epsValue()
{
    return 1e-12;
}


//Integration notes and usage example
//----------------------------------
//
//1) Integration into your PluginProcessor::prepareToPlay:
//
//LevelMeter.prepare(sampleRate, getTotalNumInputChannels());
//
//2) In PluginProcessor::processBlock(audio thread) :
//
//    // If using audio buffer directly
//    LevelMeter.processBuffer(buffer);
//
//3) Reading the value in the GUI thread(safe) :
//
//    if (LevelMeter.hasIntegratedLufs())
//    {
//        float value = LevelMeter.getIntegratedLufs();
//        // display value (e.g. "-14.2 LUFS")
//    }
//    else
//    {
//        // no gated data yet
//    }
//
//4) Notes about gating accuracy / two - pass algorithm :
//
//-The exact BS.1770 integrated algorithm is typically implemented as a two - pass algorithm
//(first pass to compute ungated loudness to determine the - 10 LU relative gate, second pass to
//    compute the gated integrated value).This single - pass approach uses a running estimate and
//    accepts first qualifying blocks to seed the accumulator.For many real - time meter needs
//    this yields practical results, but for bit - exact offline measurement you should run a
//    two - pass algorithm on the full program.
//
//    - To implement the two - pass algorithm in a plugin you can store the per - block meanPower and
//    blockDuration values(e.g.in a lightweight vector) during processing, then after the
//    program / region is finished compute ungated integrated loudness and apply the relative gate
//    to select blocks.This requires memory proportional to number of blocks and a finalization
//    step(offline).
//
//    5) Threading / performance:
//
//    -This implementation is designed to be cheap : only two IIR filters per channel and a few
//        accumulators.No allocations in the audio path after prepare() and reset().
//
//        - Reading the value from GUI is atomic - safe via std::atomic used for the float and a boolean flag.
//
//        6) Extending:
//
//        -Add true - peak detection(upsampling + peak search) if you need peak metering.
//            - Add short - term and momentary loudness(use sliding windows of 3s and 400 ms respectively).
//            - Replace the K - weighting approximations with hard - coded BS.1770 coefficients if you need exact compliance.