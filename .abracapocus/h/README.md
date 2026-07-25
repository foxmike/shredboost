# h

## Goal

Release validation, factory presets, and documentation. scripts/validate_plugin.sh runs full build, CTest execution, and binary artifact existence checks. Factory presets covering Thrash Chug, Yngwie Lead, SD-1 Rhythm, and Clean Tight Attack. Phase h verification document docs/verification/phase_h_release.md.

## Notes

- Async: Wave 1: h.1 creates validate_plugin.sh. Wave 2: h.2 creates FactoryPresets.xml and registers resource in CMakeLists.txt (depends on h.1). Final wave: h.3 executes validation script and writes docs/verification/phase_h_release.md (depends on h.2).
- Risk: Release verification document must record real executed test results.
