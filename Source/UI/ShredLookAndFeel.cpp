#include "ShredLookAndFeel.h"
#include <cmath>

namespace ShredBoost
{

ShredLookAndFeel::ShredLookAndFeel()
{
    // Window / Panel backgrounds
    setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (0xff12151a));
    setColour (juce::DocumentWindow::backgroundColourId, juce::Colour (0xff12151a));
    
    // Slider colors
    setColour (juce::Slider::thumbColourId, juce::Colour (0xff00e5ff));
    setColour (juce::Slider::trackColourId, juce::Colour (0xff1b2026));
    setColour (juce::Slider::backgroundColourId, juce::Colour (0xff0d0f12));
    setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff00e5ff));
    setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff2d333b));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffa9b2c3));
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff161a21));

    // Label colors
    setColour (juce::Label::textColourId, juce::Colour (0xffa9b2c3));
    setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);

    // TextButton colors
    setColour (juce::TextButton::buttonColourId, juce::Colour (0xff1b2026));
    setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff26303c));
    setColour (juce::TextButton::textColourOffId, juce::Colour (0xff8f9ba8));
    setColour (juce::TextButton::textColourOnId, juce::Colour (0xff00e5ff));
    
    // ToggleButton colors
    setColour (juce::ToggleButton::textColourId, juce::Colour (0xffa9b2c3));
}

ShredLookAndFeel::~ShredLookAndFeel()
{
}

void ShredLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPosProportional, float rotaryStartAngle,
                                         float rotaryEndAngle, juce::Slider& slider)
{
    if (width <= 0 || height <= 0)
        return;

    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
    auto centreX = bounds.getCentreX();
    auto centreY = bounds.getCentreY();
    
    auto maxRadius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto knobRadius = juce::jmax (10.0f, maxRadius - 14.0f);
    
    auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    
    // 1. Draw Machined Metal Knob Body
    g.setColour (juce::Colour (0x7f000000));
    g.fillEllipse (centreX - knobRadius + 1.0f, centreY - knobRadius + 2.0f, knobRadius * 2.0f, knobRadius * 2.0f);
    
    juce::ColourGradient metalGrad (juce::Colour (0xff4a525d), centreX, centreY - knobRadius,
                                    juce::Colour (0xff1a1d22), centreX, centreY + knobRadius, false);
    g.setGradientFill (metalGrad);
    g.fillEllipse (centreX - knobRadius, centreY - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);
    
    auto faceRadius = knobRadius - 2.5f;
    if (faceRadius > 0.0f)
    {
        juce::ColourGradient faceGrad (juce::Colour (0xff2b313c), centreX, centreY,
                                       juce::Colour (0xff15181c), centreX - faceRadius, centreY - faceRadius, true);
        g.setGradientFill (faceGrad);
        g.fillEllipse (centreX - faceRadius, centreY - faceRadius, faceRadius * 2.0f, faceRadius * 2.0f);
        
        g.setColour (juce::Colour (0x0dffffff));
        for (float r = faceRadius - 2.0f; r > 3.0f; r -= 3.0f)
        {
            g.drawEllipse (centreX - r, centreY - r, r * 2.0f, r * 2.0f, 0.5f);
        }
    }
    
    // 2. Draw Ticks
    auto tickRadius = knobRadius + 7.0f;
    auto tickLength = 3.5f;
    int numTicks = 11;
    for (int i = 0; i < numTicks; ++i)
    {
        float fraction = static_cast<float> (i) / static_cast<float> (numTicks - 1);
        float tickAngle = rotaryStartAngle + fraction * (rotaryEndAngle - rotaryStartAngle);
        
        bool isActive = (tickAngle <= angle + 0.005f);
        if (isActive)
            g.setColour (juce::Colour (0xff00e5ff));
        else
            g.setColour (juce::Colour (0xff3d4652));
            
        float cosA = std::cos (tickAngle - juce::MathConstants<float>::halfPi);
        float sinA = std::sin (tickAngle - juce::MathConstants<float>::halfPi);
        
        g.drawLine (centreX + tickRadius * cosA,
                    centreY + tickRadius * sinA,
                    centreX + (tickRadius + tickLength) * cosA,
                    centreY + (tickRadius + tickLength) * sinA,
                    1.5f);
    }
    
    // 3. Draw Value Arc
    auto arcRadius = knobRadius + 3.0f;
    
    juce::Path backgroundArc;
    backgroundArc.addCentredArc (centreX, centreY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour (0xff121519));
    g.strokePath (backgroundArc, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    
    if (angle > rotaryStartAngle)
    {
        juce::Path activeArc;
        activeArc.addCentredArc (centreX, centreY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, angle, true);
        
        g.setColour (juce::Colour (0x3300e5ff));
        g.strokePath (activeArc, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        
        g.setColour (juce::Colour (0xff00e5ff));
        g.strokePath (activeArc, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    
    // 4. Draw Indicator Line
    float indicatorInnerRadius = knobRadius * 0.25f;
    float indicatorOuterRadius = knobRadius - 1.5f;
    
    float cosA = std::cos (angle - juce::MathConstants<float>::halfPi);
    float sinA = std::sin (angle - juce::MathConstants<float>::halfPi);
    
    g.setColour (juce::Colour (0xff090c0f));
    g.drawLine (centreX + indicatorInnerRadius * cosA,
                centreY + indicatorInnerRadius * sinA,
                centreX + indicatorOuterRadius * cosA,
                centreY + indicatorOuterRadius * sinA,
                3.0f);
                
    g.setColour (juce::Colour (0xff00e5ff));
    g.drawLine (centreX + indicatorInnerRadius * cosA,
                centreY + indicatorInnerRadius * sinA,
                centreX + indicatorOuterRadius * cosA,
                centreY + indicatorOuterRadius * sinA,
                1.5f);
}

void ShredLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                             const juce::Colour& backgroundColour,
                                             bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto cornerSize = 4.0f;
    
    juce::Colour baseColour = backgroundColour;
    if (shouldDrawButtonAsDown)
        baseColour = baseColour.darker (0.4f);
    else if (shouldDrawButtonAsHighlighted)
        baseColour = baseColour.brighter (0.1f);
        
    g.setColour (juce::Colour (0xff1b2026));
    g.fillRoundedRectangle (bounds, cornerSize);
    
    auto innerBounds = bounds.reduced (1.0f);
    juce::ColourGradient grad (baseColour.brighter (0.05f), 0.0f, innerBounds.getY(),
                               baseColour.darker (0.15f), 0.0f, innerBounds.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (innerBounds, cornerSize - 1.0f);
    
    if (shouldDrawButtonAsHighlighted || button.getToggleState())
    {
        g.setColour (juce::Colour (0x4400e5ff));
        g.drawRoundedRectangle (innerBounds, cornerSize - 1.0f, 1.5f);
    }
    else
    {
        g.setColour (juce::Colour (0x10ffffff));
        g.drawRoundedRectangle (innerBounds, cornerSize - 1.0f, 1.0f);
    }
}

void ShredLookAndFeel::drawTickBox (juce::Graphics& g, juce::Component& component,
                                    float x, float y, float w, float h,
                                    bool ticked, bool isEnabled,
                                    bool shouldDrawButtonAsHighlighted,
                                    bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (isEnabled, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    
    auto box = juce::Rectangle<float> (x, y, w, h).reduced (2.0f);
    
    g.setColour (juce::Colour (0xff101215));
    g.fillRoundedRectangle (box, 2.0f);
    
    g.setColour (juce::Colour (0xff2d333b));
    g.drawRoundedRectangle (box, 2.0f, 1.0f);
    
    if (ticked)
    {
        g.setColour (juce::Colour (0xff00e5ff));
        g.fillRoundedRectangle (box.reduced (3.0f), 1.0f);
    }
}

} // namespace ShredBoost
