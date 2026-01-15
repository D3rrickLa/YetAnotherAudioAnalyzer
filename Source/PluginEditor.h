/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <juce_core/juce_core.h>

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
    void paintViewHeader(juce::Graphics& g);
    void paintMainView(juce::Graphics& g);
    void paintMeterFooter(juce::Graphics& g);

    void paintSpectrumScreen(juce::Graphics&, juce::Rectangle<int> area);
    void paintMultibandScreen(juce::Graphics&, juce::Rectangle<int> area);

    void drawFrequencyOverlay(juce::Graphics& g, juce::Rectangle<int> area);

private:
    void timerCallback();
    float logX(int bin, int numBins, float width, float sampleRate);
    float xToFrequency(float xNorm) const;
    float interpolateMagnitude(const std::vector<float>& mags, float freq, float sampleRate);
    YetAnotherAudioAnalyzerAudioProcessor& audioProcessor;

    // Basic values from meters
    std::vector<float> leftMagnitudes, rightMagnitudes;
    float levelValue = 0.0f;
    float correlationValue = 1.0f;
    float widthValue = 0.0f;
    float minDb = -60.0f;
    float maxDb = 0.0f;
    ViewMode currentView = ViewMode::Spectrum;
    juce::TextButton viewSwitchButton{ "Switch View" };



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(YetAnotherAudioAnalyzerAudioProcessorEditor)
};
