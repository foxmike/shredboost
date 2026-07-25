#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>

namespace ShredBoost
{

/**
    2nd-order post-drive Tone Low-Pass Filter.

    An RBJ biquad LPF with cutoff adjustable in the 1-16 kHz range.
    Cutoff frequency smoothing is performed using a multiplicative juce::SmoothedValue.
*/
class ToneFilter final
{
public:
    ToneFilter() noexcept;

    void prepare (double sampleRate, int maxBlockSize) noexcept;
    void reset() noexcept;

    void setCutoff (float cutoffHz) noexcept;
    void process (juce::dsp::AudioBlock<float>& block) noexcept;

private:
    using CutoffSmoother =
        juce::SmoothedValue<double, juce::ValueSmoothingTypes::Multiplicative>;

    struct ChannelState
    {
        double s1 = 0.0;
        double s2 = 0.0;
    };

    void updateCoefficients (double cutoffHz) noexcept;

    static constexpr double minimumCutoffHz = 1000.0;
    static constexpr double maximumCutoffHz = 16000.0;
    static constexpr double defaultCutoffHz = 6500.0;
    static constexpr double butterworthQ = 0.7071067811865476;
    static constexpr double smoothingTimeSeconds = 0.02;
    static constexpr double stateFlushThreshold = 1.0e-24;

    CutoffSmoother cutoff { defaultCutoffHz };

    double currentSampleRate = 0.0;
    int preparedMaxBlockSize = 0;

    double b0 = 1.0;
    double b1 = 0.0;
    double b2 = 0.0;
    double a1 = 0.0;
    double a2 = 0.0;

    std::vector<ChannelState> channelStates;
};

} // namespace ShredBoost
