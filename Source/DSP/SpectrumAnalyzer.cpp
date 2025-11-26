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
        if (fifoIndex >= fftSize) {
            fifoIndex = 0; // simple wrap-around FIFO
            fifoFilled = true;
        }
    }
}

void SpectrumAnalyzer::computeFFT()
{
    // Make sure we have a full frame (avoid computing on partially filled buffer).
    const juce::ScopedLock sl(lock);
    if (!fifoFilled)
        return; // not enough samples yet

    // assemble a contiguous frame of the most recent fftSize samples into temp
    std::vector<float> temp(2 * fftSize, 0.0f); // real+imag storage expected by JUCE
    for (int i = 0; i < fftSize; ++i)
        temp[i] = fftData[(fifoIndex + i) % fftSize];

    // apply window
    juce::dsp::WindowingFunction<float> win((size_t)fftSize, juce::dsp::WindowingFunction<float>::hann);
    win.multiplyWithWindowingTable(temp.data(), fftSize);

    // perform real FFT in-place (JUCE expects an array of size 2*fftSize)
    fft.performRealOnlyForwardTransform(temp.data());

    // compute magnitudes (only up to fftSize/2)
    const int numBins = fftSize / 2;
    magnitude.assign(numBins, 0.0f);

    // In JUCE real-FFT output: pairs [re0, reN/2, re1, im1, re2, im2, ...] can be complicated;
    // the common approach below matches the layout used by many examples (real/imag interleaved).
    for (int bin = 0; bin < numBins; ++bin)
    {
        const float re = temp[2 * bin];
        const float im = temp[2 * bin + 1];
        magnitude[bin] = std::sqrt(re * re + im * im);
    }
}

juce::CriticalSection& SpectrumAnalyzer::getLock()
{
    return lock;
}
