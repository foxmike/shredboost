#pragma once

#include <juce_dsp/juce_dsp.h>

namespace ShredBoost
{

/**
    Level-independent pick-attack transient shaper.

    A linked fast/slow envelope detector derives a relative attack delta, so
    channels receive the same transient gain and stereo balance is preserved.
*/
class TransientShaper final
{
public:
    TransientShaper() noexcept;

    void prepare (double sampleRate, int maxBlockSize) noexcept;
    void reset() noexcept;

    void setPickAttackDb (float amountDb) noexcept;
    void process (juce::dsp::AudioBlock<float>& block) noexcept;

private:
    using PickAttackSmoother =
        juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear>;

    static constexpr double fastTimeSeconds = 0.001;
    static constexpr double slowTimeSeconds = 0.015;
    static constexpr double smoothingTimeSeconds = 0.02;
    static constexpr double envelopeEpsilon = 1.0e-12;
    static constexpr double maximumRelativeDelta = 2.0;
    static constexpr double minimumPickAttackDb = 0.0;
    static constexpr double maximumPickAttackDb = 9.0;
    static constexpr double defaultPickAttackDb = 2.0;

    PickAttackSmoother pickAttackDb { defaultPickAttackDb };

    double fastCoefficient = 0.0;
    double slowCoefficient = 0.0;
    double fastEnvelope = 0.0;
    double slowEnvelope = 0.0;
    int preparedMaxBlockSize = 0;
};

} // namespace ShredBoost
