#pragma once

#include "PluginProcessor.h"
#include "UI/ShredLookAndFeel.h"
#include "UI/TransferCurveScope.h"
#include "UI/SpectrumOverlay.h"
#include "UI/LevelMeters.h"

//==============================================================================
class ShredBoostAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    ShredBoostAudioProcessorEditor (ShredBoostAudioProcessor&);
    ~ShredBoostAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    ShredBoostAudioProcessor& audioProcessor;

    ShredBoost::ShredLookAndFeel lookAndFeel;

    // Telemetry components
    ShredBoost::SpectrumOverlay spectrumOverlay;
    ShredBoost::TransferCurveScope transferCurveScope;
    ShredBoost::LevelMeters levelMeters;

    // Rotary Sliders
    juce::Slider inputTrimSlider;
    juce::Slider tightCutSlider;
    juce::Slider midFreqSlider;
    juce::Slider midAmountSlider;
    juce::Slider driveSlider;
    juce::Slider toneSlider;
    juce::Slider levelSlider;
    juce::Slider pickAttackSlider;

    // Dropdowns
    juce::ComboBox voicingModeCombo;
    juce::ComboBox oversamplingCombo;

    // Buttons
    juce::TextButton bypassButton;

    // Labels
    juce::Label inputTrimLabel;
    juce::Label tightCutLabel;
    juce::Label midFreqLabel;
    juce::Label midAmountLabel;
    juce::Label driveLabel;
    juce::Label toneLabel;
    juce::Label levelLabel;
    juce::Label pickAttackLabel;
    juce::Label voicingModeLabel;
    juce::Label oversamplingLabel;

    // APVTS Attachments
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> inputTrimAttachment;
    std::unique_ptr<SliderAttachment> tightCutAttachment;
    std::unique_ptr<SliderAttachment> midFreqAttachment;
    std::unique_ptr<SliderAttachment> midAmountAttachment;
    std::unique_ptr<SliderAttachment> driveAttachment;
    std::unique_ptr<SliderAttachment> toneAttachment;
    std::unique_ptr<SliderAttachment> levelAttachment;
    std::unique_ptr<SliderAttachment> pickAttackAttachment;

    std::unique_ptr<ComboBoxAttachment> voicingModeAttachment;
    std::unique_ptr<ComboBoxAttachment> oversamplingAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShredBoostAudioProcessorEditor)
};
