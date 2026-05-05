#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

mkdir -p "$ROOT_DIR/logs" "$ROOT_DIR/results"

TERRAIN="${TERRAIN:-perlinNoise}"
W="${W:-8192}"
H="${H:-$W}"
STEPS="${STEPS:-10}"
RANKS_PER_NODE="${RANKS_PER_NODE:-16}"
OMP="${OMP:-8}"
EROSION_MODE="${EROSION_MODE:-11}"

for nodes in 1 2 4 8; do
  total_ranks=$((nodes * RANKS_PER_NODE))
  case "$total_ranks" in
    16) p_rows=4; p_cols=4 ;;
    32) p_rows=4; p_cols=8 ;;
    64) p_rows=8; p_cols=8 ;;
    128) p_rows=8; p_cols=16 ;;
    *)
      echo "No default decomposition for $total_ranks ranks; set RANKS_PER_NODE to 16 or edit this script." >&2
      exit 1
      ;;
  esac

  sbatch \
    --nodes="$nodes" \
    --ntasks-per-node="$RANKS_PER_NODE" \
    --cpus-per-task="$OMP" \
    --export=ALL,TERRAIN="$TERRAIN",W="$W",H="$H",STEPS="$STEPS",P_ROWS="$p_rows",P_COLS="$p_cols",EROSION_MODE="$EROSION_MODE" \
    "$SCRIPT_DIR/run_one_config.slurm"
done
