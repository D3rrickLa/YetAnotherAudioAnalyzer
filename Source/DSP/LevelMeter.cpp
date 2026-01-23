/*
  ==============================================================================

    LevelMeter.cpp
    Created: 14 Nov 2025 4:07:26pm
    Author:  Gen3r

  ==============================================================================
*/



#include "LevelMeter.h"
#include <cmath>
#include <atomic>

LevelMeter::LevelMeter()
{
    integratedLufs.store(std::numeric_limits<float>::quiet_NaN());
    integratedValid.store(false);
    lastBlockLufs.store(std::numeric_limits<float>::quiet_NaN());
}

double LevelMeter::powerToLufs(double power)
{
    // small epsilon to avoid -inf
    return -0.691 + 10.0 * std::log10(power + 1e-12);
}

void LevelMeter::prepare(double sr, int channels)
{
    sampleRate = sr > 0.0 ? sr : 44100.0;
    numChannels = std::max(1, channels);

    blockSize = static_cast<int>(std::round(0.400 * sampleRate)); // 400 ms blocks (example)
    if (blockSize < 1) blockSize = 1;

    hpFilters.clear();
    shelfFilters.clear();
    hpFilters.resize(numChannels);
    shelfFilters.resize(numChannels);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        hpFilters[ch].setCoefficients(
            juce::IIRCoefficients::makeHighPass(sampleRate, 40.0, 0.70710678));

        const double gainDb = 4.0;
        shelfFilters[ch].setCoefficients(
            juce::IIRCoefficients::makeHighShelf(sampleRate, 4000.0, 0.70710678, juce::Decibels::decibelsToGain(gainDb)));
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
    lastBlockLufs.store(std::numeric_limits<float>::quiet_NaN());

    for (auto& f : hpFilters) f.reset();
    for (auto& f : shelfFilters) f.reset();
}

void LevelMeter::processBuffer(const juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    const int available = buffer.getNumSamples();
    if (startSample < 0) startSample = 0;
    if (numSamples < 0 || startSample + numSamples > available)
        numSamples = available - startSample;
    if (numSamples <= 0) return;

    std::vector<const float*> chPtrs(numChannels, nullptr);
    for (int ch = 0; ch < numChannels; ++ch)
        chPtrs[ch] = (ch < buffer.getNumChannels()) ? buffer.getReadPointer(ch) + startSample : nullptr;

    std::vector<double> rmsSums(numChannels, 0.0);

    for (int i = 0; i < numSamples; ++i)
    {
        double samplePower = 0.0;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float in = chPtrs[ch] ? chPtrs[ch][i] : 0.0f;

            float filtered = hpFilters[ch].processSingleSampleRaw(in);
            filtered = shelfFilters[ch].processSingleSampleRaw(filtered);

            const double p = filtered * filtered;
            samplePower += p;
            rmsSums[ch] += p;
        }

        samplePower /= static_cast<double>(numChannels);
        blockEnergy += samplePower;
        blockCounter++;

        if (blockCounter >= blockSize)
            finalizeBlock();
    }

    // Update per-channel RMS and scale for visibility
    constexpr float displayScale = 10.0f; // increase visual response
    for (int ch = 0; ch < numChannels; ++ch)
    {
        float rms = static_cast<float>(std::sqrt(rmsSums[ch] / numSamples));
        rms *= displayScale;                // scale RMS for visual feedback
        rms = juce::jlimit(0.0f, 1.0f, rms); // clamp 0–1

        if (ch == 0)
            lastBlockRmsL.store(rms);
        else if (ch == 1)
            lastBlockRmsR.store(rms);
    }
}


void LevelMeter::finalizeBlock()
{
    if (blockCounter <= 0) return;

    const double meanPower = blockEnergy / static_cast<double>(blockCounter);
    const double blockDuration = static_cast<double>(blockCounter) / sampleRate;
    const double blockLufs = powerToLufs(meanPower);

    // Always store last block value for immediate GUI feedback
    lastBlockLufs.store(static_cast<float>(blockLufs), std::memory_order_relaxed);

    // Absolute gate
    if (blockLufs > absoluteGate)
    {
        bool includeBlock = false;

        if (!integratedValid.load(std::memory_order_acquire))
        {
            includeBlock = true; // seed
        }
        else
        {
            const double runningMeanPower = accumulatedEnergy / std::max(accumulatedTime, 1e-12);
            const double runningLufs = powerToLufs(runningMeanPower);
            const double relativeGateThreshold = runningLufs - 10.0;
            if (blockLufs > relativeGateThreshold)
                includeBlock = true;
        }

        if (includeBlock)
        {
            accumulatedEnergy += meanPower * blockDuration;
            accumulatedTime += blockDuration;

            const double integratedMeanPower = accumulatedEnergy / std::max(accumulatedTime, 1e-12);
            const double integrated = powerToLufs(integratedMeanPower);

            integratedLufs.store(static_cast<float>(integrated), std::memory_order_release);
            integratedValid.store(true, std::memory_order_release);
        }
    }

    // reset block accumulators
    blockCounter = 0;
    blockEnergy = 0.0;
}

float LevelMeter::getIntegratedLufs() const noexcept
{
    return integratedLufs.load(std::memory_order_acquire);
}

bool LevelMeter::hasIntegratedLufs() const noexcept
{
    return integratedValid.load(std::memory_order_acquire);
}

float LevelMeter::getLastBlockLufs() const noexcept
{
    return lastBlockLufs.load(std::memory_order_relaxed);
}

