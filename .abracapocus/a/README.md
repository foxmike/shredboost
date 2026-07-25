# a

## Goal

Establish the buildable ShredBoost plugin scaffold with exact product identity and Linux target formats. PluginProcessor.h/.cpp shell enforces bus layout (mono-in/mono-out, mono-in/stereo-out, stereo-in/stereo-out) with deterministic unused output channel clearing and juce::ScopedNoDenormals. PluginEditor.h/.cpp minimal editor shell. CMakeLists.txt configures juce_add_plugin with PRODUCT_NAME ShredBoost, COMPANY_NAME 'nSpace Systems', VERSION 1.0.0, PLUGIN_MANUFACTURER_CODE NSPC, PLUGIN_CODE ShBs, BUNDLE_ID com.nspacesystems.shredspace, LV2URI urn:nspacesystems:plugin:shredboost, FORMATS VST3 LV2 Standalone, CXX_STANDARD 20. CMakePresets.json provides linux-release and windows-msvc presets. scripts/build_all.sh paste-and-run build script. Phase verification document docs/verification/phase_a_scaffold.md.

## Notes

- Async: Wave 1: a.1 creates PluginProcessor shell. Wave 2: a.2 creates PluginEditor shell (depends on a.1). Wave 3: a.3 creates CMakeLists.txt and CMakePresets.json (depends on a.2). Wave 4: a.4 creates scripts/build_all.sh and .gitignore (depends on a.3). Final wave: a.5 writes docs/verification/phase_a_scaffold.md (depends on a.4).
- Risk: Bus layout acceptance contract and frozen identity codes (NSPC/ShBs) must be exact in this phase.
