#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

namespace ShredBoost
{

/**
    Input trim / calibration stage at chain position [0].

    Applies a smoothed gain Gin = 10^(trim/20) driven by a linear SmoothedValue
    in the dB domain, and provides lock-free telemetry metering for peak and RMS levels.
*/
class InputTrimStage final
{
public:
    InputTrimStage() noexcept;

    void prepare (double sampleRate) noexcept;
    void reset() noexcept;

    void setTrimDb (float trimDb) noexcept;

    [[nodiscard]] float process (float sample) noexcept;
    void processBlock (float* samples, int numberOfSamples) noexcept;

    [[nodiscard]] float getPeakLevel() const noexcept;
    [[nodiscard]] float getRmsLevel() const noexcept;

private:
    using TrimSmoother = juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear>;

    static constexpr double minimumTrimDb = -18.0;
    static constexpr double maximumTrimDb = 18.0;
    static constexpr double defaultTrimDb = 0.0;
    static constexpr double smoothingTimeSeconds = 0.02;

    TrimSmoother trimDb { defaultTrimDb };

    std::atomic<float> peakLevel { 0.0f };
    std::atomic<float> rmsLevel { 0.0f };
};

} // namespace ShredBoost
