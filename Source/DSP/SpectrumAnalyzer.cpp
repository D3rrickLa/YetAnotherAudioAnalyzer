#include "SpectrumAnalyzer.h"

SpectrumAnalyzer::SpectrumAnalyzer(int fftOrder) : fftOrder(fftOrder), fftSize(1 << fftOrder), fft(fftOrder)
{
	fftData.calloc(2 * fftSize); // FFT needs 2x space for RT+imag
}

void SpectrumAnalyzer::prepareToPlay(double sampleRate)
{
    juce::ignoreUnused(sampleRate);
    fifoIndex = 0;
    zeromem(fftData, sizeof(float) * 2 * fftSize);
    zeromem(magnitude, sizeof(float) * fftSize / 2);
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
    fft.performRealOnlyForwardTransform(fftData);
    for (int i = 0; i < fftSize / 2; ++i)
    {
        const float re = fftData[i * 2];
        const float im = fftData[i * 2 + 1];
        magnitude[i] = std::sqrt(re * re + im * im);
    }
}