#include "ToneFilter.h"

#include <algorithm>
#include <cmath>

namespace ShredBoost
{

ToneFilter::ToneFilter() noexcept = default;

void ToneFilter::prepare (double sampleRate, int maxBlockSize) noexcept
{
    jassert (sampleRate > 0.0);
    jassert (maxBlockSize > 0);

    currentSampleRate = sampleRate;
    preparedMaxBlockSize = maxBlockSize;

    cutoff.reset (sampleRate, smoothingTimeSeconds);

    // Pre-allocate states for up to 8 channels to handle mono/stereo/surround
    // without performing real-time heap allocations.
    channelStates.assign (8, ChannelState());

    reset();
    updateCoefficients (cutoff.getCurrentValue());
}

void ToneFilter::reset() noexcept
{
    for (auto& state : channelStates)
    {
        state.s1 = 0.0;
        state.s2 = 0.0;
    }
}

void ToneFilter::setCutoff (float cutoffHz) noexcept
{
    cutoff.setTargetValue (
        std::clamp (static_cast<double> (cutoffHz), minimumCutoffHz, maximumCutoffHz));
}

void ToneFilter::process (juce::dsp::AudioBlock<float>& block) noexcept
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

    for (std::size_t sampleIndex = 0; sampleIndex < numberOfSamples; ++sampleIndex)
    {
        if (cutoff.isSmoothing())
            updateCoefficients (cutoff.getNextValue());

        for (std::size_t channel = 0; channel < channelsToProcess; ++channel)
        {
            auto* channelData = block.getChannelPointer (channel);
            auto& state = channelStates[channel];

            const double input = static_cast<double> (channelData[sampleIndex]);
            const double output = b0 * input + state.s1;

            state.s1 = b1 * input - a1 * output + state.s2;
            state.s2 = b2 * input - a2 * output;

            // Flush state to avoid denormal numbers
            if (std::abs (state.s1) < stateFlushThreshold)
                state.s1 = 0.0;

            if (std::abs (state.s2) < stateFlushThreshold)
                state.s2 = 0.0;

            channelData[sampleIndex] = static_cast<float> (output);
        }
    }
}

void ToneFilter::updateCoefficients (double cutoffHz) noexcept
{
    jassert (currentSampleRate > 0.0);

    const auto clampedCutoff = std::clamp (cutoffHz, minimumCutoffHz, maximumCutoffHz);
    const auto angularFrequency =
        2.0 * juce::MathConstants<double>::pi * clampedCutoff / currentSampleRate;
    const auto cosine = std::cos (angularFrequency);
    const auto alpha = std::sin (angularFrequency) / (2.0 * butterworthQ);

    const auto a0 = 1.0 + alpha;
    const auto inverseA0 = 1.0 / a0;

    b0 = 0.5 * (1.0 - cosine) * inverseA0;
    b1 = (1.0 - cosine) * inverseA0;
    b2 = b0;
    a1 = -2.0 * cosine * inverseA0;
    a2 = (1.0 - alpha) * inverseA0;
}

} // namespace ShredBoost
