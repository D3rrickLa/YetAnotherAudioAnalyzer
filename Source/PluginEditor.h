/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class YetAnotherAudioAnalyzerAudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    YetAnotherAudioAnalyzerAudioProcessorEditor (YetAnotherAudioAnalyzerAudioProcessor&);
    ~YetAnotherAudioAnalyzerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback();
    YetAnotherAudioAnalyzerAudioProcessor& audioProcessor;

    // Basic values from meters
    float correlationValue = 1.0f;
    float widthValue = 0.0f;
    float levelValue = 0.0f;

    std::vector<float> leftMagnitudes;
    std::vector<float> rightMagnitudes;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(YetAnotherAudioAnalyzerAudioProcessorEditor)
};
