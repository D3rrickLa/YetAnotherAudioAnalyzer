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


    addAndMakeVisible(viewSwitchButton);
    viewSwitchButton.setButtonText("Switch View");


    viewSwitchButton.onClick = [this]()
        {
            if (currentView == ViewMode::Spectrum)
                currentView = ViewMode::MultibandCorrelation;
            else
                currentView = ViewMode::Spectrum;

            repaint();
        };


    // Update GUI 60 times per second
    startTimerHz(60);
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
    g.fillAll(juce::Colours::grey);

    const int headerHeight = 50;

    // Draw top header background
    g.setColour(juce::Colours::darkgrey.darker(0.2f));
    g.fillRect(0, 0, getWidth(), headerHeight);

    // Draw static UI labels in the header area
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);

    g.drawText("LUFS: " + juce::String(levelValue, 2),
        170, 10, 100, 30, juce::Justification::left);

    g.drawText("Corr: " + juce::String(correlationValue, 2),
        280, 10, 100, 30, juce::Justification::left);

    g.drawText("Width: " + juce::String(widthValue, 2),
        390, 10, 100, 30, juce::Justification::left);

    // Paint the currently active screen *below* the header
    if (currentView == ViewMode::Spectrum)
        paintSpectrumScreen(g, headerHeight);
    else
        paintMultibandScreen(g, headerHeight);

}

void YetAnotherAudioAnalyzerAudioProcessorEditor::paintSpectrumScreen(juce::Graphics& g, int headerHeight)
{
    const auto& mags = leftMagnitudes;
    if (mags.empty())
        return;

    const int w = getWidth();
    const int h = getHeight();
    const int availableHeight = h - headerHeight;

    const int numBins = (int)mags.size();
    const float binW = (float)w / (float)numBins;

    for (int i = 0; i < numBins; ++i)
    {
        const float mag = mags[(size_t)i];
        const float db = juce::Decibels::gainToDecibels(mag + 1e-12f);
        const float dbClamped = juce::jlimit(-100.0f, 0.0f, db);

        // map into area *below header*
        float y = juce::jmap(dbClamped,
            -100.0f, 0.0f,
            (float)(h), (float)(headerHeight + 10));

        g.setColour(juce::Colours::lightblue.withAlpha(0.9f));
        g.fillRect(i * binW,
            y,
            juce::jmax(1.0f, binW - 1.0f),
            (float)h - y);
    }

}

void YetAnotherAudioAnalyzerAudioProcessorEditor::paintMultibandScreen(juce::Graphics& g, int headerHeight)
{
    g.setColour(juce::Colours::white);
    g.drawText("Multiband correlation screen (WIP)",
        20, headerHeight + 20, 400, 30,
        juce::Justification::left);
}

void YetAnotherAudioAnalyzerAudioProcessorEditor::resized()
{
    const int headerHeight = 50;

    viewSwitchButton.setBounds(10, 10, 150, 30);
}