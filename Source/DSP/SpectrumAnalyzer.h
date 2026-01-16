#pragma once

#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>

// Simple thread-safe spectrum analyzer that maintains a circular FIFO,
// produces an FFT when a full frame is available, and exposes a thread-safe
// copy API for magnitudes for the GUI.
class SpectrumAnalyzer
{
public:
    SpectrumAnalyzer(int fftOrder = 14); // 16384 FFT by default
    ~SpectrumAnalyzer() = default;

    void prepareToPlay(double sampleRate, int numChannels);
    void pushAudioBlock(const float* input, int numSamples);
    void updateSmoothedMagnitudes();
    std::vector<float> getMagnitudesCopy() const;

private:
    void computeFFT();
    
    mutable juce::CriticalSection lock;

    const int fftOrder;
    const int fftSize;
    const int hopSize;

    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> fifo;
    std::vector<float> fftData;
    std::vector<float> magnitude;          // linear FFT magnitude
    std::vector<float> smoothedMagnitude;  // linear, smoothed

    std::vector<float> hannWindow;
    float windowRMS = 1.0f;

    int fifoIndex = 0;
    bool fifoWrapped = false;
    int samplesSinceLastFFT = 0;

    // SPAN-style smoothing parameters
    float attack = 0.6f;   // fast attack
    float releaseLow = 0.05f;   // low freq release
    float releaseHigh = 0.4f;   // high freq release
};