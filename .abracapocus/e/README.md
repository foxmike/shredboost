# e

## Goal

Implement oversampling and auto gain-matched bypass. Source/DSP/OversamplingEngine.h/.cpp: polyphase FIR half-band engine wrapping juce::dsp::Oversampling<float> for 1x, 2x, 4x rates; reports PDC latency via setLatencySamples; oversampling factor changes handled on message thread with re-prepare and 10 ms mute-fade. Source/DSP/AutoGainBypass.h/.cpp: RMS tracking circuit comparing dry input vs wet output to calculate dynamic level difference, applying dry compensation gain during bypass preview. tests/OversamplingTests.cpp asserts inharmonic aliasing floor rejection and exact PDC latency reporting.

## Notes

- Async: Wave 1: e.1 (OversamplingEngine) and e.3 (AutoGainBypass) run in parallel. Wave 2: e.2 adds message-thread factor switching with mute-fade to OversamplingEngine (depends on e.1). Final wave: e.4 creates tests/OversamplingTests.cpp and registers sources in CMakeLists.txt (depends on e.2, e.3).
- Risk: Oversampling re-allocation must never occur inside processBlock.
