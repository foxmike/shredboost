#include "ParameterSnapshot.h"

#include "ParameterIDs.h"

namespace
{
static_assert (std::atomic<float>::is_always_lock_free,
               "ParameterSnapshot requires lock-free float atomics on the audio thread");

float loadOr (const std::atomic<float>* value, float fallback) noexcept
{
    return value != nullptr ? value->load (std::memory_order_relaxed) : fallback;
}
} // namespace

namespace ShredBoost
{

ParameterSnapshot::ParameterSnapshot() noexcept
{
    snapTo ({});
}

void ParameterSnapshot::bindTo (juce::AudioProcessorValueTreeState& state)
{
    RawParameterPointers pointers;
    pointers.input_trim = state.getRawParameterValue (ParamID::input_trim);
    pointers.tight_cut = state.getRawParameterValue (ParamID::tight_cut);
    pointers.mid_freq = state.getRawParameterValue (ParamID::mid_freq);
    pointers.mid_amount = state.getRawParameterValue (ParamID::mid_amount);
    pointers.drive = state.getRawParameterValue (ParamID::drive);
    pointers.tone = state.getRawParameterValue (ParamID::tone);
    pointers.level = state.getRawParameterValue (ParamID::level);
    pointers.pick_attack = state.getRawParameterValue (ParamID::pick_attack);
    pointers.voicing_mode = state.getRawParameterValue (ParamID::voicing_mode);
    pointers.oversampling = state.getRawParameterValue (ParamID::oversampling);
    pointers.auto_gain = state.getRawParameterValue (ParamID::auto_gain);

    setRawParameterPointers (pointers);
}

void ParameterSnapshot::setRawParameterPointers (RawParameterPointers pointers) noexcept
{
    raw = pointers;
    snapTo (readRawParameters());
}

void ParameterSnapshot::prepare (double sampleRate) noexcept
{
    jassert (sampleRate > 0.0);

    inputTrim.reset (sampleRate, smoothingTimeSeconds);
    tightCut.reset (sampleRate, smoothingTimeSeconds);
    midFreq.reset (sampleRate, smoothingTimeSeconds);
    midAmount.reset (sampleRate, smoothingTimeSeconds);
    drive.reset (sampleRate, smoothingTimeSeconds);
    tone.reset (sampleRate, smoothingTimeSeconds);
    level.reset (sampleRate, smoothingTimeSeconds);
    pickAttack.reset (sampleRate, smoothingTimeSeconds);

    snapTo (readRawParameters());
}

void ParameterSnapshot::updateTargetsFromRawParameters() noexcept
{
    setTargets (readRawParameters());
}

void ParameterSnapshot::setTargets (const Values& targets) noexcept
{
    inputTrim.setTargetValue (targets.input_trim);
    tightCut.setTargetValue (targets.tight_cut);
    midFreq.setTargetValue (targets.mid_freq);
    midAmount.setTargetValue (targets.mid_amount);
    drive.setTargetValue (targets.drive);
    tone.setTargetValue (targets.tone);
    level.setTargetValue (targets.level);
    pickAttack.setTargetValue (targets.pick_attack);

    voicingMode = targets.voicing_mode;
    oversamplingMode = targets.oversampling;
    autoGain = targets.auto_gain;
}

ParameterSnapshot::Values ParameterSnapshot::advance() noexcept
{
    inputTrim.getNextValue();
    tightCut.getNextValue();
    midFreq.getNextValue();
    midAmount.getNextValue();
    drive.getNextValue();
    tone.getNextValue();
    level.getNextValue();
    pickAttack.getNextValue();
    return current();
}

ParameterSnapshot::Values ParameterSnapshot::advance (int numberOfSamples) noexcept
{
    jassert (numberOfSamples >= 0);

    inputTrim.skip (numberOfSamples);
    tightCut.skip (numberOfSamples);
    midFreq.skip (numberOfSamples);
    midAmount.skip (numberOfSamples);
    drive.skip (numberOfSamples);
    tone.skip (numberOfSamples);
    level.skip (numberOfSamples);
    pickAttack.skip (numberOfSamples);
    return current();
}

ParameterSnapshot::Values ParameterSnapshot::current() const noexcept
{
    return {
        inputTrim.getCurrentValue(),
        tightCut.getCurrentValue(),
        midFreq.getCurrentValue(),
        midAmount.getCurrentValue(),
        drive.getCurrentValue(),
        tone.getCurrentValue(),
        level.getCurrentValue(),
        pickAttack.getCurrentValue(),
        voicingMode,
        oversamplingMode,
        autoGain
    };
}

ParameterSnapshot::Values ParameterSnapshot::readRawParameters() const noexcept
{
    const auto fallback = current();

    return {
        loadOr (raw.input_trim, fallback.input_trim),
        loadOr (raw.tight_cut, fallback.tight_cut),
        loadOr (raw.mid_freq, fallback.mid_freq),
        loadOr (raw.mid_amount, fallback.mid_amount),
        loadOr (raw.drive, fallback.drive),
        loadOr (raw.tone, fallback.tone),
        loadOr (raw.level, fallback.level),
        loadOr (raw.pick_attack, fallback.pick_attack),
        static_cast<int> (loadOr (raw.voicing_mode, static_cast<float> (fallback.voicing_mode))),
        static_cast<int> (loadOr (raw.oversampling, static_cast<float> (fallback.oversampling))),
        loadOr (raw.auto_gain, fallback.auto_gain ? 1.0f : 0.0f) >= 0.5f
    };
}

void ParameterSnapshot::snapTo (const Values& values) noexcept
{
    inputTrim.setCurrentAndTargetValue (values.input_trim);
    tightCut.setCurrentAndTargetValue (values.tight_cut);
    midFreq.setCurrentAndTargetValue (values.mid_freq);
    midAmount.setCurrentAndTargetValue (values.mid_amount);
    drive.setCurrentAndTargetValue (values.drive);
    tone.setCurrentAndTargetValue (values.tone);
    level.setCurrentAndTargetValue (values.level);
    pickAttack.setCurrentAndTargetValue (values.pick_attack);

    voicingMode = values.voicing_mode;
    oversamplingMode = values.oversampling;
    autoGain = values.auto_gain;
}

} // namespace ShredBoost
