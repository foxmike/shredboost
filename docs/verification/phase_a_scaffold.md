# Phase A Scaffold Verification

This document describes how to verify the Phase A scaffold of the **ShredBoost** plugin.
Phase A delivers a minimal, buildable plugin shell — no DSP, no GUI behavior — that establishes
the project identity, build configuration, and bus-layout contract that all later phases build on.

## 1. Phase A Deliverables

Phase A produces the following scaffold artifacts:

- **Processor shell** — `Source/PluginProcessor.cpp` / `.h`: an empty JUCE `AudioProcessor`
  with a frozen `isBusesLayoutSupported` contract and a no-op `processBlock`.
- **Editor shell** — `Source/PluginEditor.cpp` / `.h`: a placeholder `AudioProcessorEditor`
  so the plugin loads and shows a window.
- **CMake config** — `CMakeLists.txt` and `CMakePresets.json`: JUCE-based plugin project with the
  frozen product identity and a `linux-release` configure preset.
- **Build script** — `scripts/build_all.sh`: a paste-and-run script that configures and builds the
  project in Release mode.
- **Gitignore** — `.gitignore`: excludes `build/` and other generated artifacts from version control.

## 2. Frozen Product Identity

The following identity values are frozen and MUST NOT change after Phase A. They are sourced from
`CMakeLists.txt` (`juce_add_plugin` arguments).

| Field                        | Value                                |
|------------------------------|--------------------------------------|
| Plugin name                  | `ShredBoost`                         |
| Vendor                       | `nSpace Systems`                     |
| Version                      | `1.0.0`                              |
| `PLUGIN_MANUFACTURER_CODE`   | `NSPC`                               |
| `PLUGIN_CODE`                | `ShBs`                               |
| `BUNDLE_ID`                  | `com.nspacesystems.shredboost`       |
| `LV2URI`                     | `urn:nspacesystems:plugin:shredboost`|
| `FORMATS`                    | `VST3` `LV2` `Standalone`            |

## 3. Bus-Layout Acceptance Contract

`ShredBoostAudioProcessor::isBusesLayoutSupported` accepts exactly the following main bus layouts:

- **mono-in / mono-out**
- **mono-in / stereo-out**
- **stereo-in / stereo-out**

All other bus layouts are **rejected** (the function returns `false`). Any change to this contract
in later phases is an architecture drift and must be flagged against Phase A scope.

## 4. Verification Commands

Run the following paste-and-run commands from the repository root. The JUCE submodule must be
populated (e.g. `git submodule update --init --recursive`) before configuring.

### Configure

```bash
cmake --preset linux-release
```

### Build

```bash
cmake --build build/linux-release --config Release
```

…or, equivalently, run the helper script which performs both steps:

```bash
bash scripts/build_all.sh
```

Both forms configure into `build/linux-release` and build the Release configuration.

## 5. Build Artifacts

After a successful build, the plugin artifacts are produced under `build/linux-release/`:

- **VST3** — `build/linux-release/ShredBoost_artifacts/VST3/ShredBoost.vst3`
- **LV2** — `build/linux-release/ShredBoost_artifacts/LV2/ShredBoost.lv2`
- **Standalone** — `build/linux-release/ShredBoost_artifacts/Standalone/ShredBoost`

(Assert that each of the three `FORMATS` directories exists and contains its expected bundle/binary.)

## 6. Acceptance Checklist

- [ ] `cmake --preset linux-release` completes with exit code `0`.
- [ ] `cmake --build build/linux-release --config Release` (or `bash scripts/build_all.sh`)
      completes with exit code `0`.
- [ ] VST3, LV2, and Standalone artifacts exist under `build/linux-release/ShredBoost_artifacts/`.
- [ ] Identity values match the table in §2 exactly.
- [ ] `isBusesLayoutSupported` accepts only the three layouts in §3 and rejects all others.
