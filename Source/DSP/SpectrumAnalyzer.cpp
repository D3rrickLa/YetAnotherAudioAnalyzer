#include "SpectrumAnalyzer.h"



SpectrumAnalyzer::SpectrumAnalyzer(int order)
    : fftOrder(order),
    fftSize(1 << order),
    hopSize(fftSize / 4), // 25% hop
    fft(std::make_unique<juce::dsp::FFT>(order)),
    window((size_t)(1 << order), juce::dsp::WindowingFunction<float>::hann),
    fifo((size_t)(1 << order), 0.0f),
    fftData((size_t)(2 * (1 << order)), 0.0f),
    magnitude((size_t)(1 << order) / 2, 0.0f),
    smoothedMagnitude((size_t)(1 << order) / 2, 0.0f)
{
    fifoIndex = 0;
    fifoWrapped = false;

    hannWindow.resize(fftSize);
    for (int i = 0; i < fftSize; ++i)
    {
        hannWindow[i] = 0.5f * (1.0f - std::cos(2.0 * juce::MathConstants<float>::pi * i / (fftSize - 1)));
        windowSum += hannWindow[i];
    }
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

    // Zero-pad the rest
    std::fill(fftData.begin() + fftSize, fftData.end(), 0.0f);

    // Apply Hann window manually
    for (int i = 0; i < fftSize; ++i)
        fftData[i] *= hannWindow[i];

    // Perform FFT
    fft->performRealOnlyForwardTransform(fftData.data());

    const int numBins = fftSize / 2;
    if (magnitude.size() != numBins)
    {
        magnitude.resize(numBins);
        smoothedMagnitude.resize(numBins);
    }

    // --- Correct magnitude with Hann window normalization ---
    // DC bin
    magnitude[0] = std::abs(fftData[0]) / windowSum;

    for (int bin = 1; bin < numBins; ++bin)
    {
        float re = fftData[2 * bin];
        float im = fftData[2 * bin + 1];

        // Multiply by 2 for non-DC bins (FFT symmetry) and divide by window sum
        magnitude[bin] = 2.0f * std::sqrt(re * re + im * im) / windowSum;
    }
}

void SpectrumAnalyzer::updateSmoothedMagnitudes()
{
    const juce::ScopedLock sl(lock);

    const int numBins = (int)magnitude.size();
    const float smoothingLow = smoothingFactor * 0.01f;  // very slow for bass
    const float smoothingHigh = smoothingFactor;         // fast for treble

    for (int i = 0; i < numBins; ++i)
    {
        float freqRatio = (float)i / (float)numBins;
        float factor = juce::jmap(freqRatio, 0.0f, 1.0f, smoothingLow, smoothingHigh);

        // RMS IIR smoothing
        float prev = smoothedMagnitude[i];
        smoothedMagnitude[i] = std::sqrt(
            factor * magnitude[i] * magnitude[i] +
            (1.0f - factor) * prev * prev
        );
    }

    // Low-frequency averaging (20–80 Hz) to reduce spikes
    int bassBins = juce::jmax(3, numBins / 32);
    std::vector<float> temp = smoothedMagnitude;
    for (int i = 0; i < bassBins; ++i)
    {
        int start = juce::jmax(0, i - 1);
        int end = juce::jmin(bassBins - 1, i + 1);

        float sum = 0.0f;
        for (int j = start; j <= end; ++j)
            sum += temp[j];

        smoothedMagnitude[i] = sum / (end - start + 1);
    }
}

std::vector<float> SpectrumAnalyzer::getMagnitudesCopy() const
{
    const juce::ScopedLock sl(lock);
    return smoothedMagnitude;
}