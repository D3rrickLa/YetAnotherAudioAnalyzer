/*
  ==============================================================================

    CorrelationMeter.cpp
    Created: 14 Nov 2025 4:06:31pm
    Author:  Gen3r

  ==============================================================================
*/

#include "CorrelationMeter.h"

CorrelationMeter::CorrelationMeter()
{
}

CorrelationMeter::~CorrelationMeter()
{
}

void CorrelationMeter::prepareToPlay(int size)
{
    const juce::ScopedLock sl(lock);
    bufferSize = size;
    leftBuffer.calloc(bufferSize);
    rightBuffer.calloc(bufferSize);
    fifoIndex = 0;
}

void CorrelationMeter::pushAudioBlock(const float* left, const float* right, int numSamples)
{
    const juce::ScopedLock sl(lock);
    for (int i = 0; i < numSamples; ++i)
    {
        leftBuffer[fifoIndex] = left[i];
        rightBuffer[fifoIndex] = right[i];
        fifoIndex = (fifoIndex + 1) % bufferSize;
    }
}

float CorrelationMeter::getCorrelation() const
{
    const juce::ScopedLock sl(lock);
    double sumLR = 0.0;
    double sumL2 = 0.0;
    double sumR2 = 0.0;

    for (int i = 0; i < bufferSize; ++i)
    {
        sumLR += leftBuffer[i] * rightBuffer[i];
        sumL2 += leftBuffer[i] * leftBuffer[i];
        sumR2 += rightBuffer[i] * rightBuffer[i];
    }

    if (sumL2 <= 0.0 || sumR2 <= 0.0)
        return 0.0f; // avoid divide by zero

    return static_cast<float>(sumLR / std::sqrt(sumL2 * sumR2));
}
