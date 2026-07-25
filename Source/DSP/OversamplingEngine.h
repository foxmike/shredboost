#pragma once

#include <juce_dsp/juce_dsp.h>

#include <array>
#include <atomic>
#include <memory>

namespace ShredBoost
{

/**
    Polyphase FIR half-band oversampling for the nonlinear processing path.

    Factor changes are prepared on the message thread in an inactive slot.
    The audio thread fades out over 10 ms, adopts the prepared slot at silence,
    and fades back in. No construction or buffer allocation takes place in
    upsample() or downsample().
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

    /** Requests and prepares a factor change. Call this on the message thread. */
    void setOversamplingFactor (int factor);
    void setOversamplingFactor (Factor factor);

    /** Records a desired factor without allocating. Safe to call from any thread. */
    void requestFactorChange (int factor) noexcept;
    void requestFactorChange (Factor factor) noexcept;

    /**
        Constructs and initialises a pending oversampler. Call on the message
        thread after requestFactorChange(). Returns true when a new path was
        prepared for the audio-thread mute-fade handoff.
    */
    bool applyPendingFactor();

    [[nodiscard]] bool isFactorChangePending() const noexcept;

    [[nodiscard]] int getOversamplingFactor() const noexcept;
    [[nodiscard]] int getLatencySamples() const noexcept;

    [[nodiscard]] juce::dsp::AudioBlock<float>
    upsample (const juce::dsp::AudioBlock<const float>& inputBlock) noexcept;

    void downsample (juce::dsp::AudioBlock<float>& outputBlock) noexcept;

private:
    using Oversampler = juce::dsp::Oversampling<float>;

    static constexpr std::size_t numberOfSlots = 2;
    static constexpr double muteFadeSeconds = 0.01;

    [[nodiscard]] static int factorToIndex (int factor) noexcept;
    [[nodiscard]] static int indexToFactor (int index) noexcept;
    [[nodiscard]] std::unique_ptr<Oversampler>
    createPreparedOversampler (int factorIndex) const;
    void applyMuteFade (juce::dsp::AudioBlock<float>& outputBlock) noexcept;

    std::array<std::unique_ptr<Oversampler>, numberOfSlots> oversamplers;
    std::array<int, numberOfSlots> slotFactorIndices { 0, 0 };
    std::array<int, numberOfSlots> slotLatencies { 0, 0 };

    std::atomic<int> requestedFactorIndex { 0 };
    std::atomic<int> appliedFactorIndex { 0 };
    std::atomic<int> activeSlot { 0 };
    std::atomic<int> preparedPendingSlot { -1 };
    std::atomic<int> reportedLatencySamples { 0 };

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> muteGain { 1.0f };
    int activeProcessingSlot = 0;
    int preparedMaximumBlockSize = 0;
    int preparedNumberOfChannels = 0;
};

} // namespace ShredBoost
