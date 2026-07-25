#include "TransientShaper.h"

#include <algorithm>
#include <cmath>

namespace ShredBoost
{

TransientShaper::TransientShaper() noexcept = default;

void TransientShaper::prepare (double sampleRate, int maxBlockSize) noexcept
{
    jassert (sampleRate > 0.0);
    jassert (maxBlockSize > 0);

    fastCoefficient = std::exp (-1.0 / (sampleRate * fastTimeSeconds));
    slowCoefficient = std::exp (-1.0 / (sampleRate * slowTimeSeconds));
    preparedMaxBlockSize = maxBlockSize;

    pickAttackDb.reset (sampleRate, smoothingTimeSeconds);
    reset();
}

void TransientShaper::reset() noexcept
{
    fastEnvelope = 0.0;
    slowEnvelope = 0.0;
}

void TransientShaper::setPickAttackDb (float amountDb) noexcept
{
    pickAttackDb.setTargetValue (
        std::clamp (static_cast<double> (amountDb),
                    minimumPickAttackDb,
                    maximumPickAttackDb));
}

void TransientShaper::process (juce::dsp::AudioBlock<float>& block) noexcept
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
        double detectorInput = 0.0;

        for (std::size_t channel = 0; channel < numberOfChannels; ++channel)
        {
            const auto magnitude =
                std::abs (static_cast<double> (block.getChannelPointer (channel)[sampleIndex]));
            detectorInput = std::max (detectorInput, magnitude);
        }

        fastEnvelope =
            fastCoefficient * fastEnvelope + (1.0 - fastCoefficient) * detectorInput;
        slowEnvelope =
            slowCoefficient * slowEnvelope + (1.0 - slowCoefficient) * detectorInput;

        const auto deltaRel =
            std::clamp ((fastEnvelope - slowEnvelope) / (slowEnvelope + envelopeEpsilon),
                        0.0,
                        maximumRelativeDelta);
        const auto shapedGainDb = pickAttackDb.getNextValue() * deltaRel;
        const auto shapedGain = std::pow (10.0, shapedGainDb / 20.0);

        for (std::size_t channel = 0; channel < numberOfChannels; ++channel)
            block.getChannelPointer (channel)[sampleIndex] *= static_cast<float> (shapedGain);
    }
}

} // namespace ShredBoost
