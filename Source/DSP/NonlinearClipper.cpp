#include "NonlinearClipper.h"

#include <algorithm>
#include <cmath>

namespace ShredBoost
{

NonlinearClipper::NonlinearClipper() noexcept = default;

void NonlinearClipper::prepare (double sampleRate, int maxBlockSize) noexcept
{
    jassert (sampleRate > 0.0);
    jassert (maxBlockSize > 0);

    crossfadeLengthSamples =
        std::max (1, static_cast<int> (std::round (sampleRate * topologyCrossfadeSeconds)));
    preparedMaxBlockSize = maxBlockSize;
    reset();
}

void NonlinearClipper::reset() noexcept
{
    currentMode = requestedMode.load (std::memory_order_relaxed);
    targetMode = currentMode;
    crossfadePosition = 0;
    isCrossfading = false;
}

void NonlinearClipper::setVoicingMode (int mode) noexcept
{
    requestedMode.store (std::clamp (mode, minimumVoicingMode, maximumVoicingMode),
                         std::memory_order_relaxed);
}

void NonlinearClipper::process (juce::dsp::AudioBlock<float>& block) noexcept
{
    juce::ScopedNoDenormals noDenormals;

    const auto numberOfChannels = block.getNumChannels();
    const auto numberOfSamples = block.getNumSamples();

    jassert (preparedMaxBlockSize > 0);
    jassert (numberOfSamples <= static_cast<std::size_t> (preparedMaxBlockSize));

    if (numberOfChannels == 0 || numberOfSamples == 0)
        return;

    for (std::size_t sampleIndex = 0; sampleIndex < numberOfSamples; ++sampleIndex)
    {
        if (! isCrossfading)
            beginPendingCrossfade();

        if (isCrossfading)
        {
            const auto progress =
                crossfadeLengthSamples == 1
                    ? 1.0f
                    : static_cast<float> (crossfadePosition)
                        / static_cast<float> (crossfadeLengthSamples - 1);
            const auto angle =
                progress * juce::MathConstants<float>::halfPi;
            const auto oldGain = std::cos (angle);
            const auto newGain = std::sin (angle);

            for (std::size_t channel = 0; channel < numberOfChannels; ++channel)
            {
                auto* channelData = block.getChannelPointer (channel);
                const auto input = channelData[sampleIndex];
                const auto oldOutput = applyTopology (input, currentMode);
                const auto newOutput = applyTopology (input, targetMode);
                channelData[sampleIndex] = oldGain * oldOutput + newGain * newOutput;
            }

            ++crossfadePosition;

            if (crossfadePosition >= crossfadeLengthSamples)
            {
                currentMode = targetMode;
                crossfadePosition = 0;
                isCrossfading = false;
            }
        }
        else
        {
            for (std::size_t channel = 0; channel < numberOfChannels; ++channel)
            {
                auto* channelData = block.getChannelPointer (channel);
                channelData[sampleIndex] =
                    applyTopology (channelData[sampleIndex], currentMode);
            }
        }
    }
}

float NonlinearClipper::applyTopology (float input, int mode) noexcept
{
    switch (mode)
    {
        case 0:
            return std::atan (input)
                + 0.15f * input * input * std::exp (-std::abs (input));

        case 1:
            return std::tanh (input);

        case 2:
            return std::tanh ((input >= 0.0f ? 1.0f : 1.8f) * input);

        case 3:
            return input;

        default:
            jassertfalse;
            return input;
    }
}

void NonlinearClipper::beginPendingCrossfade() noexcept
{
    const auto pendingMode = requestedMode.load (std::memory_order_relaxed);

    if (pendingMode == currentMode)
        return;

    targetMode = pendingMode;
    crossfadePosition = 0;
    isCrossfading = true;
}

} // namespace ShredBoost
