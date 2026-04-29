# Benchmark Workflow

Benchmark results are written to `benchmarks/results/`, which is ignored by Git.

## Main Commands

```bash
./build-headless/erosion bench erosion --terrain perlinNoise --width 1024 --height 1024 --steps 100 --warmup 3 --runs 10 --variant all --neighbors-list 4,8 --threads-list 1,2,4,8 --out benchmarks/results/erosion
./build-render/erosion bench render --terrain perlinNoise --width 1024 --height 1024 --frames 300 --warmup 3 --lod on --culling on --sync dirty --gpu-timer off --out benchmarks/results/render/lod_culling_dirty_sync
./build-render/erosion bench interaction --terrain perlinNoise --width 1024 --height 1024 --frames 120 --warmup 3 --lod on --culling on --sync dirty --out benchmarks/results/interaction
mpirun -np 2 ./build-cluster/erosion bench mpi --terrain perlinNoise --width 512 --height 512 --steps 10 --warmup 1 --runs 2 --out benchmarks/results/mpi
```

## Local Matrix

```bash
./benchmarks/run_local_matrix.sh
```

## Cluster Jobs

Use `sbatch` to allocate compute resources and `srun` inside the job script to launch MPI ranks:

```bash
RANKS_LIST="1 2 4 8 16 32" sbatch --nodes=1 --ntasks-per-node=32 --cpus-per-task=1 benchmarks/cluster_strong_scaling.slurm
RANKS_LIST="1 2 4 8 16 32" sbatch --nodes=1 --ntasks-per-node=32 --cpus-per-task=1 benchmarks/cluster_weak_scaling.slurm
```

Do not run large benchmarks on a login node. Use the login node for compilation and short smoke tests only.

## Plots

```bash
./benchmarks/plot_benchmarks.py --results benchmarks/results --figures benchmarks/figures
```

## Notes

- Warm-up runs are written to raw CSV files and excluded from summary statistics.
- `gpu_frame_ms` is `nan` when no OpenGL GPU timer context is available.
- Treat MPI + OpenMP as hybrid only when each MPI rank executes an OpenMP-enabled local kernel.
