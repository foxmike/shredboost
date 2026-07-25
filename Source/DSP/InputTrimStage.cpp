#include "InputTrimStage.h"

#include <algorithm>
#include <cmath>

namespace ShredBoost
{

InputTrimStage::InputTrimStage() noexcept = default;

void InputTrimStage::prepare (double sampleRate) noexcept
{
    jassert (sampleRate > 0.0);
    trimDb.reset (sampleRate, smoothingTimeSeconds);
    reset();
}

void InputTrimStage::reset() noexcept
{
    peakLevel.store (0.0f, std::memory_order_relaxed);
    rmsLevel.store (0.0f, std::memory_order_relaxed);
}

void InputTrimStage::setTrimDb (float trimDbVal) noexcept
{
    trimDb.setTargetValue (
        std::clamp (static_cast<double> (trimDbVal), minimumTrimDb, maximumTrimDb));
}

float InputTrimStage::process (float sample) noexcept
{
    const float currentTrimDb = static_cast<float> (trimDb.getNextValue());
    const float gain = std::pow (10.0f, currentTrimDb / 20.0f);
    return sample * gain;
}

void InputTrimStage::processBlock (float* samples, int numberOfSamples) noexcept
{
    jassert (samples != nullptr || numberOfSamples == 0);
    jassert (numberOfSamples >= 0);

    juce::ScopedNoDenormals noDenormals;

    if (numberOfSamples <= 0)
    {
        peakLevel.store (0.0f, std::memory_order_relaxed);
        rmsLevel.store (0.0f, std::memory_order_relaxed);
        return;
    }

    float sumSquares = 0.0f;
    float maxPeak = 0.0f;

    if (trimDb.isSmoothing())
    {
        for (int i = 0; i < numberOfSamples; ++i)
        {
            const float currentTrimDb = static_cast<float> (trimDb.getNextValue());
            const float gain = std::pow (10.0f, currentTrimDb / 20.0f);
            
            samples[i] *= gain;
            
            const float absVal = std::abs (samples[i]);
            if (absVal > maxPeak)
                maxPeak = absVal;
            
            sumSquares += samples[i] * samples[i];
        }
    }
    else
    {
        const float currentTrimDb = static_cast<float> (trimDb.getCurrentValue());
        const float gain = std::pow (10.0f, currentTrimDb / 20.0f);
        
        for (int i = 0; i < numberOfSamples; ++i)
        {
            samples[i] *= gain;
            
            const float absVal = std::abs (samples[i]);
            if (absVal > maxPeak)
                maxPeak = absVal;
            
            sumSquares += samples[i] * samples[i];
        }
    }

    const float rms = std::sqrt (sumSquares / static_cast<float> (numberOfSamples));

    peakLevel.store (maxPeak, std::memory_order_relaxed);
    rmsLevel.store (rms, std::memory_order_relaxed);
}

float InputTrimStage::getPeakLevel() const noexcept
{
    return peakLevel.load (std::memory_order_relaxed);
}

float InputTrimStage::getRmsLevel() const noexcept
{
    return rmsLevel.load (std::memory_order_relaxed);
}

} // namespace ShredBoost
