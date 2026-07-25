#pragma once

#include "PluginProcessor.h"

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShredBoostAudioProcessorEditor)
};
