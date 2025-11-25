/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "DSP/SpectrumAnalyzer.h"
#include "DSP/CorrelationMeter.h"
#include "DSP/LevelMeter.h"
#include "DSP/StereoWidthVisualizer.h"

//==============================================================================
/**
*/
class YetAnotherAudioAnalyzerAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    YetAnotherAudioAnalyzerAudioProcessor();
    ~YetAnotherAudioAnalyzerAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    // ====== DSP Getters for Editor ======
    SpectrumAnalyzer& getSpectrumAnalyzerL() { return spectrumAnalyzerL; }
    SpectrumAnalyzer& getSpectrumAnalyzerR() { return spectrumAnalyzerR; }
    CorrelationMeter& getCorrelationMeter() { return correlationMeter; }
    LevelMeter& getLevelMeter() { return levelMeter; }
    StereoWidthVisualizer& getStereoWidthMeter() { return stereoWidthMeter; }
private:
    //==============================================================================
    SpectrumAnalyzer spectrumAnalyzerL;
    SpectrumAnalyzer spectrumAnalyzerR;
    CorrelationMeter correlationMeter;
    LevelMeter levelMeter;
    StereoWidthVisualizer stereoWidthMeter;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(YetAnotherAudioAnalyzerAudioProcessor)
};