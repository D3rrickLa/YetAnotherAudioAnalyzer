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
    // Ask the processor analyzers to compute their current FFT (thread-safe)
    audioProcessor.getSpectrumAnalyzerL().computeFFT();
    audioProcessor.getSpectrumAnalyzerR().computeFFT();

    // Copy magnitudes out to editor-local vectors (to avoid locking during paint)
    {
        const juce::ScopedLock sl(audioProcessor.getSpectrumAnalyzerL().getLock()); // if you expose a lock getter
        leftMagnitudes = audioProcessor.getSpectrumAnalyzerL().getMagnitudesCopy(); // copy
    }
    {
        const juce::ScopedLock sl(audioProcessor.getSpectrumAnalyzerR().getLock());
        rightMagnitudes = audioProcessor.getSpectrumAnalyzerR().getMagnitudesCopy();
    }

    // Level meter: use integrated if available, otherwise draw 0 or fall back to peak/RMS.
    if (audioProcessor.getLevelMeter().hasIntegratedLufs())
        levelValue = audioProcessor.getLevelMeter().getIntegratedLufs();
    else
        levelValue = 0.0f;

    // get correlation / width as before
    correlationValue = audioProcessor.getCorrelationMeter().getCorrelation();
    audioProcessor.getStereoWidthMeter().getResults(correlationValue, widthValue);

    repaint();
}

//==============================================================================
void YetAnotherAudioAnalyzerAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);

    auto bounds = getLocalBounds();

    // Split layout into 3 rows
    auto topRow = bounds.removeFromTop(bounds.getHeight() * 0.45f);
    auto midRow = bounds.removeFromTop(bounds.getHeight() * 0.25f);
    auto bottomRow = bounds;

    // ===== TOP: FAKE SPECTRUM RECTANGLES (placeholders) =====
    auto leftSpectrum = topRow.removeFromLeft(topRow.getWidth() / 2);
    auto rightSpectrum = topRow;

    g.setColour(juce::Colours::darkgrey);
    g.fillRect(leftSpectrum.reduced(4));
    g.fillRect(rightSpectrum.reduced(4));

    g.setColour(juce::Colours::white);
    g.drawText("Left Spectrum", leftSpectrum, juce::Justification::centred);
    g.drawText("Right Spectrum", rightSpectrum, juce::Justification::centred);


    // ===== MIDDLE: CORRELATION METER =====
    float correlationNormalized = juce::jmap(correlationValue, -1.0f, 1.0f, 0.0f, 1.0f);

    auto corrRect = midRow.reduced(10);
    g.setColour(juce::Colours::grey);
    g.fillRect(corrRect);

    auto corrFill = corrRect.withWidth((float)corrRect.getWidth() * correlationNormalized);
    g.setColour(correlationValue < 0 ? juce::Colours::red : juce::Colours::green);
    g.fillRect(corrFill);

    g.setColour(juce::Colours::white);
    g.drawText("Correlation: " + juce::String(correlationValue, 2),
        corrRect, juce::Justification::centred);


    // ===== BOTTOM: LEVEL METER + WIDTH =====
    auto leftBottom = bottomRow.removeFromLeft(bottomRow.getWidth() * 0.5f);
    auto rightBottom = bottomRow;

    // Level meter vertical bar
    float levelHeight = juce::jlimit(0.0f, 1.0f, levelValue);
    auto levelRect = leftBottom.reduced(10);
    auto levelBar = levelRect.removeFromBottom(levelRect.getHeight() * levelHeight);

    g.setColour(juce::Colours::darkgrey);
    g.fillRect(levelRect);

    g.setColour(juce::Colours::skyblue);
    g.fillRect(levelBar);

    g.setColour(juce::Colours::white);
    g.drawText("Level", leftBottom, juce::Justification::centredTop);


    // Width meter bar (horizontal)
    auto widthRect = rightBottom.reduced(10);
    float widthNorm = juce::jlimit(0.0f, 2.0f, widthValue) / 2.0f;

    auto widthFill = widthRect.withWidth(widthRect.getWidth() * widthNorm);

    g.setColour(juce::Colours::darkgrey);
    g.fillRect(widthRect);

    g.setColour(juce::Colours::orange);
    g.fillRect(widthFill);

    g.setColour(juce::Colours::white);
    g.drawText("Width: " + juce::String(widthValue, 2),
        widthRect, juce::Justification::centred);
}

void YetAnotherAudioAnalyzerAudioProcessorEditor::resized()
{
    // No child components yet
}