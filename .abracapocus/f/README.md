# f

## Goal

Full processor integration and real-time execution audits. PluginProcessor processBlock incorporates ShredBoostEngine, OversamplingEngine, and AutoGainBypass into audio processing loop. Allocates max block size buffers in prepareToPlay. Lock-free snapshot double buffering for parameter changes. tests/ProcessBlockRealtimeTests.cpp verifies zero heap allocation during processBlock and denormal flushing.

## Notes

- Async: Wave 1: f.1 integrates full DSP engine into PluginProcessor. Wave 2: f.2 creates tests/ProcessBlockRealtimeTests.cpp (depends on f.1). Final wave: f.3 writes docs/verification/phase_f_integration.md (depends on f.2).
- Risk: ProcessBlock MUST pass heap allocation tracking assertions in tests.
