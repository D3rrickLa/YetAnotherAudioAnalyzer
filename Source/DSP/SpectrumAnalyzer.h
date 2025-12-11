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
    void pushAudioBlock(const float* input, int numSamples); // called on audio thread
    void computeFFT(); // safe to call from timer thread (will early-return if not ready)

    // thread-safe copy of magnitudes for GUI
    std::vector<float> getMagnitudesCopy() const;

    int getFftSize() const noexcept { return fftSize; }

private:
    mutable juce::CriticalSection lock;

    const int fftOrder;
    const int fftSize;

    std::unique_ptr<juce::dsp::FFT> fft;
    juce::dsp::WindowingFunction<float> window;

    // for smoothing the spectrum
    std::vector<float> smoothedMagnitude;
    float smoothingFactor = 0.3f; //tweak for smoothness, 0.1-0.5

    // circular FIFO holding the most recent fftSize samples
    std::vector<float> fifo;
    int fifoIndex = 0;
    bool fifoWrapped = false;

    // working buffer used for FFT (size 2*fftSize for JUCE real FFT)
    std::vector<float> fftData;

    // magnitude output (fftSize/2 bins)
    std::vector<float> magnitude;
};