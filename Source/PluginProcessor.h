#pragma once

#include <array>
#include <atomic>

#include <juce_audio_processors/juce_audio_processors.h>

#include "DSP/AutoGainBypass.h"
#include "DSP/InputTrimStage.h"
#include "DSP/MidVoicingFilter.h"
#include "DSP/OversamplingEngine.h"
#include "DSP/ShredBoostEngine.h"
#include "DSP/TightFilter.h"
#include "Parameters/ParameterLayout.h"
#include "Parameters/ParameterSnapshot.h"

class ShredBoostAudioProcessorEditor;

class ShredBoostAudioProcessor : public juce::AudioProcessor,
                                 private juce::Timer
{
public:
    ShredBoostAudioProcessor();
    ~ShredBoostAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    using ParameterValues = ShredBoost::ParameterSnapshot::Values;

    void timerCallback() override;
    void publishParameterValues() noexcept;
    [[nodiscard]] ParameterValues readRawParameterValues() const noexcept;
    [[nodiscard]] ParameterValues acquireParameterValues() noexcept;
    void processFloatBlock (juce::AudioBuffer<float>& buffer,
                            int numberOfChannels,
                            int numberOfSamples) noexcept;

    static constexpr int maximumChannels = 2;
    static constexpr int numberOfOversamplingFactors = 3;

    juce::AudioProcessorValueTreeState apvts;
    ShredBoost::ParameterSnapshot parameterSnapshot;
    ShredBoost::ParameterSnapshot::RawParameterPointers rawParameters;

    std::array<ParameterValues, 2> parameterBuffers {};
    std::array<std::atomic<unsigned int>, 2> parameterBufferReaders {};
    std::atomic<int> publishedParameterBuffer { 0 };
    ParameterValues lastAudioParameterValues {};

    std::array<ShredBoost::InputTrimStage, maximumChannels> inputTrimStages;
    std::array<ShredBoost::TightFilter, maximumChannels> tightFilters;
    std::array<ShredBoost::MidVoicingFilter, maximumChannels> midVoicingFilters;
    std::array<ShredBoost::ShredBoostEngine, numberOfOversamplingFactors> shredBoostEngines;
    ShredBoost::OversamplingEngine oversamplingEngine;
    ShredBoost::AutoGainBypass autoGainBypass;

    juce::AudioBuffer<float> dryBuffer;
    juce::AudioBuffer<float> doublePrecisionScratch;

    float currentDriveGain = 1.0f;
    float currentOutputGain = 1.0f;
    int preparedMaximumBlockSize = 0;
    int lastRequestedOversamplingFactor = 0;
    int lastReportedLatencySamples = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShredBoostAudioProcessor)
};
