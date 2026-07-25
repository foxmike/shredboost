#pragma once

#include <atomic>

#include <juce_dsp/juce_dsp.h>

namespace ShredBoost
{

/**
    Four-topology nonlinear clipper with click-free voicing changes.

    Topology changes are applied by the audio thread using a 15 ms equal-power
    crossfade. Parameter updates only publish the requested mode.
*/
class NonlinearClipper final
{
public:
    NonlinearClipper() noexcept;

    void prepare (double sampleRate, int maxBlockSize) noexcept;
    void reset() noexcept;

    void setVoicingMode (int mode) noexcept;
    void process (juce::dsp::AudioBlock<float>& block) noexcept;

private:
    static_assert (std::atomic<int>::is_always_lock_free);

    static constexpr int minimumVoicingMode = 0;
    static constexpr int maximumVoicingMode = 3;
    static constexpr double topologyCrossfadeSeconds = 0.015;

    static float applyTopology (float input, int mode) noexcept;
    void beginPendingCrossfade() noexcept;

    std::atomic<int> requestedMode { minimumVoicingMode };

    int currentMode = minimumVoicingMode;
    int targetMode = minimumVoicingMode;
    int crossfadeLengthSamples = 1;
    int crossfadePosition = 0;
    int preparedMaxBlockSize = 0;
    bool isCrossfading = false;
};

} // namespace ShredBoost
