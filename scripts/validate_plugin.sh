#!/usr/bin/env bash
# chmod +x scripts/validate_plugin.sh
#
# ShredBoost 1.0 release validation script.
#
# Self-contained release validation for the ShredBoost audio plugin
# (plugin name: ShredBoost, bundle id: com.nspacesystems.shredboost).
#
# Runs, in order:
#   1. CMake configure + Release build from a clean build directory.
#   2. The full test suite via CTest (juce::UnitTest console runner targets,
#      no third-party testing frameworks).
#   3. Binary artifact existence checks for VST3, LV2, and Standalone builds.
#   4. A final human-readable PASS/FAIL summary.
#
# Exits non-zero on any failure. Nothing under JUCE/ is touched.
#
# Usage (from the repository root):
#   bash scripts/validate_plugin.sh

set -euo pipefail

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

PLUGIN_NAME="ShredBoost"
CONFIG="Release"

# Binary artifacts produced by JUCE under the build tree. JUCE lays these out
# as <build>/ShredBoost_artefacts/<CONFIG>/<FORMAT>/<ShredBoost...>. Because
# single-config generators (Unix Makefiles / Ninja) put artefacts under the
# configured CMAKE_BUILD_TYPE subdirectory while multi-config generators put
# them under the per-config subdirectory selected by --config, we resolve each
# artifact to its first existing candidate path at check time.
ARTEFACT_ROOT="${BUILD_DIR}/ShredBoost_artefacts"
VST3_CANDIDATES=(
    "${ARTEFACT_ROOT}/${CONFIG}/VST3/${PLUGIN_NAME}.vst3"
    "${ARTEFACT_ROOT}/VST3/${PLUGIN_NAME}.vst3"
)
LV2_CANDIDATES=(
    "${ARTEFACT_ROOT}/${CONFIG}/LV2/${PLUGIN_NAME}.lv2"
    "${ARTEFACT_ROOT}/LV2/${PLUGIN_NAME}.lv2"
)
STANDALONE_CANDIDATES=(
    "${ARTEFACT_ROOT}/${CONFIG}/Standalone/${PLUGIN_NAME}"
    "${ARTEFACT_ROOT}/Standalone/${PLUGIN_NAME}"
)

# Track overall result; 0 = pass, 1 = fail.
OVERALL_STATUS=0

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

log()  { printf '\033[1m[%s]\033[0m %s\n' "validate" "$*"; }
ok()   { printf '\033[1;32m  PASS\033[0m  %s\n' "$*"; }
fail() { printf '\033[1;31m  FAIL\033[0m  %s\n' "$*"; OVERALL_STATUS=1; }

# Resolve the first existing candidate path. Echoes the resolved path and
# returns 0 if found, returns 1 if none exist (and echoes nothing).
resolve_artifact() {
    local candidate
    for candidate in "$@"; do
        if [[ -e "${candidate}" ]]; then
            printf '%s' "${candidate}"
            return 0
        fi
    done
    return 1
}

# ---------------------------------------------------------------------------
# 1. Configure + build (clean build directory)
# ---------------------------------------------------------------------------

step_build() {
    log "Step 1: configure + build (Release) from a clean ${BUILD_DIR}"

    if [[ -d "${BUILD_DIR}" ]]; then
        log "Removing existing build directory: ${BUILD_DIR}"
        rm -rf "${BUILD_DIR}"
    fi

    log "Configuring: cmake -S . -B build -DCMAKE_BUILD_TYPE=Release"
    (
        cd "${ROOT_DIR}"
        cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    )

    log "Building: cmake --build build --config Release"
    (
        cd "${ROOT_DIR}"
        cmake --build build --config Release
    )

    ok "configure + build completed"
}

# ---------------------------------------------------------------------------
# 2. Run the full test suite via CTest
# ---------------------------------------------------------------------------

step_test() {
    log "Step 2: run full test suite via CTest (juce::UnitTest console runner)"

    (
        cd "${ROOT_DIR}"
        ctest --test-dir build --output-on-failure
    )

    ok "test suite passed"
}

# ---------------------------------------------------------------------------
# 3. Binary artifact existence checks
# ---------------------------------------------------------------------------

step_artifacts() {
    log "Step 3: check binary artifacts (VST3, LV2, Standalone)"

    local resolved

    if resolved="$(resolve_artifact "${VST3_CANDIDATES[@]}")"; then
        ok "VST3 artifact found: ${resolved}"
    else
        fail "VST3 artifact missing (tried: ${VST3_CANDIDATES[*]})"
    fi

    if resolved="$(resolve_artifact "${LV2_CANDIDATES[@]}")"; then
        ok "LV2 artifact found: ${resolved}"
    else
        fail "LV2 artifact missing (tried: ${LV2_CANDIDATES[*]})"
    fi

    if resolved="$(resolve_artifact "${STANDALONE_CANDIDATES[@]}")"; then
        ok "Standalone artifact found: ${resolved}"
    else
        fail "Standalone artifact missing (tried: ${STANDALONE_CANDIDATES[*]})"
    fi
}

# ---------------------------------------------------------------------------
# 4. Final summary
# ---------------------------------------------------------------------------

step_summary() {
    log "Step 4: summary"
    if [[ "${OVERALL_STATUS}" -eq 0 ]]; then
        printf '\n\033[1;32m==================== VALIDATION PASSED ====================\033[0m\n'
        printf 'ShredBoost 1.0 (com.nspacesystems.shredboost): build, tests, and artifacts OK.\n'
    else
        printf '\n\033[1;31m==================== VALIDATION FAILED ====================\033[0m\n'
        printf 'ShredBoost 1.0 (com.nspacesystems.shredboost): one or more checks failed.\n'
    fi
}

# ---------------------------------------------------------------------------
# Orchestration
# ---------------------------------------------------------------------------

main() {
    log "ShredBoost 1.0 release validation"
    log "root: ${ROOT_DIR}"
    log "build: ${BUILD_DIR}"

    step_build   || OVERALL_STATUS=1
    step_test    || OVERALL_STATUS=1
    step_artifacts
    step_summary

    exit "${OVERALL_STATUS}"
}

main "$@"
