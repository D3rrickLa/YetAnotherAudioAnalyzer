/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
YetAnotherAudioAnalyzerAudioProcessor::YetAnotherAudioAnalyzerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

YetAnotherAudioAnalyzerAudioProcessor::~YetAnotherAudioAnalyzerAudioProcessor()
{
}

//==============================================================================
const juce::String YetAnotherAudioAnalyzerAudioProcessor::getName() const { return JucePlugin_Name; }

bool YetAnotherAudioAnalyzerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool YetAnotherAudioAnalyzerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool YetAnotherAudioAnalyzerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double YetAnotherAudioAnalyzerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int YetAnotherAudioAnalyzerAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int YetAnotherAudioAnalyzerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void YetAnotherAudioAnalyzerAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String YetAnotherAudioAnalyzerAudioProcessor::getProgramName (int index)
{
    return {};
}

void YetAnotherAudioAnalyzerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void YetAnotherAudioAnalyzerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    spectrumAnalyzerL.prepareToPlay(sampleRate, samplesPerBlock);
    spectrumAnalyzerR.prepareToPlay(sampleRate, samplesPerBlock);

    correlationMeter.prepareToPlay(1024);
    levelMeter.prepare(sampleRate, samplesPerBlock);
    stereoWidthMeter.prepare(sampleRate, samplesPerBlock);

}

void YetAnotherAudioAnalyzerAudioProcessor::releaseResources()
{
    // Nothing to release for analyzers
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool YetAnotherAudioAnalyzerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void YetAnotherAudioAnalyzerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{

    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const float* left = buffer.getReadPointer(0);
    const float* right = buffer.getReadPointer(1);

    // ---- FEED DSP MODULES ----
    spectrumAnalyzerL.pushAudioBlock(left, numSamples);
    spectrumAnalyzerR.pushAudioBlock(right, numSamples);

    correlationMeter.pushAudioBlock(left, right, numSamples);
    levelMeter.processBuffer(buffer);
    stereoWidthMeter.processBlock(buffer);

    // Analyzer plugin → pass audio through untouched
}

//==============================================================================
bool YetAnotherAudioAnalyzerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* YetAnotherAudioAnalyzerAudioProcessor::createEditor()
{
    return new YetAnotherAudioAnalyzerAudioProcessorEditor (*this);
}

//==============================================================================
void YetAnotherAudioAnalyzerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ignoreUnused(destData);
    // No parameters yet — implement later if needed
}

void YetAnotherAudioAnalyzerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused(data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new YetAnotherAudioAnalyzerAudioProcessor();
}
