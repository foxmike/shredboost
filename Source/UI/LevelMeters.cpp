#include "LevelMeters.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <cmath>
#include <vector>

namespace ShredBoost
{

LevelMeters::LevelMeters() noexcept
{
    startTimerHz (60);
}

LevelMeters::~LevelMeters()
{
    stopTimer();
}

void LevelMeters::setTelemetrySources (const std::atomic<float>* inputPeakLeft,
                                       const std::atomic<float>* inputPeakRight,
                                       const std::atomic<float>* inputRmsLeft,
                                       const std::atomic<float>* inputRmsRight,
                                       const std::atomic<float>* outputPeakLeft,
                                       const std::atomic<float>* outputPeakRight,
                                       const std::atomic<float>* outputRmsLeft,
                                       const std::atomic<float>* outputRmsRight,
                                       const std::atomic<float>* peakReductionDb) noexcept
{
    externalInputPeakLeft = inputPeakLeft;
    externalInputPeakRight = inputPeakRight;
    externalInputRmsLeft = inputRmsLeft;
    externalInputRmsRight = inputRmsRight;
    externalOutputPeakLeft = outputPeakLeft;
    externalOutputPeakRight = outputPeakRight;
    externalOutputRmsLeft = outputRmsLeft;
    externalOutputRmsRight = outputRmsRight;
    externalPeakReductionDb = peakReductionDb;
}

void LevelMeters::timerCallback()
{
    // Update input L peak
    const float inPeakL = externalInputPeakLeft != nullptr ? externalInputPeakLeft->load (std::memory_order_relaxed) : 0.0f;
    const float inPeakDbL = juce::Decibels::gainToDecibels (inPeakL, -100.0f);
    if (inPeakDbL > displayedInputPeakLeft)
    {
        displayedInputPeakLeft = inPeakDbL;
        inputPeakHoldLeft = peakHoldFrames;
    }
    else if (inputPeakHoldLeft > 0)
    {
        --inputPeakHoldLeft;
    }
    else
    {
        displayedInputPeakLeft = std::max (-100.0f, displayedInputPeakLeft - peakDecayRate);
    }

    // Update input R peak
    const float inPeakR = externalInputPeakRight != nullptr ? externalInputPeakRight->load (std::memory_order_relaxed) : 0.0f;
    const float inPeakDbR = juce::Decibels::gainToDecibels (inPeakR, -100.0f);
    if (inPeakDbR > displayedInputPeakRight)
    {
        displayedInputPeakRight = inPeakDbR;
        inputPeakHoldRight = peakHoldFrames;
    }
    else if (inputPeakHoldRight > 0)
    {
        --inputPeakHoldRight;
    }
    else
    {
        displayedInputPeakRight = std::max (-100.0f, displayedInputPeakRight - peakDecayRate);
    }

    // Update input L RMS
    const float inRmsL = externalInputRmsLeft != nullptr ? externalInputRmsLeft->load (std::memory_order_relaxed) : 0.0f;
    const float inRmsDbL = juce::Decibels::gainToDecibels (inRmsL, -100.0f);
    displayedInputRmsLeft += (inRmsDbL - displayedInputRmsLeft) * rmsSmoothFactor;

    // Update input R RMS
    const float inRmsR = externalInputRmsRight != nullptr ? externalInputRmsRight->load (std::memory_order_relaxed) : 0.0f;
    const float inRmsDbR = juce::Decibels::gainToDecibels (inRmsR, -100.0f);
    displayedInputRmsRight += (inRmsDbR - displayedInputRmsRight) * rmsSmoothFactor;

    // Update output L peak
    const float outPeakL = externalOutputPeakLeft != nullptr ? externalOutputPeakLeft->load (std::memory_order_relaxed) : 0.0f;
    const float outPeakDbL = juce::Decibels::gainToDecibels (outPeakL, -100.0f);
    if (outPeakDbL > displayedOutputPeakLeft)
    {
        displayedOutputPeakLeft = outPeakDbL;
        outputPeakHoldLeft = peakHoldFrames;
    }
    else if (outputPeakHoldLeft > 0)
    {
        --outputPeakHoldLeft;
    }
    else
    {
        displayedOutputPeakLeft = std::max (-100.0f, displayedOutputPeakLeft - peakDecayRate);
    }

    // Update output R peak
    const float outPeakR = externalOutputPeakRight != nullptr ? externalOutputPeakRight->load (std::memory_order_relaxed) : 0.0f;
    const float outPeakDbR = juce::Decibels::gainToDecibels (outPeakR, -100.0f);
    if (outPeakDbR > displayedOutputPeakRight)
    {
        displayedOutputPeakRight = outPeakDbR;
        outputPeakHoldRight = peakHoldFrames;
    }
    else if (outputPeakHoldRight > 0)
    {
        --outputPeakHoldRight;
    }
    else
    {
        displayedOutputPeakRight = std::max (-100.0f, displayedOutputPeakRight - peakDecayRate);
    }

    // Update output L RMS
    const float outRmsL = externalOutputRmsLeft != nullptr ? externalOutputRmsLeft->load (std::memory_order_relaxed) : 0.0f;
    const float outRmsDbL = juce::Decibels::gainToDecibels (outRmsL, -100.0f);
    displayedOutputRmsLeft += (outRmsDbL - displayedOutputRmsLeft) * rmsSmoothFactor;

    // Update output R RMS
    const float outRmsR = externalOutputRmsRight != nullptr ? externalOutputRmsRight->load (std::memory_order_relaxed) : 0.0f;
    const float outRmsDbR = juce::Decibels::gainToDecibels (outRmsR, -100.0f);
    displayedOutputRmsRight += (outRmsDbR - displayedOutputRmsRight) * rmsSmoothFactor;

    // Update peak reduction
    const float redDb = externalPeakReductionDb != nullptr ? externalPeakReductionDb->load (std::memory_order_relaxed) : 0.0f;
    if (redDb > displayedReductionDb)
    {
        displayedReductionDb = redDb;
        reductionHold = reductionHoldFrames;
    }
    else if (reductionHold > 0)
    {
        --reductionHold;
    }
    else
    {
        displayedReductionDb = std::max (0.0f, displayedReductionDb - reductionDecayRate);
    }

    repaint();
}

void LevelMeters::paint (juce::Graphics& g)
{
    // Professional background
    g.fillAll (juce::Colour (0xff12151a));

    auto bounds = getLocalBounds().toFloat().reduced (8.0f);
    if (bounds.isEmpty())
        return;

    // Split bounds into three columns
    const float w = bounds.getWidth();
    const float inColW = w * 0.38f;
    const float outColW = w * 0.38f;
    const float satColW = w - (inColW + outColW);

    auto inputBounds = bounds.withWidth (inColW);
    auto satBounds = bounds.withWidth (satColW).withX (bounds.getX() + inColW);
    auto outputBounds = bounds.withWidth (outColW).withX (bounds.getX() + inColW + satColW);

    drawMeterGroup (g, inputBounds, displayedInputRmsLeft, displayedInputPeakLeft, 
                    displayedInputRmsRight, displayedInputPeakRight, "INPUT");

    drawSaturationGroup (g, satBounds, displayedReductionDb);

    drawMeterGroup (g, outputBounds, displayedOutputRmsLeft, displayedOutputPeakLeft,
                    displayedOutputRmsRight, displayedOutputPeakRight, "OUTPUT");
}

void LevelMeters::resized()
{
}

void LevelMeters::drawMeterGroup (juce::Graphics& g, const juce::Rectangle<float>& bounds,
                                  float rmsDbL, float peakDbL,
                                  float rmsDbR, float peakDbR,
                                  const juce::String& groupLabel)
{
    // Label at top
    g.setColour (juce::Colour (0xff8f9ba8));
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (groupLabel, bounds.getX(), bounds.getY(), bounds.getWidth(), 15.0f, juce::Justification::centred);

    auto meterArea = bounds.withTrimmedTop (20.0f).withTrimmedBottom (12.0f);
    
    // Width subdivisions
    const float w = meterArea.getWidth();
    const float trackW = 10.0f;
    const float ticksW = 28.0f;
    const float pad = std::max (2.0f, (w - (trackW * 2.0f + ticksW)) / 2.0f);
    
    auto leftTrackBounds = meterArea.withWidth (trackW).withX (meterArea.getX() + pad);
    auto rightTrackBounds = meterArea.withWidth (trackW).withX (meterArea.getX() + pad + trackW + ticksW);
    auto ticksBounds = meterArea.withWidth (ticksW).withX (meterArea.getX() + pad + trackW);

    // Draw individual L/R meters
    drawVerticalMeter (g, leftTrackBounds, rmsDbL, peakDbL, true, "L");
    drawVerticalMeter (g, rightTrackBounds, rmsDbR, peakDbR, false, "R");

    // Draw central dB tick markings
    g.setColour (juce::Colour (0xff5a626a));
    g.setFont (9.0f);
    
    const std::vector<float> tickDbs = { 6.0f, 0.0f, -6.0f, -12.0f, -18.0f, -24.0f, -36.0f, -48.0f, -60.0f };
    const float trackHeight = leftTrackBounds.getHeight();
    const float trackBottom = leftTrackBounds.getBottom();

    for (float db : tickDbs)
    {
        const float frac = (db - minDb) / (maxDb - minDb);
        const float y = trackBottom - frac * trackHeight;
        
        // Horizontal tick line in the gap
        g.setColour (juce::Colour (0xff252c34));
        g.drawHorizontalLine (static_cast<int> (y), ticksBounds.getX(), ticksBounds.getRight());

        // Center-aligned dB text
        g.setColour (juce::Colour (0xff7a8694));
        juce::String labelStr = (db > 0.0f ? "+" : "") + juce::String (static_cast<int> (db));
        g.drawText (labelStr, ticksBounds.getX(), y - 6.0f, ticksBounds.getWidth(), 12.0f, juce::Justification::centred);
    }
}

void LevelMeters::drawVerticalMeter (juce::Graphics& g, const juce::Rectangle<float>& bounds,
                                     float rmsDb, float peakDb, bool isLeft, const juce::String& channelLabel)
{
    juce::ignoreUnused (isLeft);

    // Dark track background
    g.setColour (juce::Colour (0xff161a21));
    g.fillRoundedRectangle (bounds, 1.5f);

    const float trackBottom = bounds.getBottom();
    const float trackHeight = bounds.getHeight();

    // 1. Draw RMS bar
    const float rmsFrac = std::clamp ((rmsDb - minDb) / (maxDb - minDb), 0.0f, 1.0f);
    const float rmsH = rmsFrac * trackHeight;
    if (rmsH > 0.0f)
    {
        auto rmsRect = bounds.withHeight (rmsH).withY (trackBottom - rmsH);
        
        // Vertical gradient transitioning from neon cyan to neon magenta/pink at top
        juce::ColourGradient grad (
            juce::Colour (0xff00e5ff), bounds.getX(), trackBottom,
            juce::Colour (0xffd000ff), bounds.getX(), bounds.getY(),
            false
        );
        g.setGradientFill (grad);
        g.fillRoundedRectangle (rmsRect, 1.0f);
    }

    // 2. Draw Peak Hold tick (white horizontal line)
    const float peakFrac = std::clamp ((peakDb - minDb) / (maxDb - minDb), 0.0f, 1.0f);
    const float peakY = trackBottom - peakFrac * trackHeight;
    if (peakY >= bounds.getY() && peakY <= trackBottom)
    {
        g.setColour (juce::Colours::white);
        g.fillRoundedRectangle (bounds.getX(), peakY - 1.0f, bounds.getWidth(), 2.0f, 0.5f);
    }

    // Channel label below the meter
    g.setColour (juce::Colour (0xff8f9ba8));
    g.setFont (9.0f);
    g.drawText (channelLabel, bounds.getX(), bounds.getBottom() + 1.0f, bounds.getWidth(), 10.0f, juce::Justification::centred);
}

void LevelMeters::drawSaturationGroup (juce::Graphics& g, const juce::Rectangle<float>& bounds, float reductionDb)
{
    // Label at top
    g.setColour (juce::Colour (0xff8f9ba8));
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText ("SATURATION", bounds.getX(), bounds.getY(), bounds.getWidth(), 15.0f, juce::Justification::centred);

    // Digital readout at the bottom
    g.setColour (juce::Colour (0xffffffff));
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    juce::String valueStr = juce::String (reductionDb, 1) + " dB";
    g.drawText (valueStr, bounds.getX(), bounds.getBottom() - 12.0f, bounds.getWidth(), 12.0f, juce::Justification::centred);

    // Meter area in the middle
    auto meterArea = bounds.withTrimmedTop (20.0f).withTrimmedBottom (15.0f);
    
    const float w = meterArea.getWidth();
    const float trackW = 10.0f;
    const float ticksW = 16.0f;
    const float pad = std::max (2.0f, (w - (trackW + ticksW)) / 2.0f);

    auto ticksBounds = meterArea.withWidth (ticksW).withX (meterArea.getX() + pad);
    auto trackBounds = meterArea.withWidth (trackW).withX (meterArea.getX() + pad + ticksW);

    // Dark track background
    g.setColour (juce::Colour (0xff161a21));
    g.fillRoundedRectangle (trackBounds, 1.5f);

    // Draw active reduction bar (growing downward from 0 dB at top)
    const float fraction = std::clamp (reductionDb / 12.0f, 0.0f, 1.0f);
    const float barH = fraction * trackBounds.getHeight();
    if (barH > 0.0f)
    {
        auto barRect = trackBounds.withHeight (barH).withY (trackBounds.getY());
        
        // Amber/orange to red gradient
        juce::ColourGradient grad (
            juce::Colour (0xffffcc00), trackBounds.getX(), trackBounds.getY(),
            juce::Colour (0xffff3300), trackBounds.getX(), trackBounds.getBottom(),
            false
        );
        g.setGradientFill (grad);
        g.fillRoundedRectangle (barRect, 1.0f);
    }

    // Draw central dB tick markings on left side of the meter
    g.setColour (juce::Colour (0xff8f9ba8));
    g.setFont (8.0f);

    const std::vector<float> tickDbs = { 0.0f, 2.0f, 4.0f, 6.0f, 9.0f, 12.0f };
    const float trackHeight = trackBounds.getHeight();
    const float trackTop = trackBounds.getY();

    for (float db : tickDbs)
    {
        const float frac = db / 12.0f;
        const float y = trackTop + frac * trackHeight;

        g.setColour (juce::Colour (0xff252c34));
        g.drawHorizontalLine (static_cast<int> (y), ticksBounds.getX(), ticksBounds.getRight());

        g.setColour (juce::Colour (0xff7a8694));
        g.drawText (juce::String (static_cast<int> (db)), ticksBounds.getX(), y - 6.0f, ticksBounds.getWidth() - 2.0f, 12.0f, juce::Justification::centredRight);
    }
}

} // namespace ShredBoost
