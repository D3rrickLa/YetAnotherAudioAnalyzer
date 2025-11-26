#pragma once

#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>

/*
should be just a helper class that handles FFTs, magnitudes, and any spectrum-specific state. 
It doesn’t need to inherit from AudioProcessor. It just needs methods to push audio blocks in and compute the FFT.
*/
class SpectrumAnalyzer
{
public:
    SpectrumAnalyzer(int fftOrder = 11); // 2^11 = 2048 points
    ~SpectrumAnalyzer() = default;

    void prepareToPlay(double sampleRate, int samplePerBlock);
    void SpectrumAnalyzer::pushAudioBlock(const float* input, int numSamples);
    void computeFFT();
    const std::vector<float>& getMagnitude() const { return magnitude; }

    std::vector<float> getMagnitudesCopy() const {
        const juce::ScopedLock sl(lock);
        return magnitude;
    }

    juce::CriticalSection& getLock();

private:
    int fftOrder;
    int fftSize;
    juce::dsp::FFT fft;
    std::vector<float> fftData;
    std::vector<float> magnitude;
    int fifoIndex = 0;
    bool fifoFilled = false;
    juce::CriticalSection lock;

    juce::dsp::WindowingFunction<float> window{ (size_t)fftSize, juce::dsp::WindowingFunction<float>::hann };
};