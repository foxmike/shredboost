#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "Parameters/ParameterIDs.h"

namespace
{
constexpr auto stateTreeType = "ShredBoostState";
constexpr auto schemaVersionProperty = "schemaVersion";
} // namespace

ShredBoostAudioProcessor::ShredBoostAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
                            .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                            .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, stateTreeType, ShredBoost::createParameterLayout())
{
    parameterSnapshot.bindTo (apvts);
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
    juce::ignoreUnused (samplesPerBlock);
    parameterSnapshot.prepare (sampleRate);
}

void ShredBoostAudioProcessor::releaseResources()
{
}

bool ShredBoostAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();

    if (input == juce::AudioChannelSet::mono() && output == juce::AudioChannelSet::mono())
        return true;

    if (input == juce::AudioChannelSet::mono() && output == juce::AudioChannelSet::stereo())
        return true;

    if (input == juce::AudioChannelSet::stereo() && output == juce::AudioChannelSet::stereo())
        return true;

    return false;
}

void ShredBoostAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());
}

void ShredBoostAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());
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
    auto state = apvts.copyState();
    state.setProperty (schemaVersionProperty,
                       ShredBoost::ParamID::kParameterSchemaVersion,
                       nullptr);

    if (const auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void ShredBoostAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const auto xml = getXmlFromBinary (data, sizeInBytes);

    if (xml == nullptr)
        return;

    const auto state = juce::ValueTree::fromXml (*xml);

    if (state.isValid() && state.hasType (apvts.state.getType()))
        apvts.replaceState (state);
}

// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ShredBoostAudioProcessor();
}
