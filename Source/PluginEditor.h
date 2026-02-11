/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <juce_core/juce_core.h>
#include <iostream>

//==============================================================================
/**
*/

enum class ViewMode { Spectrum, MultibandCorrelation, StereoWidth, AdvanceLufs };

class YetAnotherAudioAnalyzerAudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    YetAnotherAudioAnalyzerAudioProcessorEditor (YetAnotherAudioAnalyzerAudioProcessor&);
    ~YetAnotherAudioAnalyzerAudioProcessorEditor() override = default;

    void setView(ViewMode newView);

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void paintViewHeader(juce::Graphics& g);
    void paintMainView(juce::Graphics& g);
    void paintMeterFooter(juce::Graphics& g);

    void paintSpectrumScreen(juce::Graphics&, juce::Rectangle<int> area);
    void drawFrequencyOverlay(juce::Graphics& g, juce::Rectangle<int> area);

    void paintMultibandScreen(juce::Graphics&, juce::Rectangle<int> area);
    void paintStereoWidthScreen(juce::Graphics&, juce::Rectangle<int> area);
    void paintLufsScreen(juce::Graphics&, juce::Rectangle<int> area);


private:
    void timerCallback();
    float logX(int bin, int numBins, float width, float sampleRate);
    float xToFrequency(float xNorm) const;
    float interpolateMagnitude(const std::vector<float>& mags, float freq, float sampleRate);
    void drawFooterWidth(juce::Graphics& g, juce::Rectangle<int> area);
    void drawFooterCorrelation(juce::Graphics& g, juce::Rectangle<int> area);
    void updateStereoScope(const juce::AudioBuffer<float>& buffer);
    YetAnotherAudioAnalyzerAudioProcessor& audioProcessor;

    // Basic values from meters
    std::vector<float> leftMagnitudes, rightMagnitudes;
    float levelValue = 0.0f;
    float correlationValue = 1.0f;
    float widthValue = 0.5f;
    float minDb = -60.0f;
    float maxDb = 0.0f;

    float leftLevel = 0.0f;
    float rightLevel = 0.0f;

    ViewMode currentView = ViewMode::Spectrum;

    juce::Rectangle<int> mainViewArea;
    juce::Rectangle<int> meterFooterArea;
    juce::Rectangle<int> levelMeterArea;
    juce::Rectangle<int> stereoFooterArea;

    juce::TextButton multibandCorrelationTab { "Multiband Correlation" };
    juce::TextButton spectrumTab{ "Spectrum" };
    juce::TextButton stereoTab{ "Stereo" };
    juce::TextButton lufsTab{ "LUFS" };

    juce::TextButton monoButton{ "Mono" };
    juce::TextButton abButton{ "A/B" };

    std::vector<juce::Point<float>> stereoScopePoints;
z
    float smoothedLeft = 0.0f;
    float smoothedRight = 0.0f;

    float peakLeft = 0.0f;
    float peakRight = 0.0f;
     
    const float smoothingFactor = 0.2f; // 0 = instant, 1 = no change
    const float peakDecay = 0.01f;      // peak drops slowly



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(YetAnotherAudioAnalyzerAudioProcessorEditor)
};
