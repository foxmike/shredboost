#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ShredBoostAudioProcessorEditor::ShredBoostAudioProcessorEditor (ShredBoostAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setResizable (true, true);
    setSize (600, 400);
}

ShredBoostAudioProcessorEditor::~ShredBoostAudioProcessorEditor()
{
}

//==============================================================================
void ShredBoostAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("ShredBoost", getLocalBounds(), juce::Justification::centred, 1);
}

void ShredBoostAudioProcessorEditor::resized()
{
}
