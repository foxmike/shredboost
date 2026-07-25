#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>

namespace ShredBoost
{

/**
    1st-order high-pass DC Blocker.

    Implements y_dc(n) = x(n) - x(n-1) + R * y_dc(n-1) with R = 0.9975.
    Keep per-channel x(n-1) and y_dc(n-1) state allocated in prepare.
*/
class DcBlocker final
{
public:
    DcBlocker() noexcept;

    void prepare (double sampleRate, int maxBlockSize) noexcept;
    void reset() noexcept;

    void process (juce::dsp::AudioBlock<float>& block) noexcept;

private:
    struct ChannelState
    {
        float x1 = 0.0f;
        float y1 = 0.0f;
    };

    static constexpr float R = 0.9975f;

    std::vector<ChannelState> channelStates;
    int preparedMaxBlockSize = 0;
};

} // namespace ShredBoost
