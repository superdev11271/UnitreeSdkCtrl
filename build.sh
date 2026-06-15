#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

mkdir -p build
cmake -S . -B build
cmake --build build -j"$(nproc)"

echo "Build complete: ${SCRIPT_DIR}/build/unitree_sdk_ctrl"
