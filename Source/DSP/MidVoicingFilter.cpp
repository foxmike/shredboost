#include "MidVoicingFilter.h"

#include <algorithm>
#include <cmath>

namespace ShredBoost
{

MidVoicingFilter::MidVoicingFilter() noexcept = default;

void MidVoicingFilter::prepare (double sampleRate) noexcept
{
    jassert (sampleRate > 0.0);

    currentSampleRate = sampleRate;
    frequency.reset (sampleRate, smoothingTimeSeconds);
    amount.reset (sampleRate, smoothingTimeSeconds);
    reset();
    updateCoefficients (frequency.getCurrentValue(), amount.getCurrentValue());
}

void MidVoicingFilter::reset() noexcept
{
    state1 = 0.0;
    state2 = 0.0;
}

void MidVoicingFilter::setFrequency (float frequencyHz) noexcept
{
    frequency.setTargetValue (
        std::clamp (static_cast<double> (frequencyHz),
                    minimumFrequencyHz,
                    maximumFrequencyHz));
}

void MidVoicingFilter::setAmount (float amountDb) noexcept
{
    amount.setTargetValue (
        std::clamp (static_cast<double> (amountDb), minimumAmountDb, maximumAmountDb));
}

float MidVoicingFilter::process (float sample) noexcept
{
    if (frequency.isSmoothing() || amount.isSmoothing())
        updateCoefficients (frequency.getNextValue(), amount.getNextValue());

    const auto input = static_cast<double> (sample);
    const auto output = b0 * input + state1;

    state1 = b1 * input - a1 * output + state2;
    state2 = b2 * input - a2 * output;
    flushState();

    return static_cast<float> (output);
}

void MidVoicingFilter::processBlock (float* samples, int numberOfSamples) noexcept
{
    jassert (samples != nullptr || numberOfSamples == 0);
    jassert (numberOfSamples >= 0);

    juce::ScopedNoDenormals noDenormals;

    for (int sampleIndex = 0; sampleIndex < numberOfSamples; ++sampleIndex)
        samples[sampleIndex] = process (samples[sampleIndex]);
}

void MidVoicingFilter::updateCoefficients (double frequencyHz, double amountDb) noexcept
{
    jassert (currentSampleRate > 0.0);

    const auto clampedFrequency =
        std::clamp (frequencyHz, minimumFrequencyHz, maximumFrequencyHz);
    const auto clampedAmount =
        std::clamp (amountDb, minimumAmountDb, maximumAmountDb);

    const auto amplitude = std::pow (10.0, clampedAmount / 40.0);
    const auto angularFrequency =
        2.0 * juce::MathConstants<double>::pi * clampedFrequency / currentSampleRate;
    const auto cosine = std::cos (angularFrequency);
    const auto alpha = std::sin (angularFrequency) / (2.0 * midQ);

    // RBJ Audio EQ Cookbook second-order peaking-EQ coefficients.
    const auto a0 = 1.0 + alpha / amplitude;
    const auto inverseA0 = 1.0 / a0;
    b0 = (1.0 + alpha * amplitude) * inverseA0;
    b1 = -2.0 * cosine * inverseA0;
    b2 = (1.0 - alpha * amplitude) * inverseA0;
    a1 = -2.0 * cosine * inverseA0;
    a2 = (1.0 - alpha / amplitude) * inverseA0;
}

void MidVoicingFilter::flushState() noexcept
{
    if (std::abs (state1) < stateFlushThreshold)
        state1 = 0.0;

    if (std::abs (state2) < stateFlushThreshold)
        state2 = 0.0;
}

} // namespace ShredBoost
