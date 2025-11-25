/*
  ==============================================================================

    StereoWidthVisualizer.cpp
    Created: 14 Nov 2025 4:10:46pm
    Author:  Gen3r

  ==============================================================================
*/

#include "StereoWidthVisualizer.h"

void StereoWidthVisualizer::prepare(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(sampleRate, samplesPerBlock);
    reset();
}

void StereoWidthVisualizer::reset()
{
    sumL = sumR = sumLR = 0.0;
    sumM = sumS = 0.0;
    sampleCount = 0;
}

void StereoWidthVisualizer::processBlock(const juce::AudioBuffer<float>& buffer)
{
    if (buffer.getNumChannels() < 2)
        return;

    const float* L = buffer.getReadPointer(0);
    const float* R = buffer.getReadPointer(1);
    const int N = buffer.getNumSamples();

    for (int i = 0; i < N; ++i)
    {
        const float l = L[i];
        const float r = R[i];

        const float M = 0.5f * (l + r);
        const float S = 0.5f * (l - r);

        // Correlation data
        sumL += (double)l * l;
        sumR += (double)r * r;
        sumLR += (double)l * r;

        // M/S width data
        sumM += (double)M * M;
        sumS += (double)S * S;

        ++sampleCount;
    }
}

void StereoWidthVisualizer::getResults(float& correlationOut, float& widthOut)
{
    if (sampleCount == 0)
    {
        correlationOut = 1.0f;
        widthOut = 0.0f;
        return;
    }

    // ===== Correlation (–1 to +1) =====
    const double denom = std::sqrt(sumL * sumR);
    float corr = (denom > 0.0)
        ? (float)(sumLR / denom)
        : 1.0f;

    corr = juce::jlimit(-1.0f, 1.0f, corr);

    // ===== Width based on MS energy ratio =====
    const float rmsM = std::sqrt((float)(sumM / sampleCount));
    const float rmsS = std::sqrt((float)(sumS / sampleCount));

    float width = (rmsM > 0.0f ? rmsS / rmsM : 0.0f);
    width = juce::jlimit(0.0f, 2.0f, width);  // typical normalized range

    correlationOut = corr;
    widthOut = width;

    reset(); // Clear for next visual frame
}