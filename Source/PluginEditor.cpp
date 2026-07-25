#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Parameters/ParameterIDs.h"

//==============================================================================
ShredBoostAudioProcessorEditor::ShredBoostAudioProcessorEditor (ShredBoostAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Apply Look and Feel
    setLookAndFeel (&lookAndFeel);

    // 1. Setup Telemetry components
    addAndMakeVisible (spectrumOverlay);
    spectrumOverlay.setFIFOs (&audioProcessor.getPreDriveFIFO(), &audioProcessor.getPostDriveFIFO());
    spectrumOverlay.setSampleRate (audioProcessor.getSampleRate());

    addAndMakeVisible (transferCurveScope);
    transferCurveScope.setTelemetrySources (audioProcessor.getTelemetryAmplitude(), audioProcessor.getTelemetryVoicingMode());

    addAndMakeVisible (levelMeters);
    levelMeters.setTelemetrySources (
        audioProcessor.getInputPeakLeft(),
        audioProcessor.getInputPeakRight(),
        audioProcessor.getInputRmsLeft(),
        audioProcessor.getInputRmsRight(),
        audioProcessor.getOutputPeakLeft(),
        audioProcessor.getOutputPeakRight(),
        audioProcessor.getOutputRmsLeft(),
        audioProcessor.getOutputRmsRight(),
        audioProcessor.getPeakReductionDb()
    );

    // 2. Setup Sliders and Labels with helper lambda
    auto setupRotary = [this] (juce::Slider& slider, juce::Label& label, const juce::String& name)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 65, 16);
        addAndMakeVisible (slider);
        
        label.setText (name, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (label);
    };

    setupRotary (inputTrimSlider, inputTrimLabel, "Input Trim");
    setupRotary (tightCutSlider, tightCutLabel, "Tight Cut");
    setupRotary (midFreqSlider, midFreqLabel, "Mid Freq");
    setupRotary (midAmountSlider, midAmountLabel, "Mid Boost");
    setupRotary (driveSlider, driveLabel, "Drive");
    setupRotary (toneSlider, toneLabel, "Tone");
    setupRotary (levelSlider, levelLabel, "Level");
    setupRotary (pickAttackSlider, pickAttackLabel, "Pick Attack");

    // 3. Setup Dropdowns and labels
    voicingModeCombo.addItemList (juce::StringArray { "Yng-Grey", "Precision Tight", "SD-1", "Clean" }, 1);
    addAndMakeVisible (voicingModeCombo);
    voicingModeLabel.setText ("Voicing Mode", juce::dontSendNotification);
    voicingModeLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (voicingModeLabel);

    oversamplingCombo.addItemList (juce::StringArray { "1x", "2x", "4x" }, 1);
    addAndMakeVisible (oversamplingCombo);
    oversamplingLabel.setText ("Oversampling", juce::dontSendNotification);
    oversamplingLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (oversamplingLabel);

    // 4. Setup Bypass Button
    bypassButton.setButtonText ("Bypass");
    bypassButton.setClickingTogglesState (true);
    addAndMakeVisible (bypassButton);

    // 5. APVTS Attachments
    namespace ParamID = ShredBoost::ParamID;
    auto& apvts = audioProcessor.getAPVTS();

    inputTrimAttachment   = std::make_unique<SliderAttachment> (apvts, ParamID::input_trim, inputTrimSlider);
    tightCutAttachment    = std::make_unique<SliderAttachment> (apvts, ParamID::tight_cut, tightCutSlider);
    midFreqAttachment     = std::make_unique<SliderAttachment> (apvts, ParamID::mid_freq, midFreqSlider);
    midAmountAttachment   = std::make_unique<SliderAttachment> (apvts, ParamID::mid_amount, midAmountSlider);
    driveAttachment       = std::make_unique<SliderAttachment> (apvts, ParamID::drive, driveSlider);
    toneAttachment        = std::make_unique<SliderAttachment> (apvts, ParamID::tone, toneSlider);
    levelAttachment       = std::make_unique<SliderAttachment> (apvts, ParamID::level, levelSlider);
    pickAttackAttachment  = std::make_unique<SliderAttachment> (apvts, ParamID::pick_attack, pickAttackSlider);

    voicingModeAttachment  = std::make_unique<ComboBoxAttachment> (apvts, ParamID::voicing_mode, voicingModeCombo);
    oversamplingAttachment = std::make_unique<ComboBoxAttachment> (apvts, ParamID::oversampling, oversamplingCombo);
    bypassAttachment       = std::make_unique<ButtonAttachment> (apvts, ParamID::auto_gain, bypassButton);

    // 6. Resizability & Default Size
    setResizable (true, true);
    setResizeLimits (900, 550, 1800, 1100);
    setSize (1000, 600);
}

ShredBoostAudioProcessorEditor::~ShredBoostAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

//==============================================================================
void ShredBoostAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Dark premium background gradient
    juce::ColourGradient grad (
        juce::Colour (0xff0f1219), 0.0f, 0.0f,
        juce::Colour (0xff080a0f), 0.0f, static_cast<float> (getHeight()),
        false
    );
    g.setGradientFill (grad);
    g.fillAll();

    // Draw header panel
    g.setColour (juce::Colour (0xff00e5ff));
    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.drawText ("SHREDBOOST", 20, 15, 150, 24, juce::Justification::left);

    g.setColour (juce::Colour (0xff8f9ba8));
    g.setFont (juce::FontOptions (11.0f, juce::Font::plain));
    g.drawText ("PREAMP & SATURATION SHAPER", 145, 20, 300, 16, juce::Justification::left);
}

void ShredBoostAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Reserved top margin for title/header
    const int headerHeight = 45;
    auto contentArea = bounds.withTrimmedTop (headerHeight).reduced (15);

    // Split contentArea vertically: top for visuals, bottom for controls
    // Visuals take 55% of height, Controls take 45%
    const int totalHeight = contentArea.getHeight();
    const int visualsHeight = static_cast<int> (totalHeight * 0.55);
    const int controlsHeight = totalHeight - visualsHeight - 15; // 15px gap

    auto visualsArea = contentArea.withHeight (visualsHeight);
    auto controlsArea = contentArea.withHeight (controlsHeight).withY (visualsArea.getBottom() + 15);

    // Lay out visuals (SpectrumOverlay, TransferCurveScope, LevelMeters)
    // SpectrumOverlay: 45% width
    // TransferCurveScope: 30% width
    // LevelMeters: 25% width
    const int totalWidth = visualsArea.getWidth();
    const int spectrumWidth = static_cast<int> (totalWidth * 0.45);
    const int scopeWidth = static_cast<int> (totalWidth * 0.30);
    const int metersWidth = totalWidth - spectrumWidth - scopeWidth - 20; // 10px gaps

    spectrumOverlay.setBounds (visualsArea.removeFromLeft (spectrumWidth));
    visualsArea.removeFromLeft (10); // gap
    transferCurveScope.setBounds (visualsArea.removeFromLeft (scopeWidth));
    visualsArea.removeFromLeft (10); // gap
    levelMeters.setBounds (visualsArea.removeFromLeft (metersWidth));

    // Lay out controls (controlsArea)
    // Divide controlsArea horizontally into three main columns:
    // Left Column (Pre-processing controls): 4 sliders
    // Center Column (ComboBoxes and Bypass)
    // Right Column (Core / Post-processing controls): 4 sliders
    
    const int ctrlWidth = controlsArea.getWidth();
    const int colWidth = (ctrlWidth - 40) / 3; // 20px gaps

    auto preCol = controlsArea.removeFromLeft (colWidth);
    controlsArea.removeFromLeft (20); // gap
    auto centerCol = controlsArea.removeFromLeft (colWidth);
    controlsArea.removeFromLeft (20); // gap
    auto postCol = controlsArea; // remainder

    // Lay out pre-processing sliders
    const int sliderWidthPre = preCol.getWidth() / 4;
    auto preSliderArea = preCol;
    
    const int labelHeight = 18;

    auto layOutSlider = [&] (juce::Slider& slider, juce::Label& label, juce::Rectangle<int> area)
    {
        label.setBounds (area.removeFromTop (labelHeight));
        slider.setBounds (area);
    };

    layOutSlider (inputTrimSlider, inputTrimLabel, preSliderArea.removeFromLeft (sliderWidthPre));
    layOutSlider (tightCutSlider, tightCutLabel, preSliderArea.removeFromLeft (sliderWidthPre));
    layOutSlider (midFreqSlider, midFreqLabel, preSliderArea.removeFromLeft (sliderWidthPre));
    layOutSlider (midAmountSlider, midAmountLabel, preSliderArea);

    // Lay out core/post-processing sliders
    const int sliderWidthPost = postCol.getWidth() / 4;
    auto postSliderArea = postCol;
    layOutSlider (driveSlider, driveLabel, postSliderArea.removeFromLeft (sliderWidthPost));
    layOutSlider (toneSlider, toneLabel, postSliderArea.removeFromLeft (sliderWidthPost));
    layOutSlider (levelSlider, levelLabel, postSliderArea.removeFromLeft (sliderWidthPost));
    layOutSlider (pickAttackSlider, pickAttackLabel, postSliderArea);

    // Lay out center panel (voicingModeCombo, oversamplingCombo, bypassButton) stacked vertically
    const int centerItemHeight = 24;
    const int centerLabelHeight = 16;
    const int itemGap = 6;
    
    auto centerArea = centerCol.reduced (20, 0); // inset slightly for a cleaner look
    
    // Voicing mode
    voicingModeLabel.setBounds (centerArea.removeFromTop (centerLabelHeight));
    voicingModeCombo.setBounds (centerArea.removeFromTop (centerItemHeight));
    centerArea.removeFromTop (itemGap);

    // Oversampling
    oversamplingLabel.setBounds (centerArea.removeFromTop (centerLabelHeight));
    oversamplingCombo.setBounds (centerArea.removeFromTop (centerItemHeight));
    centerArea.removeFromTop (itemGap + 4);

    // Bypass Button (centered and prominent)
    bypassButton.setBounds (centerArea.removeFromTop (32));
}
