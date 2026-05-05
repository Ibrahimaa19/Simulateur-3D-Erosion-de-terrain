#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

mkdir -p "$ROOT_DIR/logs" "$ROOT_DIR/results"

TERRAIN="${TERRAIN:-perlinNoise}"
W="${W:-4096}"
H="${H:-$W}"
STEPS="${STEPS:-10}"
EROSION_MODE="${EROSION_MODE:-11}"

configs=(
  "1 128 1 1"
  "2 64 1 2"
  "4 32 2 2"
  "8 16 2 4"
  "16 8 4 4"
  "32 4 4 8"
  "64 2 8 8"
  "128 1 8 16"
)

for config in "${configs[@]}"; do
  read -r ranks omp p_rows p_cols <<< "$config"
  sbatch \
    --nodes=1 \
    --ntasks="$ranks" \
    --cpus-per-task="$omp" \
    --export=ALL,TERRAIN="$TERRAIN",W="$W",H="$H",STEPS="$STEPS",P_ROWS="$p_rows",P_COLS="$p_cols",EROSION_MODE="$EROSION_MODE" \
    "$SCRIPT_DIR/run_one_config.slurm"
done
