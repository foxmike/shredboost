#pragma once

#include <atomic>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace ShredBoost
{

class ParameterSnapshot final
{
public:
    struct Values
    {
        float input_trim = 0.0f;
        float tight_cut = 180.0f;
        float mid_freq = 720.0f;
        float mid_amount = 4.0f;
        float drive = 12.0f;
        float tone = 6.5f;
        float level = 0.0f;
        float pick_attack = 2.0f;

        int voicing_mode = 0;
        int oversampling = 1;
        bool auto_gain = false;
    };

    struct RawParameterPointers
    {
        std::atomic<float>* input_trim = nullptr;
        std::atomic<float>* tight_cut = nullptr;
        std::atomic<float>* mid_freq = nullptr;
        std::atomic<float>* mid_amount = nullptr;
        std::atomic<float>* drive = nullptr;
        std::atomic<float>* tone = nullptr;
        std::atomic<float>* level = nullptr;
        std::atomic<float>* pick_attack = nullptr;

        std::atomic<float>* voicing_mode = nullptr;
        std::atomic<float>* oversampling = nullptr;
        std::atomic<float>* auto_gain = nullptr;
    };

    ParameterSnapshot() noexcept;

    // Binding performs parameter lookup and must be done off the audio thread.
    void bindTo (juce::AudioProcessorValueTreeState& state);
    void setRawParameterPointers (RawParameterPointers pointers) noexcept;

    // Call from prepareToPlay before using the snapshot on the audio thread.
    void prepare (double sampleRate) noexcept;

    // Capture automation targets once at the start of an audio block.
    void updateTargetsFromRawParameters() noexcept;
    void setTargets (const Values& targets) noexcept;

    // Advance by one sample, or skip a complete block.
    [[nodiscard]] Values advance() noexcept;
    [[nodiscard]] Values advance (int numberOfSamples) noexcept;
    [[nodiscard]] Values current() const noexcept;

private:
    using LinearSmoother = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>;
    using MultiplicativeSmoother = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative>;

    [[nodiscard]] Values readRawParameters() const noexcept;
    void snapTo (const Values& values) noexcept;

    static constexpr double smoothingTimeSeconds = 0.02;

    RawParameterPointers raw;

    LinearSmoother inputTrim;
    MultiplicativeSmoother tightCut;
    MultiplicativeSmoother midFreq;
    LinearSmoother midAmount;
    LinearSmoother drive;
    MultiplicativeSmoother tone;
    LinearSmoother level;
    LinearSmoother pickAttack;

    int voicingMode = 0;
    int oversamplingMode = 1;
    bool autoGain = false;
};

} // namespace ShredBoost
