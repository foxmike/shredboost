#include "PluginProcessor.h"
#include "PluginEditor.h"

ShredBoostAudioProcessor::ShredBoostAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
                            .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                            .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

ShredBoostAudioProcessor::~ShredBoostAudioProcessor() = default;

const juce::String ShredBoostAudioProcessor::getName() const
{
    return "ShredBoost";
}

bool ShredBoostAudioProcessor::acceptsMidi() const
{
    return false;
}

bool ShredBoostAudioProcessor::producesMidi() const
{
    return false;
}

bool ShredBoostAudioProcessor::isMidiEffect() const
{
    return false;
}

double ShredBoostAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ShredBoostAudioProcessor::getNumPrograms()
{
    return 1;
}

int ShredBoostAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ShredBoostAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String ShredBoostAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void ShredBoostAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void ShredBoostAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

void ShredBoostAudioProcessor::releaseResources()
{
}

bool ShredBoostAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();

    const bool inputIsMono = (input == juce::AudioChannelSet::mono());
    const bool inputIsStereo = (input == juce::AudioChannelSet::stereo());
    const bool outputIsMono = (output == juce::AudioChannelSet::mono());
    const bool outputIsStereo = (output == juce::AudioChannelSet::stereo());

    if (! (inputIsMono || inputIsStereo))
        return false;

    if (! (outputIsMono || outputIsStereo))
        return false;

    if (inputIsStereo && outputIsMono)
        return false;

    return true;
}

void ShredBoostAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    const auto numInputChannels  = getTotalNumInputChannels();
    const auto numOutputChannels = getTotalNumOutputChannels();
    const auto numSamples        = buffer.getNumSamples();

    for (auto channel = numInputChannels; channel < numOutputChannels; ++channel)
        buffer.clear (channel, 0, numSamples);
}

void ShredBoostAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    const auto numInputChannels  = getTotalNumInputChannels();
    const auto numOutputChannels = getTotalNumOutputChannels();
    const auto numSamples        = buffer.getNumSamples();

    for (auto channel = numInputChannels; channel < numOutputChannels; ++channel)
        buffer.clear (channel, 0, numSamples);
}

bool ShredBoostAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* ShredBoostAudioProcessor::createEditor()
{
    return new ShredBoostAudioProcessorEditor (*this);
}

void ShredBoostAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ignoreUnused (destData);
}

void ShredBoostAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
}

// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ShredBoostAudioProcessor();
}
