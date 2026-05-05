#!/usr/bin/env bash
#SBATCH --job-name=erosion
#SBATCH --output=logs/%x-%j.out
#SBATCH --error=logs/%x-%j.err
#SBATCH --hint=nomultithread

set -euo pipefail

mkdir -p logs results

: "${TERRAIN:=perlinNoise}"
: "${W:=512}"
: "${H:=$W}"
: "${STEPS:=10}"
: "${P_ROWS:=1}"
: "${P_COLS:=1}"
: "${EXE:=./build/erosion}"
: "${EROSION_MODE:=11}"

export OMP_NUM_THREADS="${SLURM_CPUS_PER_TASK:-1}"
export OMP_PROC_BIND=close
export OMP_PLACES=cores

CSV="${CSV:-results/erosion_${SLURM_JOB_ID:-local}_${TERRAIN}_${W}x${H}_steps${STEPS}_ranks${SLURM_NTASKS:-1}_p${P_ROWS}x${P_COLS}_omp${OMP_NUM_THREADS}.csv}"

echo "Using prebuilt $EXE. Expected compile-time EROSION_MODE=$EROSION_MODE"
srun "$EXE" MPI "$TERRAIN" "$W" "$H" "$STEPS" "$P_ROWS" "$P_COLS" --no-image --csv "$CSV"
