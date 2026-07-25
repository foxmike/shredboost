#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace ShredBoost
{

/**
    Second-order high-pass filter for the pre-drive tight control.

    TightFilter processes one mono stream. The surrounding audio callback should
    use juce::ScopedNoDenormals; processBlock also establishes that protection
    when the filter is used independently.
*/
class TightFilter final
{
public:
    TightFilter() noexcept;

    void prepare (double sampleRate) noexcept;
    void reset() noexcept;

    void setCutoff (float cutoffHz) noexcept;
    void setResonantMode (bool shouldUsePrecisionTightMode) noexcept;

    [[nodiscard]] float process (float sample) noexcept;
    void processBlock (float* samples, int numberOfSamples) noexcept;

private:
    using CutoffSmoother =
        juce::SmoothedValue<double, juce::ValueSmoothingTypes::Multiplicative>;

    void updateCoefficients (double cutoffHz) noexcept;
    void flushState() noexcept;

    static constexpr double minimumCutoffHz = 80.0;
    static constexpr double maximumCutoffHz = 400.0;
    static constexpr double defaultCutoffHz = 180.0;
    static constexpr double butterworthQ = 0.7071067811865476;
    static constexpr double precisionTightQ = 1.2;
    static constexpr double smoothingTimeSeconds = 0.02;
    static constexpr double stateFlushThreshold = 1.0e-24;

    CutoffSmoother cutoff { defaultCutoffHz };

    double currentSampleRate = 0.0;
    double coefficientCutoffHz = 0.0;
    double b0 = 1.0;
    double b1 = 0.0;
    double b2 = 0.0;
    double a1 = 0.0;
    double a2 = 0.0;
    double state1 = 0.0;
    double state2 = 0.0;
    bool resonantMode = false;
};

} // namespace ShredBoost
