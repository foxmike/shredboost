#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "Parameters/ParameterIDs.h"

#include <algorithm>

namespace
{
constexpr auto stateTreeType = "ShredBoostState";
constexpr auto schemaVersionProperty = "schemaVersion";

static_assert (std::atomic<float>::is_always_lock_free);
static_assert (std::atomic<int>::is_always_lock_free);
static_assert (std::atomic<unsigned int>::is_always_lock_free);

float loadOr (const std::atomic<float>* parameter, float fallback) noexcept
{
    return parameter != nullptr
               ? parameter->load (std::memory_order_relaxed)
               : fallback;
}

int oversamplingChoiceToFactor (int choice) noexcept
{
    return 1 << std::clamp (choice, 0, 2);
}

int oversamplingFactorToIndex (int factor) noexcept
{
    switch (factor)
    {
        case 2: return 1;
        case 4: return 2;
        default: return 0;
    }
}

void applyGainRamp (juce::dsp::AudioBlock<float>& block,
                    float& currentGain,
                    float targetGain) noexcept
{
    const auto numberOfSamples = block.getNumSamples();

    if (numberOfSamples == 0)
    {
        currentGain = targetGain;
        return;
    }

    const auto gainIncrement =
        (targetGain - currentGain) / static_cast<float> (numberOfSamples);

    for (std::size_t sample = 0; sample < numberOfSamples; ++sample)
    {
        currentGain += gainIncrement;

        for (std::size_t channel = 0; channel < block.getNumChannels(); ++channel)
            block.getChannelPointer (channel)[sample] *= currentGain;
    }

    currentGain = targetGain;
}
} // namespace

ShredBoostAudioProcessor::ShredBoostAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
                            .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                            .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, stateTreeType, ShredBoost::createParameterLayout())
{
    rawParameters.input_trim = apvts.getRawParameterValue (ShredBoost::ParamID::input_trim);
    rawParameters.tight_cut = apvts.getRawParameterValue (ShredBoost::ParamID::tight_cut);
    rawParameters.mid_freq = apvts.getRawParameterValue (ShredBoost::ParamID::mid_freq);
    rawParameters.mid_amount = apvts.getRawParameterValue (ShredBoost::ParamID::mid_amount);
    rawParameters.drive = apvts.getRawParameterValue (ShredBoost::ParamID::drive);
    rawParameters.tone = apvts.getRawParameterValue (ShredBoost::ParamID::tone);
    rawParameters.level = apvts.getRawParameterValue (ShredBoost::ParamID::level);
    rawParameters.pick_attack = apvts.getRawParameterValue (ShredBoost::ParamID::pick_attack);
    rawParameters.voicing_mode = apvts.getRawParameterValue (ShredBoost::ParamID::voicing_mode);
    rawParameters.oversampling = apvts.getRawParameterValue (ShredBoost::ParamID::oversampling);
    rawParameters.auto_gain = apvts.getRawParameterValue (ShredBoost::ParamID::auto_gain);

    parameterSnapshot.setRawParameterPointers (rawParameters);

    const auto initialValues = readRawParameterValues();
    parameterBuffers[0] = initialValues;
    parameterBuffers[1] = initialValues;
    lastAudioParameterValues = initialValues;
}

ShredBoostAudioProcessor::~ShredBoostAudioProcessor()
{
    stopTimer();
}

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
    stopTimer();

    preparedMaximumBlockSize = std::max (1, samplesPerBlock);
    parameterSnapshot.prepare (sampleRate);

    const auto initialValues = readRawParameterValues();
    parameterBuffers[0] = initialValues;
    parameterBuffers[1] = initialValues;
    publishedParameterBuffer.store (0, std::memory_order_release);
    lastAudioParameterValues = initialValues;

    for (auto& stage : inputTrimStages)
        stage.prepare (sampleRate);

    for (auto& filter : tightFilters)
        filter.prepare (sampleRate);

    for (auto& filter : midVoicingFilters)
        filter.prepare (sampleRate);

    for (int factorIndex = 0;
         factorIndex < numberOfOversamplingFactors;
         ++factorIndex)
    {
        const auto factor = 1 << factorIndex;
        shredBoostEngines[static_cast<std::size_t> (factorIndex)].prepare (
            sampleRate * static_cast<double> (factor),
            preparedMaximumBlockSize * factor);
    }

    const auto initialFactor =
        oversamplingChoiceToFactor (initialValues.oversampling);
    oversamplingEngine.requestFactorChange (initialFactor);
    oversamplingEngine.prepare (
        sampleRate, preparedMaximumBlockSize, maximumChannels);
    autoGainBypass.prepare (
        sampleRate, preparedMaximumBlockSize, maximumChannels);

    dryBuffer.setSize (
        maximumChannels, preparedMaximumBlockSize, false, true, false);
    doublePrecisionScratch.setSize (
        maximumChannels, preparedMaximumBlockSize, false, true, false);
    dryBuffer.clear();
    doublePrecisionScratch.clear();

    currentDriveGain =
        juce::Decibels::decibelsToGain (initialValues.drive);
    currentOutputGain =
        juce::Decibels::decibelsToGain (initialValues.level);

    lastRequestedOversamplingFactor = initialFactor;
    lastReportedLatencySamples = oversamplingEngine.getLatencySamples();
    setLatencySamples (lastReportedLatencySamples);
    startTimerHz (30);
}

void ShredBoostAudioProcessor::releaseResources()
{
    stopTimer();
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

    processFloatBlock (
        buffer,
        std::min (buffer.getNumChannels(), maximumChannels),
        buffer.getNumSamples());
}

void ShredBoostAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    const auto numberOfChannels =
        std::min (buffer.getNumChannels(), maximumChannels);
    const auto numberOfSamples = buffer.getNumSamples();

    if (numberOfSamples > preparedMaximumBlockSize)
    {
        buffer.clear();
        return;
    }

    for (int channel = 0; channel < numberOfChannels; ++channel)
    {
        const auto* source = buffer.getReadPointer (channel);
        auto* destination = doublePrecisionScratch.getWritePointer (channel);

        for (int sample = 0; sample < numberOfSamples; ++sample)
            destination[sample] = static_cast<float> (source[sample]);
    }

    processFloatBlock (
        doublePrecisionScratch, numberOfChannels, numberOfSamples);

    for (int channel = 0; channel < numberOfChannels; ++channel)
    {
        const auto* source = doublePrecisionScratch.getReadPointer (channel);
        auto* destination = buffer.getWritePointer (channel);

        for (int sample = 0; sample < numberOfSamples; ++sample)
            destination[sample] = static_cast<double> (source[sample]);
    }

    for (int channel = numberOfChannels;
         channel < buffer.getNumChannels();
         ++channel)
        buffer.clear (channel, 0, numberOfSamples);
}

void ShredBoostAudioProcessor::timerCallback()
{
    publishParameterValues();

    const auto values =
        parameterBuffers[static_cast<std::size_t> (
            publishedParameterBuffer.load (std::memory_order_acquire))];
    const auto requestedFactor =
        oversamplingChoiceToFactor (values.oversampling);

    if (requestedFactor != lastRequestedOversamplingFactor)
    {
        oversamplingEngine.requestFactorChange (requestedFactor);
        lastRequestedOversamplingFactor = requestedFactor;
    }

    if (oversamplingEngine.isFactorChangePending())
        oversamplingEngine.applyPendingFactor();

    const auto latency = oversamplingEngine.getLatencySamples();

    if (latency != lastReportedLatencySamples)
    {
        lastReportedLatencySamples = latency;
        setLatencySamples (latency);
    }
}

void ShredBoostAudioProcessor::publishParameterValues() noexcept
{
    const auto current =
        publishedParameterBuffer.load (std::memory_order_acquire);
    const auto destination = 1 - current;

    if (parameterBufferReaders[static_cast<std::size_t> (destination)].load (
            std::memory_order_acquire) != 0)
        return;

    parameterBuffers[static_cast<std::size_t> (destination)] =
        readRawParameterValues();
    publishedParameterBuffer.store (destination, std::memory_order_release);
}

ShredBoostAudioProcessor::ParameterValues
ShredBoostAudioProcessor::readRawParameterValues() const noexcept
{
    const auto fallback =
        parameterBuffers[static_cast<std::size_t> (
            publishedParameterBuffer.load (std::memory_order_relaxed))];

    return {
        loadOr (rawParameters.input_trim, fallback.input_trim),
        loadOr (rawParameters.tight_cut, fallback.tight_cut),
        loadOr (rawParameters.mid_freq, fallback.mid_freq),
        loadOr (rawParameters.mid_amount, fallback.mid_amount),
        loadOr (rawParameters.drive, fallback.drive),
        loadOr (rawParameters.tone, fallback.tone),
        loadOr (rawParameters.level, fallback.level),
        loadOr (rawParameters.pick_attack, fallback.pick_attack),
        static_cast<int> (
            loadOr (rawParameters.voicing_mode,
                    static_cast<float> (fallback.voicing_mode))),
        static_cast<int> (
            loadOr (rawParameters.oversampling,
                    static_cast<float> (fallback.oversampling))),
        loadOr (rawParameters.auto_gain, fallback.auto_gain ? 1.0f : 0.0f)
            >= 0.5f
    };
}

ShredBoostAudioProcessor::ParameterValues
ShredBoostAudioProcessor::acquireParameterValues() noexcept
{
    for (int attempt = 0; attempt < 2; ++attempt)
    {
        const auto source =
            publishedParameterBuffer.load (std::memory_order_acquire);
        auto& readers =
            parameterBufferReaders[static_cast<std::size_t> (source)];
        readers.fetch_add (1, std::memory_order_acquire);

        if (source
            == publishedParameterBuffer.load (std::memory_order_acquire))
        {
            lastAudioParameterValues =
                parameterBuffers[static_cast<std::size_t> (source)];
            readers.fetch_sub (1, std::memory_order_release);
            return lastAudioParameterValues;
        }

        readers.fetch_sub (1, std::memory_order_release);
    }

    return lastAudioParameterValues;
}

void ShredBoostAudioProcessor::processFloatBlock (
    juce::AudioBuffer<float>& buffer,
    int numberOfChannels,
    int numberOfSamples) noexcept
{
    if (numberOfSamples > preparedMaximumBlockSize || numberOfChannels <= 0)
    {
        buffer.clear();
        return;
    }

    const auto targets = acquireParameterValues();
    parameterSnapshot.setTargets (targets);
    const auto values = parameterSnapshot.advance (numberOfSamples);

    const auto numberOfInputChannels =
        std::min (getTotalNumInputChannels(), numberOfChannels);

    for (int channel = numberOfInputChannels;
         channel < numberOfChannels;
         ++channel)
        buffer.clear (channel, 0, numberOfSamples);

    for (int channel = 0; channel < numberOfChannels; ++channel)
        dryBuffer.copyFrom (
            channel, 0, buffer, channel, 0, numberOfSamples);

    for (int channel = 0; channel < numberOfChannels; ++channel)
    {
        auto* samples = buffer.getWritePointer (channel);
        auto& inputTrim =
            inputTrimStages[static_cast<std::size_t> (channel)];
        auto& tightFilter =
            tightFilters[static_cast<std::size_t> (channel)];
        auto& midFilter =
            midVoicingFilters[static_cast<std::size_t> (channel)];

        inputTrim.setTrimDb (values.input_trim);
        tightFilter.setCutoff (values.tight_cut);
        tightFilter.setResonantMode (values.voicing_mode == 1);
        midFilter.setFrequency (values.mid_freq);
        midFilter.setAmount (values.mid_amount);

        inputTrim.processBlock (samples, numberOfSamples);
        tightFilter.processBlock (samples, numberOfSamples);
        midFilter.processBlock (samples, numberOfSamples);
    }

    juce::dsp::AudioBlock<float> fullWetBlock (buffer);
    auto wetBlock =
        fullWetBlock
            .getSubsetChannelBlock (
                0, static_cast<std::size_t> (numberOfChannels))
            .getSubBlock (0, static_cast<std::size_t> (numberOfSamples));
    const juce::dsp::AudioBlock<const float> constWetBlock (wetBlock);
    auto oversampledBlock = oversamplingEngine.upsample (constWetBlock);

    if (oversampledBlock.getNumChannels() == 0)
    {
        wetBlock.clear();
        return;
    }

    applyGainRamp (
        oversampledBlock,
        currentDriveGain,
        juce::Decibels::decibelsToGain (values.drive));

    const auto factorIndex =
        oversamplingFactorToIndex (
            oversamplingEngine.getOversamplingFactor());
    auto& engine =
        shredBoostEngines[static_cast<std::size_t> (factorIndex)];
    engine.setPickAttack (values.pick_attack);
    engine.setVoicingMode (values.voicing_mode);
    engine.setTone (values.tone);
    engine.process (oversampledBlock);

    oversamplingEngine.downsample (wetBlock);

    applyGainRamp (
        wetBlock,
        currentOutputGain,
        juce::Decibels::decibelsToGain (values.level));

    juce::dsp::AudioBlock<float> fullDryBlock (dryBuffer);
    const auto drySubBlock =
        fullDryBlock
            .getSubsetChannelBlock (
                0, static_cast<std::size_t> (numberOfChannels))
            .getSubBlock (0, static_cast<std::size_t> (numberOfSamples));
    const juce::dsp::AudioBlock<const float> constDryBlock (drySubBlock);
    autoGainBypass.process (constDryBlock, wetBlock, values.auto_gain);
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
