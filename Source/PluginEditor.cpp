/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
YetAnotherAudioAnalyzerAudioProcessorEditor::YetAnotherAudioAnalyzerAudioProcessorEditor(YetAnotherAudioAnalyzerAudioProcessor& p) 
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize(800, 400);

    // Update GUI 30 times per second
    startTimerHz(30);
}

YetAnotherAudioAnalyzerAudioProcessorEditor::~YetAnotherAudioAnalyzerAudioProcessorEditor()
{
}

void YetAnotherAudioAnalyzerAudioProcessorEditor::timerCallback()
{
    // trigger FFT compute on the processors (computeFFT is safe and copy-under-lock)
    audioProcessor.getSpectrumAnalyzerL().computeFFT();
    audioProcessor.getSpectrumAnalyzerR().computeFFT();

    // get copies for GUI
    leftMagnitudes = audioProcessor.getSpectrumAnalyzerL().getMagnitudesCopy();
    rightMagnitudes = audioProcessor.getSpectrumAnalyzerR().getMagnitudesCopy();

    // level: prefer integrated, otherwise last-block
    if (audioProcessor.getLevelMeter().hasIntegratedLufs())
        levelValue = audioProcessor.getLevelMeter().getIntegratedLufs();
    else
        levelValue = audioProcessor.getLevelMeter().getLastBlockLufs();

    // correlation/width (use your existing accessors)
    correlationValue = audioProcessor.getCorrelationMeter().getCorrelation();
    audioProcessor.getStereoWidthMeter().getResults(correlationValue, widthValue);

    repaint();
}

//==============================================================================
void YetAnotherAudioAnalyzerAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);

    // draw LUFS text
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    g.drawText("LUFS: " + juce::String(levelValue, 2), 10, 10, 200, 20, juce::Justification::left);

    // draw a simple spectrum (leftMagnitudes)
    const auto& mags = leftMagnitudes;
    if (mags.empty())
        return;

    const int w = getWidth();
    const int h = getHeight();
    const int numBins = (int)mags.size();
    const float binW = (float)w / (float)numBins;

    for (int i = 0; i < numBins; ++i)
    {
        const float mag = mags[(size_t)i];
        const float db = juce::Decibels::gainToDecibels(mag + 1e-12f);
        const float dbClamped = juce::jlimit(-100.0f, 0.0f, db);
        const float y = juce::jmap(dbClamped, -100.0f, 0.0f, (float)h, 40.0f); // leave top margin
        const float x = i * binW;
        g.setColour(juce::Colours::lightblue.withAlpha(0.9f));
        g.fillRect(x, y, juce::jmax(1.0f, binW - 1.0f), (float)h - y);
    }

    // draw correlation and width
    g.setColour(juce::Colours::yellow);
    g.setFont(14.0f);
    g.drawText("Correlation: " + juce::String(correlationValue, 2),
        10, 30, 200, 20, juce::Justification::left);

    g.drawText("Width: " + juce::String(widthValue, 2),
        10, 50, 200, 20, juce::Justification::left);

}

void YetAnotherAudioAnalyzerAudioProcessorEditor::resized()
{
    // No child components yet
}