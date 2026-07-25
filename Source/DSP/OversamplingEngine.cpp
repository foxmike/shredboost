#include "OversamplingEngine.h"

namespace ShredBoost
{

OversamplingEngine::OversamplingEngine() noexcept = default;

void OversamplingEngine::prepare (double sampleRate,
                                  int maximumBlockSize,
                                  int numberOfChannels)
{
    jassert (sampleRate > 0.0);
    jassert (maximumBlockSize > 0);
    jassert (numberOfChannels > 0);

    juce::ignoreUnused (sampleRate);

    if (maximumBlockSize <= 0 || numberOfChannels <= 0)
        return;

    std::array<std::unique_ptr<Oversampler>, numberOfFactors> preparedOversamplers;

    for (std::size_t index = 0; index < preparedOversamplers.size(); ++index)
    {
        preparedOversamplers[index] = std::make_unique<Oversampler> (
            static_cast<std::size_t> (numberOfChannels),
            index,
            Oversampler::filterHalfBandFIREquiripple,
            true,
            true);

        preparedOversamplers[index]->initProcessing (
            static_cast<std::size_t> (maximumBlockSize));
    }

    oversamplers = std::move (preparedOversamplers);
    preparedMaximumBlockSize = maximumBlockSize;
    preparedNumberOfChannels = numberOfChannels;
    activeProcessingIndex = selectedIndex.load (std::memory_order_relaxed);
}

void OversamplingEngine::reset() noexcept
{
    for (auto& oversampler : oversamplers)
        if (oversampler != nullptr)
            oversampler->reset();
}

void OversamplingEngine::setOversamplingFactor (int factor) noexcept
{
    const auto index = factorToIndex (factor);

    jassert (index >= 0);

    if (index >= 0)
        selectedIndex.store (index, std::memory_order_relaxed);
}

void OversamplingEngine::setOversamplingFactor (Factor factor) noexcept
{
    setOversamplingFactor (static_cast<int> (factor));
}

int OversamplingEngine::getOversamplingFactor() const noexcept
{
    return indexToFactor (selectedIndex.load (std::memory_order_relaxed));
}

int OversamplingEngine::getLatencySamples() const noexcept
{
    const auto index = selectedIndex.load (std::memory_order_relaxed);
    const auto& oversampler = oversamplers[static_cast<std::size_t> (index)];

    if (oversampler == nullptr)
        return 0;

    return juce::roundToInt (oversampler->getLatencyInSamples());
}

juce::dsp::AudioBlock<float>
OversamplingEngine::upsample (
    const juce::dsp::AudioBlock<const float>& inputBlock) noexcept
{
    jassert (preparedMaximumBlockSize > 0);
    jassert (inputBlock.getNumSamples()
             <= static_cast<std::size_t> (preparedMaximumBlockSize));
    jassert (inputBlock.getNumChannels()
             <= static_cast<std::size_t> (preparedNumberOfChannels));

    activeProcessingIndex = selectedIndex.load (std::memory_order_relaxed);
    auto& oversampler = oversamplers[static_cast<std::size_t> (activeProcessingIndex)];

    if (oversampler == nullptr
        || inputBlock.getNumSamples()
               > static_cast<std::size_t> (preparedMaximumBlockSize)
        || inputBlock.getNumChannels()
               > static_cast<std::size_t> (preparedNumberOfChannels))
        return {};

    return oversampler->processSamplesUp (inputBlock);
}

void OversamplingEngine::downsample (
    juce::dsp::AudioBlock<float>& outputBlock) noexcept
{
    jassert (preparedMaximumBlockSize > 0);
    jassert (outputBlock.getNumSamples()
             <= static_cast<std::size_t> (preparedMaximumBlockSize));
    jassert (outputBlock.getNumChannels()
             <= static_cast<std::size_t> (preparedNumberOfChannels));

    auto& oversampler = oversamplers[static_cast<std::size_t> (activeProcessingIndex)];

    if (oversampler == nullptr
        || outputBlock.getNumSamples()
               > static_cast<std::size_t> (preparedMaximumBlockSize)
        || outputBlock.getNumChannels()
               > static_cast<std::size_t> (preparedNumberOfChannels))
        return;

    oversampler->processSamplesDown (outputBlock);
}

int OversamplingEngine::factorToIndex (int factor) noexcept
{
    switch (factor)
    {
        case 1: return 0;
        case 2: return 1;
        case 4: return 2;
        default: return -1;
    }
}

int OversamplingEngine::indexToFactor (int index) noexcept
{
    return 1 << index;
}

} // namespace ShredBoost
