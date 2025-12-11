#pragma once

#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>

// Simple thread-safe spectrum analyzer that maintains a circular FIFO,
// produces an FFT when a full frame is available, and exposes a thread-safe
// copy API for magnitudes for the GUI.
class SpectrumAnalyzer
{
public:
    SpectrumAnalyzer(int fftOrder = 11); // 2^11 = 2048
    ~SpectrumAnalyzer() = default;

    void prepareToPlay(double sampleRate, int samplesPerBlockExpected);
    void pushAudioBlock(const float* input, int numSamples);   // called on audio thread
    void computeFFT();                                         // internal
    void updateSmoothedMagnitudes();                           // called on GUI timer

    std::vector<float> getMagnitudesCopy() const;             // thread-safe copy for GUI

    int getFftSize() const noexcept { return fftSize; }

private:
    mutable juce::CriticalSection lock;

    const int fftOrder;
    const int fftSize;
    const int hopSize;
    int fifoIndex = 0;
    int samplesSinceLastFFT = 0;
    bool fifoWrapped = false;

    std::unique_ptr<juce::dsp::FFT> fft;
    juce::dsp::WindowingFunction<float> window;

    std::vector<float> fifo;
    std::vector<float> fftData;
    std::vector<float> magnitude;
    std::vector<float> smoothedMagnitude;

    float smoothingFactor = 0.5f; // tweak 0.3-0.7 for smoothness
};