#include "SpectrumAnalyzer.h"

SpectrumAnalyzer::SpectrumAnalyzer(int fftOrder)     : fftOrder(fftOrder),
      fftSize(1 << fftOrder),
      fft(fftOrder)
{
    fftData.resize(2 * fftSize, 0.0f);  // FFT needs 2x space for RT+imag
}

void SpectrumAnalyzer::prepareToPlay(double sampleRate, int samplePerBlock)
{
    juce::ignoreUnused(sampleRate);
    fifoIndex = 0;
    fftData.resize(2 * fftSize, 0.0f);          // allocate and zero
    magnitude.resize(fftSize / 2, 0.0f);        // allocate and zero
}

void SpectrumAnalyzer::pushAudioBlock(const float* input, int numSamples)
{
    const juce::ScopedLock sl(lock);
    for (int i = 0; i < numSamples; ++i)
    {
        fftData[fifoIndex++] = input[i];
        if (fifoIndex >= fftSize)
            fifoIndex = 0; // simple wrap-around FIFO
    }
}

void SpectrumAnalyzer::computeFFT()
{
    const juce::ScopedLock sl(lock);
    fft.performRealOnlyForwardTransform(fftData.data());
    for (int i = 0; i < fftSize / 2; ++i)
    {
        const float re = fftData[i * 2];
        const float im = fftData[i * 2 + 1];
        magnitude[i] = std::sqrt(re * re + im * im);
    }
}