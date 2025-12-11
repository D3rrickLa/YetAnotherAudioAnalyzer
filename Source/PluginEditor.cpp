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


    // Update GUI 30 times per second
    startTimerHz(60);
}

YetAnotherAudioAnalyzerAudioProcessorEditor::~YetAnotherAudioAnalyzerAudioProcessorEditor()
{
}

void YetAnotherAudioAnalyzerAudioProcessorEditor::timerCallback()
{
    audioProcessor.getSpectrumAnalyzerL().updateSmoothedMagnitudes();
    audioProcessor.getSpectrumAnalyzerR().updateSmoothedMagnitudes();

    leftMagnitudes = audioProcessor.getSpectrumAnalyzerL().getMagnitudesCopy();
    rightMagnitudes = audioProcessor.getSpectrumAnalyzerR().getMagnitudesCopy();

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
    {
        paintSpectrumScreen(g, headerHeight);
        drawFrequencyOverlay(g, headerHeight);
    }
    else
    {
        paintMultibandScreen(g, headerHeight);
    }

}

float logX(int bin, int numBins, float width, float sampleRate)
{
    // frequency of this bin
    float freq = (float)bin / (float)numBins * (sampleRate / 2.0f); // 0..Nyquist

    // log scale (avoid log(0))
    float minFreq = 20.0f;
    float maxFreq = sampleRate / 2.0f;
    float logMin = std::log10(minFreq);
    float logMax = std::log10(maxFreq);
    float logFreq = std::log10(juce::jmax(freq, minFreq));

    return ((logFreq - logMin) / (logMax - logMin)) * width;
}

void YetAnotherAudioAnalyzerAudioProcessorEditor::paintSpectrumScreen(juce::Graphics& g, int headerHeight)
{
    const auto& mags = leftMagnitudes;
    if (mags.empty()) return;

    const int w = getWidth();
    const int h = getHeight();
    const int numBins = (int)mags.size();
    const float minY = (float)headerHeight + 10.0f;
    const float maxY = (float)h;

    g.setColour(juce::Colours::lightblue.withAlpha(0.9f));

    for (int i = 0; i < numBins; ++i)
    {
        float x0 = logX(i, numBins, (float)w, audioProcessor.getSampleRate());
        float x1 = logX(i + 1, numBins, (float)w, audioProcessor.getSampleRate());
        float binW = juce::jmax(1.0f, x1 - x0);

        const float mag = mags[(size_t)i];
        const float db = juce::Decibels::gainToDecibels(mag + 1e-12f);
        const float dbClamped = juce::jlimit(-100.0f, 0.0f, db);

        float y = juce::jmap(dbClamped, -100.0f, 0.0f, maxY, minY);

        g.fillRect(x0, y, binW, maxY - y);
    }
}

void YetAnotherAudioAnalyzerAudioProcessorEditor::drawFrequencyOverlay(juce::Graphics& g, int headerHeight)
{
    const int w = getWidth();
    const int h = getHeight();
    const float minY = (float)headerHeight;

    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(12.0f);

    // typical logarithmic frequencies for labeling
    const std::vector<float> freqs = { 20, 50, 100, 200, 500, 1e3, 2e3, 5e3, 10e3, 20e3 };

    for (auto f : freqs)
    {
        float logMin = std::log10(20.0f);
        float logMax = std::log10(audioProcessor.getSampleRate() / 2.0f);
        float x = ((std::log10(f) - logMin) / (logMax - logMin)) * w;

        g.drawLine(x, (float)headerHeight, x, (float)h, 1.0f); // vertical grid line
        g.drawText(juce::String((int)f), x - 10, 0, 40, headerHeight - 2, juce::Justification::centred);
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