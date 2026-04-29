# 3D Terrain Erosion Simulator

Numerical programming project for the M1 CHPS program at UVSQ / Paris-Saclay.

## Team

- Ibrahima DIALLO
- Aboubacar-Bonfing SY
- Papa Moussa NIANG
- Amar LECHANI

Supervisor: Mathys JAM

## Overview

This project simulates terrain generation and thermal erosion, with an optional OpenGL renderer and a benchmark workflow for local and cluster experiments.

The application supports:

- interactive terrain rendering;
- terrain generation from heightmaps, fault formation, midpoint displacement, and Perlin noise;
- thermal erosion kernels with sequential, blocked, OpenMP, and checkerboard variants;
- LOD, frustum culling, and dirty-patch mesh synchronization measurements;
- MPI execution for distributed erosion benchmarks;
- CSV exports and plotting scripts for performance analysis.

## Requirements

### Core Build

- Linux
- CMake 3.16 or newer
- C++17 compiler
- OpenMP

### Rendering Build

- OpenGL
- GLEW
- GLFW
- GLM
- X11 session for interactive rendering

### MPI Build

- MPI C++ wrapper, such as `mpicxx`
- `srun` or `mpirun`
- SLURM for cluster job scripts

### Plotting

- Python 3
- `pandas`
- `matplotlib`

## Build

Build a headless binary for tests and CPU benchmarks:

```bash
cmake -S . -B build-headless -DCMAKE_BUILD_TYPE=Release -DEROSION_ENABLE_RENDERING=OFF -DEROSION_ENABLE_MPI=OFF -DEROSION_NATIVE_ARCH=ON
cmake --build build-headless -j
```

Build with rendering support:

```bash
cmake -S . -B build-render -DCMAKE_BUILD_TYPE=Release -DEROSION_ENABLE_RENDERING=ON -DEROSION_ENABLE_MPI=OFF -DEROSION_NATIVE_ARCH=ON
cmake --build build-render -j
```

Build with MPI support:

```bash
cmake -S . -B build-cluster -DCMAKE_BUILD_TYPE=Release -DEROSION_ENABLE_RENDERING=OFF -DEROSION_ENABLE_MPI=ON -DEROSION_NATIVE_ARCH=ON
cmake --build build-cluster -j
```

Run the core tests:

```bash
ctest --test-dir build-headless --output-on-failure
```

## Interactive Mode

Start the OpenGL application:

```bash
./build-render/erosion render
```

The renderer requires a graphical session. Use the headless build for automated benchmarks.

## Validation Mode

Run validation on one terrain:

```bash
./build-headless/erosion test <terrain> <steps>
```

Available terrain names:

- `loadHeightmap`
- `faultFormation`
- `midpointDisplacement`
- `perlinNoise`

The legacy validation script is still available:

```bash
./validation/validation.sh <terrain> <start> <end> <step>
```

## Benchmark Mode

General interface:

```bash
./erosion bench <erosion|render|interaction|mpi> [options]
```

Common options:

```text
--terrain <terrain>
--width <width>
--height <height>
--steps <iterations>
--warmup <ignored_runs>
--runs <measured_runs>
--out <output_directory>
```

Warm-up runs are written to raw CSV files with `is_warmup=1`. Summary statistics exclude warm-up runs.

## Thermal Erosion Benchmarks

Small smoke test:

```bash
./build-headless/erosion bench erosion \
  --terrain perlinNoise \
  --width 128 \
  --height 128 \
  --steps 2 \
  --warmup 1 \
  --runs 2 \
  --variant blocked \
  --neighbors 8 \
  --threads 1 \
  --out benchmarks/results/smoke/erosion
```

Larger local benchmark:

```bash
./build-headless/erosion bench erosion \
  --terrain perlinNoise \
  --width 1024 \
  --height 1024 \
  --steps 100 \
  --warmup 3 \
  --runs 10 \
  --variant all \
  --neighbors-list 4,8 \
  --threads-list 1,2,4,8 \
  --out benchmarks/results/erosion
```

Erosion-specific options:

- `--variant pure|blocked|blockedParallel|checkerboard|blockedCheckerboard|inPlace|inPlaceParallel|all`
- `--neighbors 4|8`
- `--neighbors-list 4,8`
- `--threads <n>`
- `--threads-list 1,2,4,8`
- `--talus <angle_degrees>`
- `--transfer <coefficient>`

Generated files:

- `raw_runs.csv`
- `summary_stats.csv`

The summary file contains mean, median, sample standard deviation, min, max, 95% confidence interval, p05, p95, speedup, and parallel efficiency.

## Rendering Benchmarks

The render benchmark measures CPU-side frame cost, mesh synchronization cost, logical draw selection cost, triangle count, visible patches, and FPS. The CSV schema always contains `gpu_frame_ms`. The value is `nan` when no OpenGL GPU timer context is available.

Example:

```bash
./build-render/erosion bench render \
  --terrain perlinNoise \
  --width 1024 \
  --height 1024 \
  --frames 300 \
  --warmup 3 \
  --lod on \
  --culling on \
  --sync dirty \
  --gpu-timer off \
  --out benchmarks/results/render/lod_culling_dirty_sync
```

Render-specific options:

- `--lod on|off`
- `--culling on|off`
- `--sync full|dirty`
- `--frames <frame_count>`
- `--gpu-timer on|off`

Stable `render_frames.csv` schema:

```text
frame_id,cpu_frame_ms,gpu_frame_ms,total_frame_ms,sync_ms,draw_ms,triangles,visible_patches,total_patches,dirty_patches,fps
```

Typical render cases:

```bash
./build-render/erosion bench render --terrain perlinNoise --width 1024 --height 1024 --frames 300 --warmup 3 --lod off --culling off --sync full  --out benchmarks/results/render/baseline
./build-render/erosion bench render --terrain perlinNoise --width 1024 --height 1024 --frames 300 --warmup 3 --lod on  --culling off --sync full  --out benchmarks/results/render/lod
./build-render/erosion bench render --terrain perlinNoise --width 1024 --height 1024 --frames 300 --warmup 3 --lod off --culling on  --sync full  --out benchmarks/results/render/culling
./build-render/erosion bench render --terrain perlinNoise --width 1024 --height 1024 --frames 300 --warmup 3 --lod on  --culling on  --sync full  --out benchmarks/results/render/lod_culling
./build-render/erosion bench render --terrain perlinNoise --width 1024 --height 1024 --frames 300 --warmup 3 --lod on  --culling on  --sync dirty --out benchmarks/results/render/lod_culling_dirty_sync
```

## Interaction Benchmarks

This mode measures erosion, mesh synchronization, and render selection in the same loop.

```bash
./build-render/erosion bench interaction \
  --terrain perlinNoise \
  --width 1024 \
  --height 1024 \
  --frames 120 \
  --warmup 3 \
  --lod on \
  --culling on \
  --sync dirty \
  --out benchmarks/results/interaction
```

Generated file:

- `interaction_frames.csv`

## MPI Benchmarks

Run a small local MPI benchmark:

```bash
mpirun -np 2 ./build-cluster/erosion bench mpi \
  --terrain perlinNoise \
  --width 512 \
  --height 512 \
  --steps 10 \
  --warmup 1 \
  --runs 2 \
  --out benchmarks/results/mpi
```

Generated files:

- `mpi_raw_runs.csv`
- `mpi_summary_stats.csv`

The MPI benchmark records total time, compute time, communication time, and final mass error. Treat an MPI + OpenMP run as hybrid only when the kernel executed by each MPI rank actually uses OpenMP threads.

## Local Benchmark Matrix

Run the predefined local benchmark matrix:

```bash
./benchmarks/run_local_matrix.sh
```

Useful environment variables:

```bash
EXE=./build-headless/erosion RENDER_EXE=./build-render/erosion OUT=benchmarks/results ./benchmarks/run_local_matrix.sh
```

Case description files:

- `benchmarks/render_cases.csv`
- `benchmarks/erosion_local_cases.csv`

## Cluster Usage

Do not run long or expensive experiments on a login node. Use the login node for file transfer, configuration, compilation, and short smoke tests only. Submit benchmark jobs to compute nodes with SLURM.

Copy the project to the cluster:

```bash
rsync -avzp ./Simulateur-3D-Erosion-de-terrain/ mzen:Simulateur-3D-Erosion-de-terrain/
```

On the cluster, load the compiler and MPI modules required by the site, then build:

```bash
module avail
module load gcc/13.2.0
# module load <mpi_module>
cmake -S . -B build-cluster -DCMAKE_BUILD_TYPE=Release -DEROSION_ENABLE_RENDERING=OFF -DEROSION_ENABLE_MPI=ON -DEROSION_NATIVE_ARCH=ON
cmake --build build-cluster -j
```

Run a very small smoke test before submitting larger jobs:

```bash
srun --ntasks=2 --cpus-per-task=1 ./build-cluster/erosion bench mpi --terrain perlinNoise --width 64 --height 64 --steps 1 --warmup 1 --runs 1 --out benchmarks/results/smoke/mpi
```

Submit strong scaling jobs. Allocate at least as many tasks as the largest value in `RANKS_LIST`.

```bash
RANKS_LIST="1 2 4 8 16 32" sbatch --nodes=1 --ntasks-per-node=32 --cpus-per-task=1 benchmarks/cluster_strong_scaling.slurm
```

Submit weak scaling jobs:

```bash
RANKS_LIST="1 2 4 8 16 32" sbatch --nodes=1 --ntasks-per-node=32 --cpus-per-task=1 benchmarks/cluster_weak_scaling.slurm
```

The scripts use `srun`, which launches the MPI benchmark on the allocated compute resources.

## Plot Generation

Generate figures from CSV outputs:

```bash
./benchmarks/plot_benchmarks.py --results benchmarks/results --figures benchmarks/figures
```

Figures are written to `benchmarks/figures/`.

## Ignored Outputs

Benchmark results are ignored by Git:

```text
benchmarks/results/
```

This keeps generated CSV files and smoke-test outputs out of version control.

## Docker

Build the Docker image:

```bash
docker build -t erosion .
```

Run the graphical application with X11 forwarding:

```bash
xhost +local:
docker run -it -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix erosion
```
