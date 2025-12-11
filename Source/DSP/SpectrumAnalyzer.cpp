#include "SpectrumAnalyzer.h"

SpectrumAnalyzer::SpectrumAnalyzer(int order)
    : 
    fftOrder(order),
    fftSize(1 << order),
    hopSize(fftSize / 4), // 25% hope, tweak for performance/smoothness
    fft(std::make_unique<juce::dsp::FFT>(order)),
    window((size_t)(1 << order), juce::dsp::WindowingFunction<float>::hann),
    fifo((size_t)(1 << order), 0.0f),
    fftData((size_t)2 * (1 << order), 0.0f),
    magnitude((size_t)((1 << order) / 2), 0.0f),
    smoothedMagnitude((size_t)((1 << order) / 2), 0.0f)
{
    fifoIndex = 0;
    fifoWrapped = false;
}

void SpectrumAnalyzer::prepareToPlay(double /*sampleRate*/, int /*samplesPerBlockExpected*/)
{
    const juce::ScopedLock sl(lock);
    std::fill(fifo.begin(), fifo.end(), 0.0f);
    std::fill(fftData.begin(), fftData.end(), 0.0f);
    std::fill(magnitude.begin(), magnitude.end(), 0.0f);
    fifoIndex = 0;
    fifoWrapped = false;
}

void SpectrumAnalyzer::pushAudioBlock(const float* input, int numSamples)
{
    // Called on audio thread. We append samples to fifo; when it wraps once
    // we mark fifoWrapped true so computeFFT() can produce frames.
    if (input == nullptr || numSamples <= 0)
        return;

    const juce::ScopedLock sl(lock);
    for (int i = 0; i < numSamples; ++i)
    {
        fifo[fifoIndex++] = input[i];
        samplesSinceLastFFT++;

        if (fifoIndex >= fftSize)
        {   
            fifoIndex = 0;
        }

        // computer FFT every hopSize samples (sliding window)
        if (samplesSinceLastFFT >= hopSize)
        {
            samplesSinceLastFFT = 0;
            if (fifoWrapped ||| fifoIndex >= hopSize) // ensure FIFO has enough data
            {
                computeFFT();
            }
        }
        if (fifoIndex == 0)
        {
            fifoWrapped = true;
        }
    }
}

void SpectrumAnalyzer::computeFFT()
{
    if (!fifoWrapped)
        return;

    // Copy latest fftSize samples in chronological order
    for (int i = 0; i < fftSize; ++i)
    {
        int src = (fifoIndex + i) % fftSize;
        fftData[i] = fifo[src];
    }

    std::fill(fftData.begin() + fftSize, fftData.end(), 0.0f);

    window.multiplyWithWindowingTable(fftData.data(), fftSize);

    fft->performRealOnlyForwardTransform(fftData.data());

    const int numBins = fftSize / 2;
    if ((int)magnitude.size() != numBins)
    {
        magnitude.resize(numBins);
        smoothedMagnitude.resize(numBins);
    }

    magnitude[0] = std::abs(fftData[0]);
    for (int bin = 1; bin < numBins; ++bin)
    {
        const float re = fftData[2 * bin];
        const float im = fftData[2 * bin + 1];
        magnitude[bin] = std::sqrt(re * re + im * im);

        // exponential smoothing
        smoothedMagnitude[bin] = smoothingFactor * magnitude[bin]
            + (1.0f - smoothingFactor) * smoothedMagnitude[bin];
    }
}

std::vector<float> SpectrumAnalyzer::getMagnitudesCopy() const
{
    const juce::ScopedLock sl(lock);
    return smoothedMagnitude; // copy under lock, returns the smoothed out version, not raw
}