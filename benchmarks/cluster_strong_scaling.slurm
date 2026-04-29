#!/bin/bash
#SBATCH --job-name=erosion-strong
#SBATCH --output=benchmarks/results/cluster/strong_%j.out
#SBATCH --error=benchmarks/results/cluster/strong_%j.err
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=32
#SBATCH --time=00:30:00

set -euo pipefail

EXE=${EXE:-./build-cluster/erosion}
TERRAIN=${TERRAIN:-perlinNoise}
WIDTH=${WIDTH:-4096}
HEIGHT=${HEIGHT:-4096}
STEPS=${STEPS:-200}
WARMUP=${WARMUP:-3}
RUNS=${RUNS:-10}
RANKS_LIST=${RANKS_LIST:-"1 2 4 8 16 32"}

mkdir -p benchmarks/results/cluster

for ranks in ${RANKS_LIST}; do
    out="benchmarks/results/cluster/strong_np${ranks}"
    mpirun -np "${ranks}" "${EXE}" bench mpi \
        --terrain "${TERRAIN}" \
        --width "${WIDTH}" \
        --height "${HEIGHT}" \
        --steps "${STEPS}" \
        --warmup "${WARMUP}" \
        --runs "${RUNS}" \
        --out "${out}"
done
