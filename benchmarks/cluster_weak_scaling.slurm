#!/bin/bash
#SBATCH --job-name=erosion-weak
#SBATCH --output=benchmarks/results/cluster/weak_%j.out
#SBATCH --error=benchmarks/results/cluster/weak_%j.err
#SBATCH --time=00:30:00
#SBATCH --cpus-per-task=1

set -euo pipefail

EXE=${EXE:-./build-cluster/erosion}
TERRAIN=${TERRAIN:-perlinNoise}
BASE_WIDTH=${BASE_WIDTH:-2048}
BASE_HEIGHT=${BASE_HEIGHT:-512}
STEPS=${STEPS:-200}
WARMUP=${WARMUP:-3}
RUNS=${RUNS:-10}
RANKS_LIST=${RANKS_LIST:-"1 2 4 8 16 32"}
CPUS_PER_TASK=${SLURM_CPUS_PER_TASK:-1}

mkdir -p benchmarks/results/cluster

if [ -n "${MODULES:-}" ]; then
    module load ${MODULES}
fi

echo "Job started on $(hostname) at $(date)"
echo "Allocated nodes: ${SLURM_JOB_NUM_NODES:-unknown}"
echo "CPUs per task: ${CPUS_PER_TASK}"

for ranks in ${RANKS_LIST}; do
    height=$((BASE_HEIGHT * ranks))
    out="benchmarks/results/cluster/weak_np${ranks}"
    echo "Running weak-scaling case with ${ranks} MPI ranks and height ${height}"
    srun --ntasks="${ranks}" --cpus-per-task="${CPUS_PER_TASK}" "${EXE}" bench mpi \
        --terrain "${TERRAIN}" \
        --width "${BASE_WIDTH}" \
        --height "${height}" \
        --steps "${STEPS}" \
        --warmup "${WARMUP}" \
        --runs "${RUNS}" \
        --out "${out}"
done

echo "Job finished at $(date)"
