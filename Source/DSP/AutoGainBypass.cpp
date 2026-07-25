#include "AutoGainBypass.h"
#include <algorithm>
#include <cmath>

namespace ShredBoost
{

// =============================================================================
// RMSEstimator Implementation
// =============================================================================

void AutoGainBypass::RMSEstimator::prepare (double sampleRate, double timeMs) noexcept
{
    if (sampleRate > 0.0 && timeMs > 0.0)
    {
        const double tau = timeMs * 0.001;
        alpha = static_cast<float> (1.0 - std::exp (-1.0 / (sampleRate * tau)));
    }
    else
    {
        alpha = 0.0f;
    }
    reset();
}

void AutoGainBypass::RMSEstimator::reset() noexcept
{
    meanSquare = 0.0f;
}

float AutoGainBypass::RMSEstimator::processSample (float sample) noexcept
{
    const float squared = sample * sample;
    meanSquare += alpha * (squared - meanSquare);

    // Snap to zero to prevent denormals
    if (meanSquare < 1.0e-18f)
        meanSquare = 0.0f;

    return getRMS();
}

float AutoGainBypass::RMSEstimator::getRMS() const noexcept
{
    return std::sqrt (meanSquare);
}

// =============================================================================
// AutoGainBypass Implementation
// =============================================================================

AutoGainBypass::AutoGainBypass() noexcept
{
    silenceThreshold = std::pow (10.0f, silenceThresholdDb / 20.0f);
}

void AutoGainBypass::prepare (double sampleRate, int maxBlockSize, int numberOfChannels) noexcept
{
    juce::ignoreUnused (maxBlockSize);
    
    currentSampleRate = sampleRate;
    const int numChannels = std::max (1, numberOfChannels);

    dryEstimators.assign (static_cast<std::size_t> (numChannels), RMSEstimator());
    wetEstimators.assign (static_cast<std::size_t> (numChannels), RMSEstimator());

    for (int i = 0; i < numChannels; ++i)
    {
        dryEstimators[static_cast<std::size_t> (i)].prepare (sampleRate, smoothingTimeMs);
        wetEstimators[static_cast<std::size_t> (i)].prepare (sampleRate, smoothingTimeMs);
    }

    bypassCrossfade.reset (sampleRate, 0.02); // 20 ms crossfade
    reset();
}

void AutoGainBypass::reset() noexcept
{
    for (auto& est : dryEstimators)
        est.reset();
    for (auto& est : wetEstimators)
        est.reset();

    gainCompensation = 1.0f;
    bypassCrossfade.setCurrentAndTargetValue (0.0f);
}

void AutoGainBypass::setSmoothingTimeMs (double timeMs) noexcept
{
    smoothingTimeMs = std::max (1.0, timeMs);
    if (currentSampleRate > 0.0)
    {
        for (auto& est : dryEstimators)
            est.prepare (currentSampleRate, smoothingTimeMs);
        for (auto& est : wetEstimators)
            est.prepare (currentSampleRate, smoothingTimeMs);
    }
}

void AutoGainBypass::process (const juce::dsp::AudioBlock<const float>& dryBlock,
                              juce::dsp::AudioBlock<float>& wetBlock,
                              bool bypassed) noexcept
{
    juce::ScopedNoDenormals noDenormals;

    const auto numDryChannels = dryBlock.getNumChannels();
    const auto numWetChannels = wetBlock.getNumChannels();
    const auto numSamples = wetBlock.getNumSamples();
    const auto maxChannels = std::max (numDryChannels, numWetChannels);

    jassert (maxChannels <= dryEstimators.size());
    jassert (maxChannels <= wetEstimators.size());

    if (numDryChannels == 0 || numWetChannels == 0 || numSamples == 0)
        return;

    // Fast path 1: Active and not transitioning
    if (! bypassed && bypassCrossfade.getCurrentValue() <= 0.0f)
    {
        for (size_t sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
        {
            float sumRmsDry = 0.0f;
            float sumRmsWet = 0.0f;

            for (size_t channel = 0; channel < maxChannels; ++channel)
            {
                const size_t dryCh = std::min (channel, numDryChannels - 1);
                const size_t wetCh = std::min (channel, numWetChannels - 1);

                const float dryVal = dryBlock.getSample (dryCh, sampleIndex);
                const float wetVal = wetBlock.getSample (wetCh, sampleIndex);

                dryEstimators[channel].processSample (dryVal);
                wetEstimators[channel].processSample (wetVal);

                sumRmsDry += dryEstimators[channel].getRMS();
                sumRmsWet += wetEstimators[channel].getRMS();
            }

            const float avgRmsDry = sumRmsDry / static_cast<float> (maxChannels);
            const float avgRmsWet = sumRmsWet / static_cast<float> (maxChannels);

            if (avgRmsDry >= silenceThreshold)
            {
                const float targetGain = avgRmsWet / (avgRmsDry + epsilon);
                gainCompensation = std::clamp (targetGain, minGain, maxGain);
            }
        }

        bypassCrossfade.setTargetValue (0.0f);
        return;
    }

    // Fast path 2: Bypassed and not transitioning
    if (bypassed && bypassCrossfade.getCurrentValue() >= 1.0f)
    {
        for (size_t channel = 0; channel < numWetChannels; ++channel)
        {
            const size_t dryCh = std::min (channel, numDryChannels - 1);
            auto* wetData = wetBlock.getChannelPointer (channel);
            const auto* dryData = dryBlock.getChannelPointer (dryCh);

            for (size_t sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
            {
                wetData[sampleIndex] = dryData[sampleIndex] * gainCompensation;
            }
        }

        bypassCrossfade.setTargetValue (1.0f);
        return;
    }

    // Slow path: Transitioning bypass state (crossfading)
    bypassCrossfade.setTargetValue (bypassed ? 1.0f : 0.0f);

    for (size_t sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        const float mix = bypassCrossfade.getNextValue();

        if (! bypassed)
        {
            float sumRmsDry = 0.0f;
            float sumRmsWet = 0.0f;

            for (size_t channel = 0; channel < maxChannels; ++channel)
            {
                const size_t dryCh = std::min (channel, numDryChannels - 1);
                const size_t wetCh = std::min (channel, numWetChannels - 1);

                const float dryVal = dryBlock.getSample (dryCh, sampleIndex);
                const float wetVal = wetBlock.getSample (wetCh, sampleIndex);

                dryEstimators[channel].processSample (dryVal);
                wetEstimators[channel].processSample (wetVal);

                sumRmsDry += dryEstimators[channel].getRMS();
                sumRmsWet += wetEstimators[channel].getRMS();
            }

            const float avgRmsDry = sumRmsDry / static_cast<float> (maxChannels);
            const float avgRmsWet = sumRmsWet / static_cast<float> (maxChannels);

            if (avgRmsDry >= silenceThreshold)
            {
                const float targetGain = avgRmsWet / (avgRmsDry + epsilon);
                gainCompensation = std::clamp (targetGain, minGain, maxGain);
            }
        }

        for (size_t channel = 0; channel < numWetChannels; ++channel)
        {
            const size_t dryCh = std::min (channel, numDryChannels - 1);
            float* wetData = wetBlock.getChannelPointer (channel);
            const float* dryData = dryBlock.getChannelPointer (dryCh);

            const float wetSample = wetData[sampleIndex];
            const float compensatedDrySample = dryData[sampleIndex] * gainCompensation;

            wetData[sampleIndex] = (1.0f - mix) * wetSample + mix * compensatedDrySample;
        }
    }
}

} // namespace ShredBoost
