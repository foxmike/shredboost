#include "TightFilter.h"

#include <algorithm>
#include <cmath>

namespace ShredBoost
{

TightFilter::TightFilter() noexcept = default;

void TightFilter::prepare (double sampleRate) noexcept
{
    jassert (sampleRate > 0.0);

    currentSampleRate = sampleRate;
    cutoff.reset (sampleRate, smoothingTimeSeconds);
    reset();
    updateCoefficients (cutoff.getCurrentValue());
}

void TightFilter::reset() noexcept
{
    state1 = 0.0;
    state2 = 0.0;
}

void TightFilter::setCutoff (float cutoffHz) noexcept
{
    cutoff.setTargetValue (
        std::clamp (static_cast<double> (cutoffHz), minimumCutoffHz, maximumCutoffHz));
}

void TightFilter::setResonantMode (bool shouldUsePrecisionTightMode) noexcept
{
    if (resonantMode == shouldUsePrecisionTightMode)
        return;

    resonantMode = shouldUsePrecisionTightMode;

    if (currentSampleRate > 0.0)
        updateCoefficients (cutoff.getCurrentValue());
}

float TightFilter::process (float sample) noexcept
{
    if (cutoff.isSmoothing())
        updateCoefficients (cutoff.getNextValue());

    const auto input = static_cast<double> (sample);
    const auto output = b0 * input + state1;

    state1 = b1 * input - a1 * output + state2;
    state2 = b2 * input - a2 * output;
    flushState();

    return static_cast<float> (output);
}

void TightFilter::processBlock (float* samples, int numberOfSamples) noexcept
{
    jassert (samples != nullptr || numberOfSamples == 0);
    jassert (numberOfSamples >= 0);

    juce::ScopedNoDenormals noDenormals;

    for (int sampleIndex = 0; sampleIndex < numberOfSamples; ++sampleIndex)
        samples[sampleIndex] = process (samples[sampleIndex]);
}

void TightFilter::updateCoefficients (double cutoffHz) noexcept
{
    jassert (currentSampleRate > 0.0);

    coefficientCutoffHz = std::clamp (cutoffHz, minimumCutoffHz, maximumCutoffHz);

    const auto q = resonantMode ? precisionTightQ : butterworthQ;
    const auto angularFrequency =
        2.0 * juce::MathConstants<double>::pi * coefficientCutoffHz / currentSampleRate;
    const auto cosine = std::cos (angularFrequency);
    const auto alpha = std::sin (angularFrequency) / (2.0 * q);

    // RBJ Audio EQ Cookbook second-order high-pass coefficients.
    const auto a0 = 1.0 + alpha;
    const auto inverseA0 = 1.0 / a0;
    b0 = 0.5 * (1.0 + cosine) * inverseA0;
    b1 = -(1.0 + cosine) * inverseA0;
    b2 = b0;
    a1 = -2.0 * cosine * inverseA0;
    a2 = (1.0 - alpha) * inverseA0;
}

void TightFilter::flushState() noexcept
{
    if (std::abs (state1) < stateFlushThreshold)
        state1 = 0.0;

    if (std::abs (state2) < stateFlushThreshold)
        state2 = 0.0;
}

} // namespace ShredBoost
