# Benchmarks rapport

Les resultats complets sont ecrits dans `benchmarks/results/`, ignore par Git.

## Commandes principales

```bash
./build-headless/erosion bench erosion --terrain perlinNoise --width 1024 --height 1024 --steps 100 --warmup 3 --runs 10 --variant all --neighbors-list 4,8 --threads-list 1,2,4,8 --out benchmarks/results/erosion
./build-render/erosion bench render --terrain perlinNoise --width 1024 --height 1024 --frames 300 --warmup 3 --lod on --culling on --sync dirty --gpu-timer off --out benchmarks/results/render/lod_culling_dirty_sync
./build-render/erosion bench interaction --terrain perlinNoise --width 1024 --height 1024 --frames 120 --warmup 3 --lod on --culling on --sync dirty --out benchmarks/results/interaction
mpirun -np 2 ./build-cluster/erosion bench mpi --terrain perlinNoise --width 512 --height 512 --steps 10 --warmup 1 --runs 2 --out benchmarks/results/mpi
```

## Matrice locale

```bash
./benchmarks/run_local_matrix.sh
```

## Plots

```bash
./benchmarks/plot_benchmarks.py --results benchmarks/results --figures benchmarks/figures
```

## Notes

- Les warm-ups sont ecrits dans les CSV bruts mais exclus des statistiques.
- `gpu_frame_ms` vaut `nan` quand aucun contexte OpenGL de timing GPU n'est cree.
- La version hybride MPI + OpenMP ne doit etre discutee que si le noyau appele par chaque rang MPI exploite reellement OpenMP.
