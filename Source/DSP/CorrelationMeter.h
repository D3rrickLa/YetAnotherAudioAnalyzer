/*
  ==============================================================================

    CorrelationMeter.h
    Created: 14 Nov 2025 4:06:31pm
    Author:  Gen3r

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class CorrelationMeter
{
public:
	CorrelationMeter();
	~CorrelationMeter();

    void prepareToPlay(int bufferSize);
    void pushAudioBlock(const float* left, const float* right, int numSamples);
    float getCorrelation() const; // Returns -1 to +1

private:
    juce::HeapBlock<float> leftBuffer;
    juce::HeapBlock<float> rightBuffer;
    int fifoIndex = 0;
    int bufferSize = 1024;
    juce::CriticalSection lock;
};
