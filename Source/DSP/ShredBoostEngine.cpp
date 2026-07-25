#include "ShredBoostEngine.h"

namespace ShredBoost
{

ShredBoostEngine::ShredBoostEngine() noexcept = default;

void ShredBoostEngine::prepare (double sampleRate, int maxBlockSize) noexcept
{
    transientShaper.prepare (sampleRate, maxBlockSize);
    nonlinearClipper.prepare (sampleRate, maxBlockSize);
    dcBlocker.prepare (sampleRate, maxBlockSize);
    toneFilter.prepare (sampleRate, maxBlockSize);
}

void ShredBoostEngine::reset() noexcept
{
    transientShaper.reset();
    nonlinearClipper.reset();
    dcBlocker.reset();
    toneFilter.reset();
}

void ShredBoostEngine::setPickAttack (float pickAttackDb) noexcept
{
    transientShaper.setPickAttackDb (pickAttackDb);
}

void ShredBoostEngine::setVoicingMode (int mode) noexcept
{
    nonlinearClipper.setVoicingMode (mode);
}

void ShredBoostEngine::setTone (float toneKhz) noexcept
{
    toneFilter.setCutoff (toneKhz * hzPerKhz);
}

void ShredBoostEngine::process (juce::dsp::AudioBlock<float>& block) noexcept
{
    juce::ScopedNoDenormals noDenormals;

    // Binding slots [0]-[2]: input, pre-filter, and oversampling stages.
    transientShaper.process (block);  // [3]
    // Binding slot [4]: drive stage.
    nonlinearClipper.process (block); // [5]
    // Binding slot [6]: reserved intermediate stage.
    dcBlocker.process (block);        // [7]
    toneFilter.process (block);       // [8]
    // Binding slot [9]: output stage.
}

} // namespace ShredBoost
