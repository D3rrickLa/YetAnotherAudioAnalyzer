#include "SpectrumAnalyzer.h"

SpectrumAnalyzer::SpectrumAnalyzer(int order)
    : fftOrder(order),
    fftSize(1 << order),
    hopSize((1 << order) / 4),
    fft(std::make_unique<juce::dsp::FFT>(order)),
    fifo(fftSize, 0.0f),
    fftData(2 * fftSize, 0.0f),
    magnitude(fftSize / 2, 0.0f),
    smoothedMagnitude(fftSize / 2, 0.0f)
{
    fifoIndex = 0;
    fifoWrapped = false;

    // Hann window + RMS normalization
    hannWindow.resize(fftSize);
    float sumSquares = 0.0f;
    for (int i = 0; i < fftSize; ++i)
    {
        hannWindow[i] = 0.5f * (1.0f - std::cos(2.0 * juce::MathConstants<float>::pi * i / (fftSize - 1)));
        sumSquares += hannWindow[i] * hannWindow[i];
    }
    windowRMS = std::sqrt(sumSquares);
}

void SpectrumAnalyzer::prepareToPlay(double, int)
{
    const juce::ScopedLock sl(lock);
    std::fill(fifo.begin(), fifo.end(), 0.0f);
    std::fill(fftData.begin(), fftData.end(), 0.0f);
    std::fill(magnitude.begin(), magnitude.end(), 0.0f);
    std::fill(smoothedMagnitude.begin(), smoothedMagnitude.end(), 0.0f);
    fifoIndex = 0;
    fifoWrapped = false;
    samplesSinceLastFFT = 0;
}

void SpectrumAnalyzer::pushAudioBlock(const float* input, int numSamples)
{
    if (!input || numSamples <= 0)
        return;

    const juce::ScopedLock sl(lock);

    for (int i = 0; i < numSamples; ++i)
    {
        fifo[fifoIndex++] = input[i];
        samplesSinceLastFFT++;

        if (fifoIndex >= fftSize)
            fifoIndex = 0;

        if (samplesSinceLastFFT >= hopSize)
        {
            samplesSinceLastFFT = 0;
            if (fifoWrapped || fifoIndex >= hopSize)
                computeFFT();
        }

        if (fifoIndex == 0)
            fifoWrapped = true;
    }
}

void SpectrumAnalyzer::computeFFT()
{
    if (!fifoWrapped)
        return;

    // Copy latest fftSize samples in chronological order
    for (int i = 0; i < fftSize; ++i)
        fftData[i] = fifo[(fifoIndex + i) % fftSize];

    // Remove DC / mean
    float mean = 0.0f;
    for (int i = 0; i < fftSize; ++i)
        mean += fftData[i];
    mean /= fftSize;
    for (int i = 0; i < fftSize; ++i)
        fftData[i] -= mean;

    // Apply Hann window
    for (int i = 0; i < fftSize; ++i)
        fftData[i] *= hannWindow[i];

    // FFT
    fft->performRealOnlyForwardTransform(fftData.data());

    const int numBins = fftSize / 2;

    // Linear magnitude, full-scale normalized
    magnitude[0] = 0.0f; // DC removed

    for (int bin = 1; bin < numBins; ++bin)
    {
        float re = fftData[2 * bin];
        float im = fftData[2 * bin + 1];
        float magLinear = 2.0f * std::sqrt(re * re + im * im) / (fftSize * 0.5f); // full-scale sine = 1.0
        magnitude[bin] = magLinear;
    }
}

void SpectrumAnalyzer::updateSmoothedMagnitudes()
{
    const juce::ScopedLock sl(lock);
    const int numBins = (int)magnitude.size();

    for (int i = 0; i < numBins; ++i)
    {
        float freqRatio = (float)i / (float)numBins;
        float release = juce::jmap(freqRatio, 0.0f, 1.0f, releaseLow, releaseHigh);

        float input = magnitude[i];
        float prev = smoothedMagnitude[i];

        if (input > prev)
            smoothedMagnitude[i] = attack * input + (1.0f - attack) * prev;
        else
            smoothedMagnitude[i] = release * input + (1.0f - release) * prev;
    }
}

std::vector<float> SpectrumAnalyzer::getMagnitudesCopy() const
{
    const juce::ScopedLock sl(lock);
    return smoothedMagnitude; // linear, ready for dB conversion at paint
}