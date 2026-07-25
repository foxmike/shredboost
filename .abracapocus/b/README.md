# b

## Goal

Implement frozen parameter taxonomy in Source/Parameters/ParameterIDs.h (schema version 1) containing all automatable parameters: input_trim (-18..+18 dB default 0), tight_cut (80..400 Hz default 180), mid_freq (500..1200 Hz default 720), mid_amount (0..+9 dB default +4), drive (0..+36 dB default +12), tone (1..16 kHz default 6.5), level (-24..+12 dB default 0), pick_attack (0..+9 dB default +2), voicing_mode (0..3 choice, default Yng-Grey), oversampling (0..2 choice, default 2x), auto_gain (bool, default Off). ParameterLayout.h/.cpp builds full APVTS. ParameterSnapshot.h/.cpp provides real-time domain-smoothed value snapshot. PluginProcessor wires APVTS ownership and getStateInformation/setStateInformation XML serialization. Unit test harness tests/TestMain.cpp and tests/ParameterTests.cpp.

## Notes

- Async: Wave 1: b.1 creates ParameterIDs.h. Wave 2: b.2 creates ParameterLayout.h/.cpp (depends on b.1). Wave 3: b.3 creates ParameterSnapshot.h/.cpp (depends on b.2). Wave 4: b.4 integrates APVTS into PluginProcessor (depends on b.3). Final wave: b.5 creates TestMain.cpp and ParameterTests.cpp and registers CTest target in CMakeLists.txt (depends on b.4).
- Risk: Parameter IDs freeze permanently in b.1. Internal states must never be exposed to APVTS.
