#!/usr/bin/env bash
set -euo pipefail

# ShredBoost paste-and-run build script.
# Run from the repository root: bash scripts/build_all.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build/linux-release"

echo "==> Configuring ShredBoost (preset: linux-release)..."
cmake --preset linux-release

echo "==> Building ShredBoost (Release) at ${BUILD_DIR}..."
cmake --build "${BUILD_DIR}" --config Release

echo "==> Build complete."
