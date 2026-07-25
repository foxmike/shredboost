#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace ShredBoost
{

/**
    Second-order peaking filter for the pre-clip mid voicing controls.

    MidVoicingFilter processes one mono stream. The surrounding audio callback
    should use juce::ScopedNoDenormals; processBlock also establishes that
    protection when the filter is used independently.
*/
class MidVoicingFilter final
{
public:
    MidVoicingFilter() noexcept;

    void prepare (double sampleRate) noexcept;
    void reset() noexcept;

    void setFrequency (float frequencyHz) noexcept;
    void setAmount (float amountDb) noexcept;

    [[nodiscard]] float process (float sample) noexcept;
    void processBlock (float* samples, int numberOfSamples) noexcept;

private:
    using FrequencySmoother =
        juce::SmoothedValue<double, juce::ValueSmoothingTypes::Multiplicative>;
    using AmountSmoother =
        juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear>;

    void updateCoefficients (double frequencyHz, double amountDb) noexcept;
    void flushState() noexcept;

    static constexpr double minimumFrequencyHz = 500.0;
    static constexpr double maximumFrequencyHz = 1200.0;
    static constexpr double defaultFrequencyHz = 720.0;
    static constexpr double minimumAmountDb = 0.0;
    static constexpr double maximumAmountDb = 9.0;
    static constexpr double defaultAmountDb = 4.0;
    static constexpr double midQ = 0.9;
    static constexpr double smoothingTimeSeconds = 0.02;
    static constexpr double stateFlushThreshold = 1.0e-24;

    FrequencySmoother frequency { defaultFrequencyHz };
    AmountSmoother amount { defaultAmountDb };

    double currentSampleRate = 0.0;
    double b0 = 1.0;
    double b1 = 0.0;
    double b2 = 0.0;
    double a1 = 0.0;
    double a2 = 0.0;
    double state1 = 0.0;
    double state2 = 0.0;
};

} // namespace ShredBoost
