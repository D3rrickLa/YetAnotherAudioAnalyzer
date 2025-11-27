#include "SpectrumAnalyzer.h"

SpectrumAnalyzer::SpectrumAnalyzer(int order)
    : fftOrder(order),
    fftSize(1 << order),
    fft(std::make_unique<juce::dsp::FFT>(order)),
    window((size_t)(1 << order), juce::dsp::WindowingFunction<float>::hann),
    fifo((size_t)(1 << order), 0.0f),
    fftData((size_t)2 * (1 << order), 0.0f),
    magnitude((size_t)((1 << order) / 2), 0.0f)
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
        if (fifoIndex >= fftSize)
        {
            fifoIndex = 0;
            fifoWrapped = true;
        }
    }
}

void SpectrumAnalyzer::computeFFT()
{
    // Call from timer thread. Early-return if we don't have a full frame yet.
    const juce::ScopedLock sl(lock);
    if (!fifoWrapped)
        return;

    // Copy most recent fftSize samples into fftData[0..fftSize-1] in chronological order:
    // oldest sample is at fifoIndex, newest just before fifoIndex.
    for (int i = 0; i < fftSize; ++i)
    {
        int src = (fifoIndex + i) % fftSize;
        fftData[i] = fifo[src];
    }

    // Zero imaginary portion (required by JUCE real FFT layout)
    std::fill(fftData.begin() + fftSize, fftData.end(), 0.0f);

    // window the real samples in-place
    window.multiplyWithWindowingTable(fftData.data(), fftSize);

    // perform in-place real FFT (data length must be 2*fftSize)
    fft->performRealOnlyForwardTransform(fftData.data());

    // compute magnitudes. JUCE real-FFT layout:
    // fftData[0] = real(0) (DC)
    // fftData[1] = real(N/2) (Nyquist) (for even N)
    // for k=1..N/2-1: re = fftData[2*k], im = fftData[2*k+1]
    const int numBins = fftSize / 2;
    if ((int)magnitude.size() != numBins)
        magnitude.resize(numBins);

    // bin 0 (DC)
    magnitude[0] = std::abs(fftData[0]);

    // bins 1..numBins-1
    for (int bin = 1; bin < numBins; ++bin)
    {
        const float re = fftData[2 * bin];
        const float im = fftData[2 * bin + 1];
        magnitude[bin] = std::sqrt(re * re + im * im);
    }

    // Note: we keep fifoWrapped true so we continuously produce frames (sliding window).
    // If you want non-overlapping frames, change pushAudioBlock to copy+compute and reset fifoWrapped.
}

std::vector<float> SpectrumAnalyzer::getMagnitudesCopy() const
{
    const juce::ScopedLock sl(lock);
    return magnitude; // copy under lock
}