/*
  ==============================================================================

    Rendering workflow

    paint()
     ├── paintViewHeader()
     ├── paintMainView()
     │    ├── paintSpectrumScreen() OR paintMultibandScreen()
     │    └── drawFrequencyOverlay()
     └── paintMeterFooter()

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

constexpr int viewHeaderHeight = 40;
constexpr int meterFooterHeight = 60;

//==============================================================================
YetAnotherAudioAnalyzerAudioProcessorEditor::YetAnotherAudioAnalyzerAudioProcessorEditor(YetAnotherAudioAnalyzerAudioProcessor& p) 
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize(1280, 720);


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

    paintViewHeader(g);
    paintMainView(g);
    paintMeterFooter(g);

}

void YetAnotherAudioAnalyzerAudioProcessorEditor::paintViewHeader(juce::Graphics& g)
{
    g.setColour(juce::Colours::darkgrey.darker(0.2f));
    g.fillRect(0, 0, getWidth(), viewHeaderHeight);

    g.setColour(juce::Colours::white);
    g.setFont(14.0f);

    g.drawText("View:",
        10, 0, 60, viewHeaderHeight,
        juce::Justification::centredLeft);
}

void YetAnotherAudioAnalyzerAudioProcessorEditor::paintMainView(juce::Graphics& g)
{
    auto area = juce::Rectangle<int>(
        0,
        viewHeaderHeight,
        getWidth(),
        getHeight() - viewHeaderHeight - meterFooterHeight);

    juce::Graphics::ScopedSaveState state(g); // doesn't affect other ddrawings
    g.reduceClipRegion(area);

    if (currentView == ViewMode::Spectrum)
        paintSpectrumScreen(g, area);
    else
        paintMultibandScreen(g, area);
}

void YetAnotherAudioAnalyzerAudioProcessorEditor::paintMeterFooter(juce::Graphics& g)
{
    auto area = juce::Rectangle<int>(
        0,
        getHeight() - meterFooterHeight,
        getWidth(),
        meterFooterHeight);

    g.setColour(juce::Colours::darkgrey.darker(0.4f));
    g.fillRect(area);

    g.setColour(juce::Colours::white);
    g.setFont(14.0f);

    g.drawText("LUFS: " + juce::String(levelValue, 2),
        area.removeFromLeft(120),
        juce::Justification::centred);

    g.drawText("Corr: " + juce::String(correlationValue, 2),
        area.removeFromLeft(120),
        juce::Justification::centred);

    g.drawText("Width: " + juce::String(widthValue, 2),
        area.removeFromLeft(120),
        juce::Justification::centred);
}

void YetAnotherAudioAnalyzerAudioProcessorEditor::paintSpectrumScreen(
    juce::Graphics& g, juce::Rectangle<int> area)
{
    const auto& magsL = leftMagnitudes;
    const auto& magsR = rightMagnitudes;
    if (magsL.empty() || magsR.empty())
        return;

    juce::Path spectrumPath;
    spectrumPath.preallocateSpace(area.getWidth() * 3);

    const float minDb = -60.0f;
    const float maxDb = 0.0f;
    const float refAmplitude = 1.0f;
    const int numBins = (int)magsL.size();
    const float nyquist = audioProcessor.getSampleRate() * 0.5f;

    // SPAN-style smoothing per pixel
    static std::vector<float> smoothed(area.getWidth(), 0.0f);
    const float attack = 0.6f;     // fast attack
    const float releaseLow = 0.02f; // low freq decay
    const float releaseHigh = 0.25f; // high freq decay

    const float logMin = std::log10(20.0f);
    const float logMax = std::log10(nyquist);

    // Compute peak for adaptive scaling
    float globalPeak = 0.0f;

    for (int x = 0; x < area.getWidth(); ++x)
    {
        float xNorm = (float)x / (float)(area.getWidth() - 1);
        float logFreq = logMin + xNorm * (logMax - logMin);
        float freq = std::pow(10.0f, logFreq);

        // fractional bin index
        float binFloat = freq / nyquist * (numBins - 1);
        int bin0 = (int)std::floor(binFloat);
        int bin1 = juce::jmin(bin0 + 1, numBins - 1);
        float frac = binFloat - bin0;

        // Stereo-averaged magnitude
        float mag = 0.5f * (magsL[bin0] + magsR[bin0]) * (1.0f - frac)
            + 0.5f * (magsL[bin1] + magsR[bin1]) * frac;

        // Optional low-frequency slope (20–200 Hz)
        if (freq < 200.0f)
            mag *= 0.6f + 0.4f * (freq / 200.0f);

        globalPeak = juce::jmax(globalPeak, mag);

        // Store temporarily for smoothing
        smoothed[x] = mag;
    }

    // Optional: adaptive scaling for bass-heavy peaks
    float scale = juce::jmax(1.0f, globalPeak);
    for (int x = 0; x < area.getWidth(); ++x)
        smoothed[x] /= scale;

    // Apply dynamic smoothing per pixel and map to dB
    for (int x = 0; x < area.getWidth(); ++x)
    {
        float freqRatio = (float)x / (float)area.getWidth();
        float release = juce::jmap(freqRatio, 0.0f, 1.0f, releaseLow, releaseHigh);

        // Dynamic smoothing
        if (smoothed[x] > smoothed[x])
            smoothed[x] = attack * smoothed[x] + (1.0f - attack) * smoothed[x];
        else
            smoothed[x] = release * smoothed[x] + (1.0f - release) * smoothed[x];

        // Clamp linear magnitude
        float mag = juce::jmin(smoothed[x], 1.0f);

        // Convert to dB
        float db = juce::Decibels::gainToDecibels(mag / refAmplitude);
        db = juce::jlimit(minDb, maxDb, db);

        // Map to vertical pixel
        float y = juce::jmap(db, minDb, maxDb, (float)area.getBottom(), (float)area.getY());
        y = juce::jlimit((float)area.getY(), (float)area.getBottom(), y);

        if (x == 0)
            spectrumPath.startNewSubPath(area.getX(), y);
        else
            spectrumPath.lineTo(area.getX() + x, y);
    }

    // Draw spectrum
    g.setColour(juce::Colours::lightblue);
    g.strokePath(spectrumPath, juce::PathStrokeType(1.5f));

    // Draw frequency overlay & grid
    drawFrequencyOverlay(g, area);
}

void YetAnotherAudioAnalyzerAudioProcessorEditor::paintMultibandScreen(juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour(juce::Colours::white);
    g.drawText("Multiband correlation screen (WIP)",
        area.reduced(20),
        juce::Justification::centredLeft);
}

void YetAnotherAudioAnalyzerAudioProcessorEditor::drawFrequencyOverlay(juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(12.0f);

    const std::vector<float> freqs = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };

    float logMin = std::log10(20.0f);
    float logMax = std::log10(audioProcessor.getSampleRate() / 2.0f);

    // Allocate a left bar for dB scale
    const int dbBarWidth = 40; // adjust as needed
    juce::Rectangle<float> plotArea = area.toFloat();
    plotArea.removeFromLeft((float)dbBarWidth); // spectrum area reduced

    // --- Horizontal dB lines & labels (in the dbBar area) ---
    const int marginYTop = 4;
    const int marginYBottom = 20;
    plotArea.setY(plotArea.getY() + marginYTop);
    plotArea.setHeight(plotArea.getHeight() - marginYTop - marginYBottom);

    for (float db = 0.0f; db >= minDb; db -= 10.0f)
    {
        float y = juce::jmap(db, minDb, maxDb, plotArea.getBottom(), plotArea.getY());
        g.drawHorizontalLine((int)y, plotArea.getX(), plotArea.getRight());
        g.drawText(juce::String((int)db),
            area.getX() + 2,    // inside left bar
            (int)(y - 8),
            dbBarWidth - 4,     // label width inside bar
            16,
            juce::Justification::right);
    }

    // --- Vertical frequency lines & labels ---
    for (float f : freqs)
    {
        float xNorm = (std::log10(f) - logMin) / (logMax - logMin);
        float x = plotArea.getX() + xNorm * plotArea.getWidth();
        float labelX = juce::jlimit(plotArea.getX(), plotArea.getRight() - 36.0f, x - 18.0f);

        g.drawLine(x, plotArea.getY(), x, plotArea.getBottom(), 1.0f);

        g.drawText(juce::String((int)f),
            (int)labelX,
            (int)(plotArea.getBottom() + 2), // below spectrum
            36, 14,
            juce::Justification::centred);
    }
}


void YetAnotherAudioAnalyzerAudioProcessorEditor::resized()
{
    const int headerHeight = 50;

    viewSwitchButton.setBounds(10, 10, 150, 30);
}

float YetAnotherAudioAnalyzerAudioProcessorEditor::logX(int bin, int numBins, float width, float sampleRate)
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

float YetAnotherAudioAnalyzerAudioProcessorEditor::xToFrequency(
    float xNorm) const
{
    const float minFreq = 20.0f;
    const float maxFreq = audioProcessor.getSampleRate() * 0.5f;

    float logMin = std::log10(minFreq);
    float logMax = std::log10(maxFreq);

    float logFreq = logMin + xNorm * (logMax - logMin);
    return std::pow(10.0f, logFreq);
}

float YetAnotherAudioAnalyzerAudioProcessorEditor::interpolateMagnitude(
    const std::vector<float>& mags,
    float freq,
    float sampleRate)
{
    if (mags.empty()) return 0.0f;

    const float nyquist = sampleRate * 0.5f;
    freq = juce::jlimit(0.0f, nyquist, freq);

    float binFloat = (freq / nyquist) * (float)(mags.size() - 1); // -1 because last bin index = size-1
    int bin0 = juce::jlimit(0, (int)mags.size() - 1, (int)std::floor(binFloat));
    int bin1 = juce::jmin(bin0 + 1, (int)mags.size() - 1);

    float frac = binFloat - bin0;
    return juce::jmap(frac, mags[bin0], mags[bin1]);
}