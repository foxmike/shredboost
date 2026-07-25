#include "SpectrumOverlay.h"
#include <algorithm>
#include <cmath>

namespace ShredBoost
{

//==============================================================================
void SpectrumOverlay::Analyser::processFromFIFO (SpectrumFIFO* fifo) noexcept
{
    if (fifo == nullptr)
        return;

    alignas (16) float tempBuffer[8192];
    const int numReady = fifo->getNumReady();
    if (numReady <= 0)
        return;

    const int numRead = fifo->pull (tempBuffer, std::min (numReady, 8192));

    for (int i = 0; i < numRead; ++i)
    {
        circularBuffer[static_cast<size_t> (writePointer)] = tempBuffer[i];
        writePointer = (writePointer + 1) % 512;

        if (++samplesSinceLastFFT >= hopSize)
        {
            performFFT();
            samplesSinceLastFFT = 0;
        }
    }
}

void SpectrumOverlay::Analyser::performFFT() noexcept
{
    alignas (16) float fftData[1024];
    std::fill (std::begin (fftData), std::end (fftData), 0.0f);

    for (int i = 0; i < 512; ++i)
    {
        const int readIdx = (writePointer + i) % 512;
        fftData[static_cast<size_t> (i)] = circularBuffer[static_cast<size_t> (readIdx)] * welchWindow[static_cast<size_t> (i)];
    }

    fft.performFrequencyOnlyForwardTransform (fftData, true);

    const float attackCoeff = 0.4f;
    const float releaseCoeff = 0.08f;

    for (int k = 0; k <= 256; ++k)
    {
        const float newMag = fftData[static_cast<size_t> (k)] * fftNormalization;
        const float oldMag = averagedSpectrum[static_cast<size_t> (k)];

        if (newMag > oldMag)
            averagedSpectrum[static_cast<size_t> (k)] = oldMag + attackCoeff * (newMag - oldMag);
        else
            averagedSpectrum[static_cast<size_t> (k)] = oldMag + releaseCoeff * (newMag - oldMag);
    }
}

void SpectrumOverlay::Analyser::reset() noexcept
{
    circularBuffer.fill (0.0f);
    averagedSpectrum.fill (0.0f);
    writePointer = 0;
    samplesSinceLastFFT = 0;
}

//==============================================================================
SpectrumOverlay::SpectrumOverlay() noexcept
{
    startTimerHz (60);
}

SpectrumOverlay::~SpectrumOverlay()
{
    stopTimer();
}

void SpectrumOverlay::setFIFOs (SpectrumFIFO* preDriveFIFO, SpectrumFIFO* postDriveFIFO) noexcept
{
    preFIFO = preDriveFIFO;
    postFIFO = postDriveFIFO;
}

void SpectrumOverlay::setSampleRate (double newSampleRate) noexcept
{
    sampleRate = std::max (1.0, newSampleRate);
}

void SpectrumOverlay::timerCallback()
{
    if (preFIFO != nullptr)
        preAnalyser.processFromFIFO (preFIFO);

    if (postFIFO != nullptr)
        postAnalyser.processFromFIFO (postFIFO);

    repaint();
}

float SpectrumOverlay::getInterpolatedMagnitude (const std::array<float, 257>& spectrum, float binIndex) const noexcept
{
    const float clampedBin = std::clamp (binIndex, 0.0f, 256.0f);
    const int indexLower = static_cast<int> (clampedBin);
    const int indexUpper = std::min (indexLower + 1, 256);
    const float fraction = clampedBin - static_cast<float> (indexLower);

    const float lowerVal = spectrum[static_cast<size_t> (indexLower)];
    const float upperVal = spectrum[static_cast<size_t> (indexUpper)];

    return (1.0f - fraction) * lowerVal + fraction * upperVal;
}

float SpectrumOverlay::magnitudeToY (float mag, const juce::Rectangle<float>& plotBounds) const noexcept
{
    const float db = 20.0f * std::log10 (std::max (mag, 1e-5f));
    const float clampedDb = std::clamp (db, minDb, maxDb);
    const float proportion = (clampedDb - minDb) / (maxDb - minDb);
    return plotBounds.getBottom() - plotBounds.getHeight() * proportion;
}

void SpectrumOverlay::paint (juce::Graphics& g)
{
    g.fillAll (backgroundColour);

    const auto bounds = getLocalBounds().toFloat();
    const auto plotBounds = bounds.reduced (15.0f, 15.0f);

    if (plotBounds.isEmpty())
        return;

    // 1. Draw Grid
    g.setColour (gridColour);
    const float dashLengths[2] = { 2.0f, 3.0f };

    // Frequencies grid
    const std::vector<float> freqs = { 20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f };
    const std::vector<juce::String> freqLabels = { "20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k" };

    for (size_t i = 0; i < freqs.size(); ++i)
    {
        const float f = freqs[i];
        const float proportion = (std::log10 (f) - std::log10 (minFreq)) / (std::log10 (maxFreq) - std::log10 (minFreq));
        const float x = plotBounds.getX() + plotBounds.getWidth() * proportion;

        g.drawDashedLine (juce::Line<float> (x, plotBounds.getY(), x, plotBounds.getBottom()), dashLengths, 2, 1.0f);

        g.setColour (gridColour.withAlpha (0.4f));
        g.setFont (10.0f);
        g.drawText (freqLabels[i], static_cast<int> (x - 20.0f), static_cast<int> (plotBounds.getBottom() + 2.0f), 40, 12, juce::Justification::centredTop);
        g.setColour (gridColour);
    }

    // dB grid
    const std::vector<float> dbs = { -60.0f, -48.0f, -36.0f, -24.0f, -12.0f, 0.0f };

    for (float db : dbs)
    {
        const float proportion = (db - minDb) / (maxDb - minDb);
        const float y = plotBounds.getBottom() - plotBounds.getHeight() * proportion;

        g.drawDashedLine (juce::Line<float> (plotBounds.getX(), y, plotBounds.getRight(), y), dashLengths, 2, 1.0f);

        g.setColour (gridColour.withAlpha (0.4f));
        g.setFont (10.0f);
        g.drawText (juce::String (static_cast<int> (db)) + " dB",
                    static_cast<int> (plotBounds.getX() + 5.0f),
                    static_cast<int> (y - 12.0f),
                    50, 12, juce::Justification::left);
        g.setColour (gridColour);
    }

    // 2. Compute Paths
    juce::Path prePath;
    juce::Path postPath;

    bool preStarted = false;
    bool postStarted = false;

    const float plotLeft = plotBounds.getX();
    const float plotWidth = plotBounds.getWidth();

    for (int x = static_cast<int> (plotLeft); x <= static_cast<int> (plotBounds.getRight()); ++x)
    {
        const float proportion = static_cast<float> (x - plotLeft) / plotWidth;
        const float freq = minFreq * std::pow (maxFreq / minFreq, proportion);
        const float binIndex = freq * 512.0f / static_cast<float> (sampleRate);

        // Pre-drive
        const float preMag = getInterpolatedMagnitude (preAnalyser.averagedSpectrum, binIndex);
        const float preY = magnitudeToY (preMag, plotBounds);

        if (! preStarted)
        {
            prePath.startNewSubPath (static_cast<float> (x), preY);
            preStarted = true;
        }
        else
        {
            prePath.lineTo (static_cast<float> (x), preY);
        }

        // Post-drive
        const float postMag = getInterpolatedMagnitude (postAnalyser.averagedSpectrum, binIndex);
        const float postY = magnitudeToY (postMag, plotBounds);

        if (! postStarted)
        {
            postPath.startNewSubPath (static_cast<float> (x), postY);
            postStarted = true;
        }
        else
        {
            postPath.lineTo (static_cast<float> (x), postY);
        }
    }

    // 3. Fill under Paths (semi-transparent gradients)
    if (! prePath.isEmpty())
    {
        juce::Path preFillPath = prePath;
        preFillPath.lineTo (plotBounds.getRight(), plotBounds.getBottom());
        preFillPath.lineTo (plotBounds.getX(), plotBounds.getBottom());
        preFillPath.closeSubPath();

        juce::ColourGradient gradient (
            preColour.withAlpha (0.15f), 0.0f, plotBounds.getY(),
            preColour.withAlpha (0.0f), 0.0f, plotBounds.getBottom(),
            false);
        g.setGradientFill (gradient);
        g.fillPath (preFillPath);
    }

    if (! postPath.isEmpty())
    {
        juce::Path postFillPath = postPath;
        postFillPath.lineTo (plotBounds.getRight(), plotBounds.getBottom());
        postFillPath.lineTo (plotBounds.getX(), plotBounds.getBottom());
        postFillPath.closeSubPath();

        juce::ColourGradient gradient (
            postColour.withAlpha (0.15f), 0.0f, plotBounds.getY(),
            postColour.withAlpha (0.0f), 0.0f, plotBounds.getBottom(),
            false);
        g.setGradientFill (gradient);
        g.fillPath (postFillPath);
    }

    // 4. Draw Neon-Glowing Lines
    if (! prePath.isEmpty())
    {
        // Glow outline
        g.setColour (preColour.withAlpha (0.15f));
        g.strokePath (prePath, juce::PathStrokeType (3.0f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
        // Core line
        g.setColour (preColour);
        g.strokePath (prePath, juce::PathStrokeType (1.5f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
    }

    if (! postPath.isEmpty())
    {
        // Glow outline
        g.setColour (postColour.withAlpha (0.15f));
        g.strokePath (postPath, juce::PathStrokeType (3.0f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
        // Core line
        g.setColour (postColour);
        g.strokePath (postPath, juce::PathStrokeType (1.5f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
    }
}

void SpectrumOverlay::resized()
{
}

} // namespace ShredBoost
