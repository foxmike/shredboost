# Phase H Release Verification

This document records the real, actually-executed results of the **Phase H** release
validation for the **ShredBoost** plugin (plugin name: `ShredBoost`, bundle id:
`com.nspacesystems.shredboost`, version `1.0.0`).

Phase H is the release phase. Its validation is performed by the self-contained
`scripts/validate_plugin.sh` script (created in task h.1), which performs a clean
CMake configure + Release build, runs the full test suite via CTest, checks the existence
of the VST3, LV2, and Standalone binary artifacts, and emits a final PASS/FAIL summary.
This document is the human-readable record of that run; every value below was captured
from an actual execution on the date stated in §6.

## 1. Validation Command(s) Executed

The release validation was driven by a single command run from the repository root:

```bash
bash scripts/validate_plugin.sh
```

`scripts/validate_plugin.sh` internally executes, in order, the following commands
(from the repository root):

```bash
# Step 1 — clean configure + Release build
rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Step 2 — full test suite via CTest
ctest --test-dir build --output-on-failure

# Step 3 — binary artifact existence checks (filesystem tests for the
# VST3, LV2, and Standalone artefacts produced under build/ShredBoost_artefacts/)
```

The script runs under `set -euo pipefail` and exits non-zero on any failure. No
commands beyond the above were required for this validation; the script is the
complete and sole driver.

## 2. Build Result

**Result: PASS** — the clean configure + Release build completed with exit code `0`.

The script removed any pre-existing `build/` directory and configured from scratch.
Relevant output excerpts from the actual run:

Configure (single-config Unix Makefiles generator, GCC 11.4.0):

```
-- The C compiler identification is GNU 11.4.0
-- The CXX compiler identification is GNU 11.4.0
-- Found PkgConfig: /usr/bin/pkg-config (found version "0.29.2")
-- Checking for module 'alsa'
--   Found alsa, version 1.2.6.1
-- Configuring juceaide
-- ...
-- Configuring done (46.6s)
-- Generating done (0.0s)
-- Build files have been written to: /home/mike/dev/mike/music/plugins/shredboost/build
```

Build (selected excerpt showing all targets linked successfully):

```
[ 31%] Linking CXX static library ShredBoost_artefacts/Release/libShredBoost_SharedCode.a
[ 31%] Built target ShredBoost
[ 38%] Linking CXX executable ShredBoost_artefacts/Release/Standalone/ShredBoost
[ 38%] Built target ShredBoost_Standalone
[ 46%] Linking CXX shared module ShredBoost_artefacts/Release/LV2/ShredBoost.lv2/libShredBoost.so
[ 46%] Built target ShredBoost_LV2
[ 53%] Linking CXX shared module ShredBoost_artefacts/Release/VST3/ShredBoost.vst3/Contents/x86_64-linux/ShredBoost.so
[ 53%] Built target ShredBoost_VST3
[ 87%] Linking CXX executable ShredBoostUnitTests
[ 87%] Built target ShredBoostUnitTests
[100%] Linking CXX executable ShredBoostTests
[100%] Built target ShredBoostTests
  PASS  configure + build completed
```

The build produced the `ShredBoost` shared-code library plus the three format targets
(`ShredBoost_VST3`, `ShredBoost_LV2`, `ShredBoost_Standalone`) and both test executables
(`ShredBoostUnitTests`, `ShredBoostTests`). A small number of harmless compiler warnings
(unused parameters in `Source/UI/ShredLookAndFeel.cpp`; a `-Wsized-deallocation` note in
`tests/ProcessBlockRealtimeTests.cpp`) were emitted; none are errors and the build
succeeded. `cmake --build build --config Release` exited `0`.

## 3. CTest Results

**Result: PASS** — 5 of 5 tests passed, 0 failed.

`ctest --test-dir build --output-on-failure` produced the following summary on the
actual run:

```
Test project /home/mike/dev/mike/music/plugins/shredboost/build
    Start 1: ShredBoostParameterTests
1/5 Test #1: ShredBoostParameterTests .........   Passed    0.03 sec
    Start 2: ProcessBlockRealtimeTests
2/5 Test #2: ProcessBlockRealtimeTests ........   Passed    0.03 sec
    Start 3: FilterTests
3/5 Test #3: FilterTests ......................   Passed    0.02 sec
    Start 4: WaveshaperTests
4/5 Test #4: WaveshaperTests ..................   Passed    0.02 sec
    Start 5: OversamplingTests
5/5 Test #5: OversamplingTests ................   Passed    0.02 sec

100% tests passed out of 5

Total Test time (real) =   0.12 sec
  PASS  test suite passed
```

| Metric                | Value |
|-----------------------|-------|
| Tests run             | 5     |
| Tests passed          | 5     |
| Tests failed          | 0     |
| Percent passed        | 100%  |
| Total test time (real)| 0.12 sec |

The CTest summary line is:

> `100% tests passed out of 5`

The five registered CTest entries are the `juce::UnitTest`-based test targets
(no third-party testing frameworks): `ShredBoostParameterTests`,
`ProcessBlockRealtimeTests` (the Phase F real-time safety audit), `FilterTests`,
`WaveshaperTests`, and `OversamplingTests`. All passed.

## 4. Binary Artifact Existence Checks

**Result: PASS** — all three plugin formats were found.

The script resolves each artifact to its first existing candidate path under
`build/ShredBoost_artefacts/`. Because the build used a single-config generator
(Unix Makefiles, `CMAKE_BUILD_TYPE=Release`), all artefacts are located under the
`Release/` subdirectory. The actual script output was:

```
  PASS  VST3 artifact found: /home/mike/dev/mike/music/plugins/shredboost/build/ShredBoost_artefacts/Release/VST3/ShredBoost.vst3
  PASS  LV2 artifact found: /home/mike/dev/mike/music/plugins/shredboost/build/ShredBoost_artefacts/Release/LV2/ShredBoost.lv2
  PASS  Standalone artifact found: /home/mike/dev/mike/music/plugins/shredboost/build/ShredBoost_artefacts/Release/Standalone/ShredBoost
```

| Format      | Artifact path                                                                            | Found |
|-------------|------------------------------------------------------------------------------------------|-------|
| VST3        | `build/ShredBoost_artefacts/Release/VST3/ShredBoost.vst3`                                | yes   |
| LV2         | `build/ShredBoost_artefacts/Release/LV2/ShredBoost.lv2`                                  | yes   |
| Standalone  | `build/ShredBoost_artefacts/Release/Standalone/ShredBoost`                               | yes   |

Filesystem confirmation of the produced artifacts:

- `build/ShredBoost_artefacts/Release/VST3/ShredBoost.vst3/` — VST3 bundle containing
  `Contents/` (with the `x86_64-linux/ShredBoost.so` shared module and a generated
  `Contents/Resources/moduleinfo.json`).
- `build/ShredBoost_artefacts/Release/LV2/ShredBoost.lv2/` — LV2 bundle containing
  `manifest.ttl`, `dsp.ttl`, `ui.ttl`, and the `libShredBoost.so` shared module.
- `build/ShredBoost_artefacts/Release/Standalone/ShredBoost` — executable
  (6,231,616 bytes, `-rwxrwxr-x`).

All three `FORMATS` declared in `CMakeLists.txt` (`VST3` `LV2` `Standalone`) were
produced and detected.

## 5. Factory Presets File Presence and Bundling

**Result: PASS** — `Resources/Presets/FactoryPresets.xml` is present, contains the four
required factory presets, and is bundled into the plugin binary.

- **Source file present:** `Resources/Presets/FactoryPresets.xml` exists
  (3381 bytes). It defines exactly four `<Preset>` entries — `Thrash Chug`,
  `Yngwie Lead`, `SD-1 Rhythm`, and `Clean Tight Attack` — each carrying a complete
  `ShredBoostState` (schemaVersion `1`) with all frozen parameter IDs.

- **CMake registration:** the file is registered in `CMakeLists.txt` via
  `juce_add_binary_data`, which compiles it into a generated C++ accessor linked into
  every build format:

  ```cmake
  juce_add_binary_data(ShredBoostResources
      SOURCES
          Resources/Presets/FactoryPresets.xml
  )

  target_link_libraries(ShredBoost
      PRIVATE
          ShredBoostResources
  )
  ```

- **Bundled into the binary:** the actual build produced the binary-data target and its
  generated accessor, confirming the presets are embedded into the plugin:

  ```
  [  1%] Generating juce_binarydata_ShredBoostResources/JuceLibraryCode/BinaryData1.cpp, juce_binarydata_ShredBoostResources/JuceLibraryCode/BinaryData.h
  [  2%] Building CXX object CMakeFiles/ShredBoostResources.dir/juce_binarydata_ShredBoostResources/JuceLibraryCode/BinaryData1.cpp.o
  [  2%] Linking CXX static library libShredBoostResources.a
  [  2%] Built target ShredBoostResources
  ```

  The generated accessor `build/juce_binarydata_ShredBoostResources/JuceLibraryCode/BinaryData.h`
  was produced, and `libShredBoostResources.a` was linked into the `ShredBoost` shared-code
  target that all three format targets depend on. The factory presets therefore ship
  inside the VST3, LV2, and Standalone binaries.

## 6. Overall Conclusion

```
==================== VALIDATION PASSED ====================
ShredBoost 1.0 (com.nspacesystems.shredboost): build, tests, and artifacts OK.
```

**Overall result: PASS**

The Phase H release validation of ShredBoost 1.0 passed in full:

- Clean CMake configure + Release build: **PASS** (exit code `0`).
- Full CTest suite: **PASS** — 5/5 tests passed, 0 failed (`100% tests passed out of 5`).
- Binary artifact existence (VST3, LV2, Standalone): **PASS** — all three found.
- Four-preset `FactoryPresets.xml` present and bundled into the plugin binary: **PASS**.

**Date of execution:** 2026-07-25.
