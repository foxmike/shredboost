#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>

namespace ShredBoost
{

/**
    Auto gain bypass compensation module.
    
    Tracks the RMS of both the dry input (before effect) and wet output (after effect)
    signals to compute a dynamic loudness difference (gain compensation factor).
    During bypass preview, it applies this dynamic compensation gain to the dry
    signal so that the bypassed audio is loudness-matched to the processed signal.
*/
class AutoGainBypass final
{
public:
    AutoGainBypass() noexcept;
    ~AutoGainBypass() = default;

    /** Pre-allocates channel estimators and prepares the module for processing. */
    void prepare (double sampleRate, int maxBlockSize, int numberOfChannels = 2) noexcept;
    
    /** Resets the estimators and active gains. */
    void reset() noexcept;

    /** Sets the RMS estimator smoothing time constant in milliseconds. */
    void setSmoothingTimeMs (double timeMs) noexcept;

    /**
        Processes a block of audio.
        If bypassed is true, wetBlock is overwritten with compensated dryBlock.
        If bypassed is false, wetBlock is left unchanged, but the RMS tracking
        estimators are updated.
    */
    void process (const juce::dsp::AudioBlock<const float>& dryBlock,
                  juce::dsp::AudioBlock<float>& wetBlock,
                  bool bypassed) noexcept;

    /** Returns the current gain compensation factor (linear amplitude). */
    [[nodiscard]] float getGainCompensation() const noexcept { return gainCompensation; }

private:
    struct RMSEstimator
    {
        float alpha = 0.0f;
        float meanSquare = 0.0f;

        void prepare (double sampleRate, double timeMs) noexcept;
        void reset() noexcept;
        float processSample (float sample) noexcept;
        [[nodiscard]] float getRMS() const noexcept;
    };

    static constexpr float epsilon = 1.0e-6f;
    static constexpr float silenceThresholdDb = -70.0f;
    static constexpr float minGain = 0.03162277f; // -30 dB
    static constexpr float maxGain = 31.6227766f; // +30 dB

    double currentSampleRate = 0.0;
    double smoothingTimeMs = 300.0;
    float silenceThreshold = 0.0003162277f; // -70 dB linear

    float gainCompensation = 1.0f;

    std::vector<RMSEstimator> dryEstimators;
    std::vector<RMSEstimator> wetEstimators;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> bypassCrossfade;
};

} // namespace ShredBoost
