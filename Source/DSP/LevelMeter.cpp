/*
  ==============================================================================

    LevelMeter.cpp
    Created: 14 Nov 2025 4:07:26pm
    Author:  Gen3r

  ==============================================================================
*/

#include "LevelMeter.h"

LevelMeter::LevelMeter()
{
}

LevelMeter::~LevelMeter()
{
}

// these are approx of the official values, replace later with exact values of BS.17770 coefficients
void LevelMeter::prepare(double sampleRate, int numChannels = 2)
{
    sr = sampleRate;

    // --- K-weighting filters ---
    // High-pass 40 Hz, Q = 0.5
    hpL.setCoefficients(juce::IIRCoefficients::makeHighPass(sr, 40.0, 0.707));
    hpR.setCoefficients(juce::IIRCoefficients::makeHighPass(sr, 40.0, 0.707));

    // High-shelf +4 dB @ 4 kHz
    shL.setCoefficients(juce::IIRCoefficients::makeHighShelf(sr, 4000.0, 0.5, juce::Decibels::decibelsToGain(4.0)));
    shR.setCoefficients(juce::IIRCoefficients::makeHighShelf(sr, 4000.0, 0.5, juce::Decibels::decibelsToGain(4.0)));

    reset();
}

void LevelMeter::processBlock(const float* const* input, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        float l = hpL.processSingleSampleRaw(input[0][i]);
        l = shL.processSingleSampleRaw(l);

        float r = hpR.processSingleSampleRaw(input[1][i]);
        r = shR.processSingleSampleRaw(r);

        double p = 0.5 * ((double)l * l + (double)r * r);

        blockEnergy += p;
        blockCounter++;

        const int blockSize = int(0.400 * sr);

        if (blockCounter >= blockSize)
            computeBlock();
    }
}

void LevelMeter::computeBlock()
{
    double duration = blockCounter / sr;
    double meanPower = blockEnergy / blockCounter;

    double loudness = -0.691 + 10.0 * std::log10(meanPower + 1e-12);

    // --- Gate ---
    if (loudness > -70.0)  // absolute gate
    {
        double relativeGate = integratedLufs - 10.0;

        if (loudness > relativeGate)
        {
            accumulatedEnergy += meanPower * duration;
            accumulatedTime += duration;

            integratedLufs = -0.691 +
                10.0 * std::log10(accumulatedEnergy / accumulatedTime);
        }
    }

    // reset block
    blockEnergy = 0.0;
    blockCounter = 0;
}
