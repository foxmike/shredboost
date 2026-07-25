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

    if (sampleRate <= 0.0 || maximumBlockSize <= 0 || numberOfChannels <= 0)
        return;

    preparedMaximumBlockSize = maximumBlockSize;
    preparedNumberOfChannels = numberOfChannels;

    const auto initialFactorIndex =
        requestedFactorIndex.load (std::memory_order_relaxed);

    oversamplers[0] = createPreparedOversampler (initialFactorIndex);
    oversamplers[1].reset();
    slotFactorIndices[0] = initialFactorIndex;
    slotLatencies[0] = oversamplers[0] != nullptr
                           ? juce::roundToInt (
                                 oversamplers[0]->getLatencyInSamples())
                           : 0;

    activeSlot.store (0, std::memory_order_relaxed);
    preparedPendingSlot.store (-1, std::memory_order_relaxed);
    appliedFactorIndex.store (initialFactorIndex, std::memory_order_relaxed);
    reportedLatencySamples.store (slotLatencies[0], std::memory_order_relaxed);
    activeProcessingSlot = 0;

    muteGain.reset (sampleRate, muteFadeSeconds);
    muteGain.setCurrentAndTargetValue (1.0f);
}

void OversamplingEngine::reset() noexcept
{
    for (auto& oversampler : oversamplers)
        if (oversampler != nullptr)
            oversampler->reset();

    muteGain.setCurrentAndTargetValue (1.0f);
}

void OversamplingEngine::setOversamplingFactor (int factor)
{
    requestFactorChange (factor);
    applyPendingFactor();
}

void OversamplingEngine::setOversamplingFactor (Factor factor)
{
    setOversamplingFactor (static_cast<int> (factor));
}

void OversamplingEngine::requestFactorChange (int factor) noexcept
{
    const auto factorIndex = factorToIndex (factor);

    jassert (factorIndex >= 0);

    if (factorIndex >= 0)
        requestedFactorIndex.store (factorIndex, std::memory_order_release);
}

void OversamplingEngine::requestFactorChange (Factor factor) noexcept
{
    requestFactorChange (static_cast<int> (factor));
}

bool OversamplingEngine::applyPendingFactor()
{
    if (preparedMaximumBlockSize <= 0 || preparedNumberOfChannels <= 0)
        return false;

    if (preparedPendingSlot.load (std::memory_order_acquire) >= 0)
        return false;

    const auto requestedIndex =
        requestedFactorIndex.load (std::memory_order_acquire);

    if (requestedIndex
        == appliedFactorIndex.load (std::memory_order_acquire))
        return false;

    const auto inactiveSlot =
        1 - activeSlot.load (std::memory_order_acquire);
    auto preparedOversampler = createPreparedOversampler (requestedIndex);

    if (preparedOversampler == nullptr)
        return false;

    slotFactorIndices[static_cast<std::size_t> (inactiveSlot)] = requestedIndex;
    slotLatencies[static_cast<std::size_t> (inactiveSlot)] =
        juce::roundToInt (preparedOversampler->getLatencyInSamples());
    oversamplers[static_cast<std::size_t> (inactiveSlot)] =
        std::move (preparedOversampler);

    // Publishing the slot arms the audio-thread fade. All construction and
    // initProcessing() above have completed before the slot becomes visible.
    preparedPendingSlot.store (inactiveSlot, std::memory_order_release);
    return true;
}

bool OversamplingEngine::isFactorChangePending() const noexcept
{
    return preparedPendingSlot.load (std::memory_order_acquire) >= 0
           || requestedFactorIndex.load (std::memory_order_acquire)
                  != appliedFactorIndex.load (std::memory_order_acquire);
}

int OversamplingEngine::getOversamplingFactor() const noexcept
{
    return indexToFactor (
        appliedFactorIndex.load (std::memory_order_acquire));
}

int OversamplingEngine::getLatencySamples() const noexcept
{
    return reportedLatencySamples.load (std::memory_order_acquire);
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

    activeProcessingSlot = activeSlot.load (std::memory_order_acquire);
    auto& oversampler =
        oversamplers[static_cast<std::size_t> (activeProcessingSlot)];

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

    auto& oversampler =
        oversamplers[static_cast<std::size_t> (activeProcessingSlot)];

    if (oversampler == nullptr
        || outputBlock.getNumSamples()
               > static_cast<std::size_t> (preparedMaximumBlockSize)
        || outputBlock.getNumChannels()
               > static_cast<std::size_t> (preparedNumberOfChannels))
        return;

    oversampler->processSamplesDown (outputBlock);
    applyMuteFade (outputBlock);
}

std::unique_ptr<OversamplingEngine::Oversampler>
OversamplingEngine::createPreparedOversampler (int factorIndex) const
{
    if (preparedMaximumBlockSize <= 0 || preparedNumberOfChannels <= 0)
        return {};

    auto preparedOversampler = std::make_unique<Oversampler> (
        static_cast<std::size_t> (preparedNumberOfChannels),
        static_cast<std::size_t> (factorIndex),
        Oversampler::filterHalfBandFIREquiripple,
        true,
        true);

    preparedOversampler->initProcessing (
        static_cast<std::size_t> (preparedMaximumBlockSize));
    return preparedOversampler;
}

void OversamplingEngine::applyMuteFade (
    juce::dsp::AudioBlock<float>& outputBlock) noexcept
{
    const auto pendingSlot =
        preparedPendingSlot.load (std::memory_order_acquire);

    if (pendingSlot >= 0)
        muteGain.setTargetValue (0.0f);

    for (std::size_t sample = 0; sample < outputBlock.getNumSamples(); ++sample)
    {
        const auto gain = muteGain.getNextValue();

        for (std::size_t channel = 0;
             channel < outputBlock.getNumChannels();
             ++channel)
            outputBlock.getChannelPointer (channel)[sample] *= gain;
    }

    if (pendingSlot < 0 || muteGain.isSmoothing()
        || muteGain.getCurrentValue() > 0.0f)
        return;

    activeSlot.store (pendingSlot, std::memory_order_release);
    appliedFactorIndex.store (
        slotFactorIndices[static_cast<std::size_t> (pendingSlot)],
        std::memory_order_release);
    reportedLatencySamples.store (
        slotLatencies[static_cast<std::size_t> (pendingSlot)],
        std::memory_order_release);
    preparedPendingSlot.store (-1, std::memory_order_release);
    muteGain.setTargetValue (1.0f);
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
