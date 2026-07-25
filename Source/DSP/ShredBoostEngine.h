#pragma once

#include <juce_dsp/juce_dsp.h>

#include "DcBlocker.h"
#include "NonlinearClipper.h"
#include "ToneFilter.h"
#include "TransientShaper.h"

namespace ShredBoost
{

/**
    Phase-D portion of the ShredBoost processing chain.

    The owned stages retain their binding positions so earlier pre-filter and
    oversampling stages, plus later output stages, can be composed around this
    engine without changing the signal order.
*/
class ShredBoostEngine final
{
public:
    ShredBoostEngine() noexcept;

    void prepare (double sampleRate, int maxBlockSize) noexcept;
    void reset() noexcept;

    void setPickAttack (float pickAttackDb) noexcept;
    void setVoicingMode (int mode) noexcept;
    void setTone (float toneKhz) noexcept;

    void process (juce::dsp::AudioBlock<float>& block) noexcept;

private:
    static constexpr float hzPerKhz = 1000.0f;

    TransientShaper transientShaper;
    NonlinearClipper nonlinearClipper;
    DcBlocker dcBlocker;
    ToneFilter toneFilter;
};

} // namespace ShredBoost
