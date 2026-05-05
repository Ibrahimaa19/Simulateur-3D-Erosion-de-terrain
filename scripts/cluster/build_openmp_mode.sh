#!/usr/bin/env bash

set -euo pipefail

MODE="${1:-11}"
BUILD_DIR="${BUILD_DIR:-build}"

cmake -S . -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE=Release \
  -DEROSION_ENABLE_RENDERING=OFF \
  -DEROSION_ENABLE_MPI=ON \
  -DEROSION_ENABLE_TESTS=ON \
  -DEROSION_MODE="$MODE"

cmake --build "$BUILD_DIR" -j

echo "Built $BUILD_DIR/erosion with EROSION_MODE=$MODE"
