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
    const juce::HeapBlock<float>& getMagnitude() const { return magnitude; }

private:
    int fftOrder;
    int fftSize;
    juce::dsp::FFT fft;
    juce::HeapBlock<float> fftData;
    juce::HeapBlock<float> magnitude;
    int fifoIndex = 0;
    juce::CriticalSection lock;
};