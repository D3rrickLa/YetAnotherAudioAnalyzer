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

enum class ViewMode { Spectrum, MultibandCorrelation, AdvanceLufs };

class YetAnotherAudioAnalyzerAudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    YetAnotherAudioAnalyzerAudioProcessorEditor (YetAnotherAudioAnalyzerAudioProcessor&);
    ~YetAnotherAudioAnalyzerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void paintSpectrumScreen(juce::Graphics&, int headerHeight);
    void paintMultibandScreen(juce::Graphics&, int headerHeight);

private:
    void timerCallback();
    YetAnotherAudioAnalyzerAudioProcessor& audioProcessor;

    // Basic values from meters
    std::vector<float> leftMagnitudes, rightMagnitudes;
    float levelValue = 0.0f;
    float correlationValue = 1.0f;
    float widthValue = 0.0f;
    ViewMode currentView = ViewMode::Spectrum;
    juce::TextButton viewSwitchButton{ "Switch View" };



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(YetAnotherAudioAnalyzerAudioProcessorEditor)
};
