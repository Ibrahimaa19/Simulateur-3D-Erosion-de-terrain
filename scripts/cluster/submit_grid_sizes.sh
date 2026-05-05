#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

mkdir -p "$ROOT_DIR/logs" "$ROOT_DIR/results"

TERRAIN="${TERRAIN:-perlinNoise}"
STEPS="${STEPS:-10}"
RANKS="${RANKS:-16}"
OMP="${OMP:-8}"
P_ROWS="${P_ROWS:-4}"
P_COLS="${P_COLS:-4}"

for size in 512 1024 2048 4096 8192; do
  sbatch \
    --nodes=1 \
    --ntasks="$RANKS" \
    --cpus-per-task="$OMP" \
    --export=ALL,TERRAIN="$TERRAIN",W="$size",H="$size",STEPS="$STEPS",P_ROWS="$P_ROWS",P_COLS="$P_COLS" \
    "$SCRIPT_DIR/run_one_config.slurm"
done
