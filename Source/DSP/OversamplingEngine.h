#pragma once

#include <juce_dsp/juce_dsp.h>

#include <array>
#include <atomic>
#include <memory>

namespace ShredBoost
{

/**
    Polyphase FIR half-band oversampling for the nonlinear processing path.

    prepare() constructs and sizes every supported oversampling path. Factor
    changes and audio processing only select and use those preallocated paths.
*/
class OversamplingEngine final
{
public:
    enum class Factor
    {
        one = 1,
        two = 2,
        four = 4
    };

    OversamplingEngine() noexcept;

    void prepare (double sampleRate, int maximumBlockSize, int numberOfChannels = 2);
    void reset() noexcept;

    void setOversamplingFactor (int factor) noexcept;
    void setOversamplingFactor (Factor factor) noexcept;

    [[nodiscard]] int getOversamplingFactor() const noexcept;
    [[nodiscard]] int getLatencySamples() const noexcept;

    [[nodiscard]] juce::dsp::AudioBlock<float>
    upsample (const juce::dsp::AudioBlock<const float>& inputBlock) noexcept;

    void downsample (juce::dsp::AudioBlock<float>& outputBlock) noexcept;

private:
    using Oversampler = juce::dsp::Oversampling<float>;

    static constexpr std::size_t numberOfFactors = 3;

    [[nodiscard]] static int factorToIndex (int factor) noexcept;
    [[nodiscard]] static int indexToFactor (int index) noexcept;

    std::array<std::unique_ptr<Oversampler>, numberOfFactors> oversamplers;
    std::atomic<int> selectedIndex { 0 };

    int activeProcessingIndex = 0;
    int preparedMaximumBlockSize = 0;
    int preparedNumberOfChannels = 0;
};

} // namespace ShredBoost
