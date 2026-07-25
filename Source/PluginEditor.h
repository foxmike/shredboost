#pragma once

#include "PluginProcessor.h"
#include "UI/SpectrumOverlay.h"

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
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ShredBoostAudioProcessor& audioProcessor;

    ShredBoost::SpectrumOverlay spectrumOverlay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShredBoostAudioProcessorEditor)
};
