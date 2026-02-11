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
constexpr int meterFooterHeight = 40;

//==============================================================================
YetAnotherAudioAnalyzerAudioProcessorEditor::YetAnotherAudioAnalyzerAudioProcessorEditor(YetAnotherAudioAnalyzerAudioProcessor& p) 
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize(1280, 720);


    addAndMakeVisible(spectrumTab);
    addAndMakeVisible(multibandCorrelationTab);
    addAndMakeVisible(stereoTab);
    addAndMakeVisible(lufsTab);

    spectrumTab.onClick = [this]() { setView(ViewMode::Spectrum); };
    multibandCorrelationTab.onClick = [this]() { setView(ViewMode::MultibandCorrelation); };
    stereoTab.onClick = [this]() { setView(ViewMode::StereoWidth); };
    lufsTab.onClick = [this]() { setView(ViewMode::AdvanceLufs); };

    addAndMakeVisible(monoButton);
    addAndMakeVisible(abButton);

    monoButton.onClick = [this]() { /* toggle mono processing */ };
    abButton.onClick = [this]() { /* trigger A/B switch */ };


    // Update GUI 30 times per second
    startTimerHz(60);
}


void YetAnotherAudioAnalyzerAudioProcessorEditor::setView(ViewMode newView)
{
    if (currentView == newView)
        return;

    currentView = newView;
    repaint();
}

void YetAnotherAudioAnalyzerAudioProcessorEditor::timerCallback()
{
    audioProcessor.getSpectrumAnalyzerL().updateSmoothedMagnitudes();
    audioProcessor.getSpectrumAnalyzerR().updateSmoothedMagnitudes();

    leftMagnitudes = audioProcessor.getSpectrumAnalyzerL().getMagnitudesCopy();
    rightMagnitudes = audioProcessor.getSpectrumAnalyzerR().getMagnitudesCopy();

    // LUFS / level
    float lufs = audioProcessor.getLevelMeter().hasIntegratedLufs() ?
        audioProcessor.getLevelMeter().getIntegratedLufs() :
        audioProcessor.getLevelMeter().getLastBlockLufs();

    levelValue = lufs;

    correlationValue = audioProcessor.getCorrelationMeter().getCorrelation();
    audioProcessor.getStereoWidthMeter().getResults(correlationValue, widthValue);
    
    // Load current RMS
    float rawLeft = audioProcessor.getLevelMeter().lastBlockRmsL.load();
    float rawRight = audioProcessor.getLevelMeter().lastBlockRmsR.load();

    // Smooth
    smoothedLeft += (rawLeft - smoothedLeft) * smoothingFactor;
    smoothedRight += (rawRight - smoothedRight) * smoothingFactor;

    // Update peak (hold)
    peakLeft = juce::jmax(peakLeft, smoothedLeft);
    peakRight = juce::jmax(peakRight, smoothedRight);

    // Decay peak slowly
    peakLeft = juce::jmax(peakLeft - peakDecay, smoothedLeft);
    peakRight = juce::jmax(peakRight - peakDecay, smoothedRight);

    repaint();
}

//==============================================================================
void YetAnotherAudioAnalyzerAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::grey);

    auto highlightTab = [this](juce::TextButton& b, ViewMode mode)
        {
            b.setColour(juce::TextButton::buttonColourId,currentView == mode ? juce::Colours::darkgrey
                : juce::Colours::black);
        };

    highlightTab(spectrumTab, ViewMode::Spectrum);
    highlightTab(multibandCorrelationTab, ViewMode::MultibandCorrelation);
    highlightTab(stereoTab, ViewMode::StereoWidth);
    highlightTab(lufsTab, ViewMode::AdvanceLufs);


    paintViewHeader(g);
    paintMainView(g);
    paintMeterFooter(g);

}

void YetAnotherAudioAnalyzerAudioProcessorEditor::paintViewHeader(juce::Graphics& g)
{
    // brack BG ONLY
    g.setColour(juce::Colours::darkgrey.darker(0.2f));
    g.fillRect(0, 0, getWidth(), viewHeaderHeight);

    g.setColour(juce::Colours::white);
    g.setFont(14.0f);

}

void YetAnotherAudioAnalyzerAudioProcessorEditor::paintMainView(juce::Graphics& g)
{
    juce::Graphics::ScopedSaveState state(g);
    g.reduceClipRegion(mainViewArea);

    switch (currentView)
    {
    case ViewMode::Spectrum:        paintSpectrumScreen(g, mainViewArea); break;
    case ViewMode::StereoWidth:     paintStereoWidthScreen(g, mainViewArea); break;
    case ViewMode::MultibandCorrelation: paintMultibandScreen(g, mainViewArea); break;
    case ViewMode::AdvanceLufs:     paintLufsScreen(g, mainViewArea); break;
    }
}

void YetAnotherAudioAnalyzerAudioProcessorEditor::paintMeterFooter(juce::Graphics& g)
{
    juce::Graphics::ScopedSaveState state(g);
    g.reduceClipRegion(meterFooterArea);

    // Background
    g.setColour(juce::Colours::darkgrey.darker(0.3f));
    g.fillRect(meterFooterArea);

    // Subtle top divider
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawLine((float)meterFooterArea.getX(),
        (float)meterFooterArea.getY(),
        (float)meterFooterArea.getRight(),
        (float)meterFooterArea.getY());

    // =============================
    // LEVEL METERS
    // =============================

    int lmX = levelMeterArea.getX();
    int lmY = levelMeterArea.getY();
    int lmW = levelMeterArea.getWidth();
    int lmH = levelMeterArea.getHeight();

    int leftFill = int(smoothedLeft * lmH);
    int rightFill = int(smoothedRight * lmH);

    leftFill = juce::jmax(leftFill, 2);
    rightFill = juce::jmax(rightFill, 2);

    // Background strip
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillRect(levelMeterArea);

    // Left
    g.setColour(juce::Colours::limegreen);
    g.fillRect(lmX,
        lmY + (lmH - leftFill),
        lmW / 2,
        leftFill);

    // Right
    g.setColour(juce::Colours::deepskyblue);
    g.fillRect(lmX + lmW / 2,
        lmY + (lmH - rightFill),
        lmW / 2,
        rightFill);

    // Peaks
    int peakLeftY = lmY + (lmH - int(peakLeft * lmH));
    int peakRightY = lmY + (lmH - int(peakRight * lmH));

    g.setColour(juce::Colours::yellow);
    g.fillRect(lmX, peakLeftY, lmW / 2, 2);
    g.fillRect(lmX + lmW / 2, peakRightY, lmW / 2, 2);

    // =============================
    // STEREO SECTION
    // =============================

    auto stereoArea = stereoFooterArea.reduced(10);

    // Vertical divider
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawLine((float)stereoFooterArea.getX(),
        (float)stereoFooterArea.getY(),
        (float)stereoFooterArea.getX(),
        (float)stereoFooterArea.getBottom());

    auto correlationArea = stereoArea.removeFromLeft(stereoArea.getWidth() / 2);
    auto widthArea = stereoArea;

    drawFooterCorrelation(g, correlationArea);
    drawFooterWidth(g, widthArea);
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

void YetAnotherAudioAnalyzerAudioProcessorEditor::paintStereoWidthScreen(juce::Graphics& g, juce::Rectangle<int> area)
{

}

void YetAnotherAudioAnalyzerAudioProcessorEditor::paintLufsScreen(juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour(juce::Colours::white);
    g.drawText("LUFS screen (WIP)",
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
        float xMin = plotArea.getX();
        float xMax = juce::jmax(plotArea.getRight() - 36.0f, xMin); // never less than min
        float labelX = juce::jlimit(xMin, xMax, x - 18.0f);

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
    auto bounds = getLocalBounds();

    // Header
    auto header = bounds.removeFromTop(viewHeaderHeight);
    int tabWidth = 120;
    
    spectrumTab.setBounds(header.removeFromLeft(tabWidth));
    multibandCorrelationTab.setBounds(header.removeFromLeft(tabWidth));
    stereoTab.setBounds(header.removeFromLeft(tabWidth));
    lufsTab.setBounds(header.removeFromLeft(tabWidth));

    // Footer
    meterFooterArea = bounds.removeFromBottom(meterFooterHeight);

    // Work on a copy for slicing
    auto footerLayout = meterFooterArea;

    // Rightmost: vertical level meter strip
    const int levelMeterWidth = 20;
    levelMeterArea = footerLayout.removeFromRight(levelMeterWidth);

    // Stereo section (left of level meter)
    const int stereoWidth = 240;
    stereoFooterArea = footerLayout.removeFromRight(stereoWidth);

    // Buttons on left side
    const int buttonWidth = 60;
    const int buttonHeight = 24;
    const int buttonSpacing = 10;

    monoButton.setBounds(
        footerLayout.getX() + 10,
        footerLayout.getCentreY() - buttonHeight / 2,
        buttonWidth,
        buttonHeight);

    abButton.setBounds(
        monoButton.getRight() + buttonSpacing,
        footerLayout.getCentreY() - buttonHeight / 2,
        buttonWidth,
        buttonHeight);
    


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

float YetAnotherAudioAnalyzerAudioProcessorEditor::xToFrequency(float xNorm) const
{
    const float minFreq = 20.0f;
    const float maxFreq = audioProcessor.getSampleRate() * 0.5f;

    float logMin = std::log10(minFreq);
    float logMax = std::log10(maxFreq);

    float logFreq = logMin + xNorm * (logMax - logMin);
    return std::pow(10.0f, logFreq);
}

float YetAnotherAudioAnalyzerAudioProcessorEditor::interpolateMagnitude(const std::vector<float>& mags, float freq,float sampleRate)
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

void YetAnotherAudioAnalyzerAudioProcessorEditor::drawFooterWidth(juce::Graphics& g, juce::Rectangle<int> area)
{

    g.setFont(12.0f);
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.drawText("WIDTH",
        area.removeFromLeft(45),
        juce::Justification::centredLeft);

    auto barArea = area.reduced(4);

    if (barArea.getWidth() <= 0 || barArea.getHeight() <= 0) {
        DBG("NOTHING");
        return; // nothing to draw
    }

    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillRoundedRectangle(barArea.toFloat(), 3.0f);

    float normalized = juce::jlimit(0.0f, 1.0f, widthValue);

    juce::Rectangle<float> fill = barArea.toFloat();
    fill.setWidth(fill.getWidth() * normalized);

    juce::Colour colour = juce::Colours::deepskyblue;

    if (widthValue < 0.2f)
        colour = juce::Colours::red;

    g.setColour(colour);
    g.fillRoundedRectangle(fill, 3.0f);
}


void YetAnotherAudioAnalyzerAudioProcessorEditor::drawFooterCorrelation(juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setFont(12.0f);
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.drawText("CORR", area.removeFromLeft(45), juce::Justification::centredLeft);

    auto barArea = area.reduced(4);

    // Background
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillRoundedRectangle(barArea.toFloat(), 3.0f);

    float centreX = (float)barArea.getCentreX();
    float halfWidth = barArea.getWidth() * 0.5f;

    float clamped = juce::jlimit(-1.0f, 1.0f, correlationValue);

    float fillWidth = halfWidth * std::abs(clamped);

    juce::Rectangle<float> fillRect;

    if (clamped >= 0.0f)
        fillRect = { centreX, (float)barArea.getY(), fillWidth, (float)barArea.getHeight() };
    else
        fillRect = { centreX - fillWidth, (float)barArea.getY(), fillWidth, (float)barArea.getHeight() };

    // Colour logic
    juce::Colour colour = juce::Colours::limegreen;

    if (clamped < 0.0f)
        colour = juce::Colours::red;
    else if (clamped < 0.3f)
        colour = juce::Colours::orange;

    g.setColour(colour);
    g.fillRoundedRectangle(fillRect, 3.0f);

    // Center line
    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.drawVerticalLine((int)centreX,
        (float)barArea.getY(),
        (float)barArea.getBottom());
}



