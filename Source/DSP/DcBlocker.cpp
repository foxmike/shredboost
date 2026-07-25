#include "DcBlocker.h"

#include <algorithm>

namespace ShredBoost
{

DcBlocker::DcBlocker() noexcept = default;

void DcBlocker::prepare (double sampleRate, int maxBlockSize) noexcept
{
    juce::ignoreUnused (sampleRate);
    jassert (maxBlockSize > 0);

    preparedMaxBlockSize = maxBlockSize;

    // Allocate state for 8 channels to handle mono/stereo and potential multi-channel setups
    // without performing real-time heap allocations.
    channelStates.assign (8, ChannelState());
    reset();
}

void DcBlocker::reset() noexcept
{
    for (auto& state : channelStates)
    {
        state.x1 = 0.0f;
        state.y1 = 0.0f;
    }
}

void DcBlocker::process (juce::dsp::AudioBlock<float>& block) noexcept
{
    juce::ScopedNoDenormals noDenormals;

    const auto numberOfChannels = block.getNumChannels();
    const auto numberOfSamples = block.getNumSamples();

    jassert (preparedMaxBlockSize > 0);
    jassert (numberOfSamples <= static_cast<std::size_t> (preparedMaxBlockSize));

    if (numberOfChannels == 0 || numberOfSamples == 0)
        return;

    const auto channelsToProcess = std::min (numberOfChannels, channelStates.size());
    jassert (numberOfChannels <= channelStates.size());

    for (std::size_t channel = 0; channel < channelsToProcess; ++channel)
    {
        auto* channelData = block.getChannelPointer (channel);
        auto& state = channelStates[channel];

        for (std::size_t sampleIndex = 0; sampleIndex < numberOfSamples; ++sampleIndex)
        {
            const float x = channelData[sampleIndex];
            const float y = x - state.x1 + R * state.y1;

            state.x1 = x;
            state.y1 = y;

            channelData[sampleIndex] = y;
        }
    }
}

} // namespace ShredBoost
