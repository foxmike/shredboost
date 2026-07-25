#include "TransferCurveScope.h"
#include <algorithm>
#include <cmath>

namespace ShredBoost
{

TransferCurveScope::TransferCurveScope() noexcept
{
    startTimerHz (60);
}

TransferCurveScope::~TransferCurveScope()
{
    stopTimer();
}

void TransferCurveScope::pushInputSample (float sample) noexcept
{
    instantaneousAmplitude.store (sample, std::memory_order_relaxed);
}

void TransferCurveScope::setVoicingMode (int mode) noexcept
{
    const int clampedMode = std::clamp (mode, 0, 3);
    for (int i = 0; i < 4; ++i)
    {
        targetWeights[static_cast<std::size_t> (i)] = (i == clampedMode) ? 1.0f : 0.0f;
    }
}

void TransferCurveScope::setTelemetrySources (const std::atomic<float>* amplitudeSource,
                                              const std::atomic<int>* voicingModeSource) noexcept
{
    externalAmplitudeSource = amplitudeSource;
    externalVoicingModeSource = voicingModeSource;
}

void TransferCurveScope::setCurveColour (juce::Colour newColour) noexcept
{
    curveColour = newColour;
}

void TransferCurveScope::setDotColour (juce::Colour newColour) noexcept
{
    dotColour = newColour;
}

void TransferCurveScope::setGridColour (juce::Colour newColour) noexcept
{
    gridColour = newColour;
}

void TransferCurveScope::timerCallback()
{
    if (externalAmplitudeSource != nullptr)
    {
        instantaneousAmplitude.store (externalAmplitudeSource->load (std::memory_order_relaxed),
                                      std::memory_order_relaxed);
    }

    if (externalVoicingModeSource != nullptr)
    {
        setVoicingMode (externalVoicingModeSource->load (std::memory_order_relaxed));
    }

    // Smoothly update instantaneous amplitude for the dot position
    const float targetAmp = instantaneousAmplitude.load (std::memory_order_relaxed);
    
    // Low-pass filter to smooth out rapid jumping, but fast enough to feel responsive.
    // If targetAmp is a raw audio signal, this acts as a fast-attack/fast-release tracking filter.
    // If it's a peak level, it will glide nicely.
    const float smoothFactor = 0.25f;
    smoothedAmplitude += (targetAmp - smoothedAmplitude) * smoothFactor;

    // Smoothly update weights for morphing
    const float morphSpeed = 0.15f; // transition takes ~15 frames (~250ms)
    for (int i = 0; i < 4; ++i)
    {
        const auto idx = static_cast<std::size_t> (i);
        currentWeights[idx] += (targetWeights[idx] - currentWeights[idx]) * morphSpeed;
    }

    repaint();
}

void TransferCurveScope::paint (juce::Graphics& g)
{
    // Sleek background
    g.fillAll (juce::Colour (0xff12161f));

    const auto bounds = getLocalBounds().toFloat();
    const auto plotBounds = bounds.reduced (15.0f);

    if (plotBounds.isEmpty())
        return;

    const float centerX = plotBounds.getCentreX();
    const float centerY = plotBounds.getCentreY();

    // 1. Draw Grid Lines
    g.setColour (gridColour);
    
    // Draw major axes (X = 0, Y = 0)
    g.drawHorizontalLine (static_cast<int> (centerY), plotBounds.getX(), plotBounds.getRight());
    g.drawVerticalLine (static_cast<int> (centerX), plotBounds.getY(), plotBounds.getBottom());

    // Grid ticks/lines at -1.0 and 1.0 (both X and Y)
    // Mapping [-2.0, 2.0] to plot bounds
    const float w = plotBounds.getWidth();
    const float h = plotBounds.getHeight();
    const float xLeft = plotBounds.getX();
    const float yTop = plotBounds.getY();
    const float yBottom = plotBounds.getBottom();

    // v = -1.0 is at 25% of width, v = 1.0 is at 75% of width
    const float x1 = xLeft + w * 0.25f;
    const float x2 = xLeft + w * 0.75f;
    // y = -1.0 is at 75% of height (from top), y = 1.0 is at 25% of height
    const float y1 = yTop + h * 0.25f;
    const float y2 = yTop + h * 0.75f;

    const float dashLengths[2] = { 4.0f, 4.0f };
    
    // Vertical dashed lines for x = -1.0, 1.0
    g.drawDashedLine (juce::Line<float> (x1, yTop, x1, yBottom), dashLengths, 2, 1.0f);
    g.drawDashedLine (juce::Line<float> (x2, yTop, x2, yBottom), dashLengths, 2, 1.0f);

    // Horizontal dashed lines for y = -1.0, 1.0
    g.drawDashedLine (juce::Line<float> (xLeft, y1, plotBounds.getRight(), y1), dashLengths, 2, 1.0f);
    g.drawDashedLine (juce::Line<float> (xLeft, y2, plotBounds.getRight(), y2), dashLengths, 2, 1.0f);

    // Draw grid labels (-1, +1)
    g.setColour (gridColour.withAlpha (0.4f));
    g.setFont (10.0f);
    g.drawText ("-1.0", static_cast<int> (x1 - 25.0f), static_cast<int> (centerY + 2.0f), 50, 12, juce::Justification::centredTop);
    g.drawText ("1.0", static_cast<int> (x2 - 25.0f), static_cast<int> (centerY + 2.0f), 50, 12, juce::Justification::centredTop);
    g.drawText ("1.0", static_cast<int> (centerX + 5.0f), static_cast<int> (y1 - 6.0f), 30, 12, juce::Justification::left);
    g.drawText ("-1.0", static_cast<int> (centerX + 5.0f), static_cast<int> (y2 - 6.0f), 30, 12, juce::Justification::left);

    // 2. Compute and Draw the Morphed Curve Path
    float sumWeights = 0.0f;
    for (float wWeight : currentWeights)
        sumWeights += wWeight;

    std::array<float, 4> normWeights { 0.0f, 0.0f, 0.0f, 0.0f };
    if (sumWeights > 0.0f)
    {
        for (int i = 0; i < 4; ++i)
            normWeights[static_cast<std::size_t> (i)] = currentWeights[static_cast<std::size_t> (i)] / sumWeights;
    }
    else
    {
        normWeights[0] = 1.0f;
    }

    juce::Path curvePath;
    constexpr int numPoints = 128;
    
    for (int i = 0; i <= numPoints; ++i)
    {
        const float progress = static_cast<float> (i) / static_cast<float> (numPoints);
        const float v = -2.0f + 4.0f * progress;

        float y = 0.0f;
        for (int m = 0; m < 4; ++m)
        {
            y += normWeights[static_cast<std::size_t> (m)] * applyTopology (v, m);
        }

        // Map domain v in [-2.0, 2.0] to X screen coords
        const float xScreen = xLeft + w * progress;
        // Map range y in [-2.0, 2.0] to Y screen coords (y=2.0 is yTop, y=-2.0 is yBottom)
        const float yScreen = yBottom - h * (y - (-2.0f)) / 4.0f;

        if (i == 0)
            curvePath.startNewSubPath (xScreen, yScreen);
        else
            curvePath.lineTo (xScreen, yScreen);
    }

    // Outer glow for the curve
    g.setColour (curveColour.withAlpha (0.15f));
    g.strokePath (curvePath, juce::PathStrokeType (4.0f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));

    // Inner bright path
    g.setColour (curveColour);
    g.strokePath (curvePath, juce::PathStrokeType (1.5f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));

    // 3. Draw Moving Dot representing Instantaneous Input Amplitude
    // Clamp the input amplitude to plotted range [-2.0, 2.0]
    const float dotV = std::clamp (smoothedAmplitude, -2.0f, 2.0f);
    
    // Evaluate the same morphed curve at the dot's input position
    float dotY = 0.0f;
    for (int m = 0; m < 4; ++m)
    {
        dotY += normWeights[static_cast<std::size_t> (m)] * applyTopology (dotV, m);
    }

    // Map to screen coordinates
    const float dotXScreen = xLeft + w * (dotV - (-2.0f)) / 4.0f;
    const float dotYScreen = yBottom - h * (dotY - (-2.0f)) / 4.0f;

    // Outer glowing ring
    g.setColour (dotColour.withAlpha (0.25f));
    g.fillEllipse (dotXScreen - 7.0f, dotYScreen - 7.0f, 14.0f, 14.0f);

    // Inner solid dot
    g.setColour (dotColour);
    g.fillEllipse (dotXScreen - 3.5f, dotYScreen - 3.5f, 7.0f, 7.0f);
}

void TransferCurveScope::resized()
{
}

float TransferCurveScope::applyTopology (float input, int mode) noexcept
{
    switch (mode)
    {
        case 0:
            return std::atan (input)
                + 0.15f * input * input * std::exp (-std::abs (input));

        case 1:
            return std::tanh (input);

        case 2:
            return std::tanh ((input >= 0.0f ? 1.0f : 1.8f) * input);

        case 3:
            return input;

        default:
            return input;
    }
}

} // namespace ShredBoost
