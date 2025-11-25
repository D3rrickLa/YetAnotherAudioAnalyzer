/*
  ==============================================================================

    LevelMeter.h
    Created: 14 Nov 2025 4:07:26pm
    Author:  Gen3r

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>


/*
    Integrated LUFS need 
        1. K-weight filter, a fixed biquad cascade defined by EBU R128
            - high pass at 40 Hz
            - high shelf around 4 kHz
        2. channel weighting for 2-channel stereo
        3. Gate
            - absolute gate at -70 LUFS
            - relative gate (current ungated loudness) at -10 LUFS
            - only do the accumlation for blocks that pass these gates

        4. Block energy accumlation
            - use 400 ms block
            for each block
                - run through the k-filters, computer average power, convert to LUFS, apply gates. If it passes, accumlatet the total energy and time
*/
class LevelMeter
{
public:
	LevelMeter() = default;
	~LevelMeter() = default;
    void prepare(double sampleRate, int numChannels = 2);
    void processBlock(const float* const* input, int numSamples);
    float getIntegratedLufs() const { return (float) integratedLufs; }

    void reset();

private:
    double sr = 44100.0;

    // K-weighting filters
    juce::IIRFilter hpL, hpR;
    juce::IIRFilter shL, shR;

    int blockCounter = 0;
    double blockEnergy = 0.0;

    double accumulatedEnergy = 0.0;
    double accumulatedTime = 0.0;

    double integratedLufs = 0.0;

    void computeBlock();
};
