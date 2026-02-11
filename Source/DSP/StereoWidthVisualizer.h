/*
  ==============================================================================

    StereoWidthVisualizer.h
    Created: 14 Nov 2025 4:10:46pm
    Author:  Gen3r

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class StereoWidthVisualizer
{
public:
    StereoWidthVisualizer() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    // Feed every audio block here
    void processBlock(const juce::AudioBuffer<float>& buffer);

    // Call from GUI timer (e.g., 30–60 Hz)
    void getResults(float& correlationOut, float& widthOut);

private:
    double sumL = 0.0;
    double sumR = 0.0;
    double sumLR = 0.0;

    double sumM = 0.0;
    double sumS = 0.0;

    int sampleCount = 0;

    float smoothedCorrelation = 1.0f;
    float smoothedWidth = 0.0f;
};