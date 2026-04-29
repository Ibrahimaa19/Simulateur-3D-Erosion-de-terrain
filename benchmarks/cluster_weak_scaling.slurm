#!/bin/bash
#SBATCH --job-name=erosion-weak
#SBATCH --output=benchmarks/results/cluster/weak_%j.out
#SBATCH --error=benchmarks/results/cluster/weak_%j.err
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=32
#SBATCH --time=00:30:00

set -euo pipefail

EXE=${EXE:-./build-cluster/erosion}
TERRAIN=${TERRAIN:-perlinNoise}
BASE_WIDTH=${BASE_WIDTH:-2048}
BASE_HEIGHT=${BASE_HEIGHT:-512}
STEPS=${STEPS:-200}
WARMUP=${WARMUP:-3}
RUNS=${RUNS:-10}
RANKS_LIST=${RANKS_LIST:-"1 2 4 8 16 32"}

mkdir -p benchmarks/results/cluster

for ranks in ${RANKS_LIST}; do
    height=$((BASE_HEIGHT * ranks))
    out="benchmarks/results/cluster/weak_np${ranks}"
    mpirun -np "${ranks}" "${EXE}" bench mpi \
        --terrain "${TERRAIN}" \
        --width "${BASE_WIDTH}" \
        --height "${height}" \
        --steps "${STEPS}" \
        --warmup "${WARMUP}" \
        --runs "${RUNS}" \
        --out "${out}"
done
