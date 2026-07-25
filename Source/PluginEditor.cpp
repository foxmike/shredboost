#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ShredBoostAudioProcessorEditor::ShredBoostAudioProcessorEditor (ShredBoostAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    addAndMakeVisible (spectrumOverlay);
    spectrumOverlay.setFIFOs (&audioProcessor.getPreDriveFIFO(), &audioProcessor.getPostDriveFIFO());
    spectrumOverlay.setSampleRate (audioProcessor.getSampleRate());

    setResizable (true, true);
    setSize (600, 400);
}

ShredBoostAudioProcessorEditor::~ShredBoostAudioProcessorEditor()
{
}

//==============================================================================
void ShredBoostAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::ignoreUnused (g);
}

void ShredBoostAudioProcessorEditor::resized()
{
    spectrumOverlay.setBounds (getLocalBounds());
}
