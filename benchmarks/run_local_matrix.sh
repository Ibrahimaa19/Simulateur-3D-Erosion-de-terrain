#!/bin/bash

set -euo pipefail

EXE=${EXE:-./build-headless/erosion}
RENDER_EXE=${RENDER_EXE:-./build-render/erosion}
OUT=${OUT:-benchmarks/results}
TERRAIN=${TERRAIN:-perlinNoise}
WARMUP=${WARMUP:-3}
RUNS=${RUNS:-10}

mkdir -p "${OUT}"

echo "== Profilage simulation seule =="
for size in 512 1024 2048; do
    "${EXE}" bench erosion \
        --terrain "${TERRAIN}" \
        --width "${size}" \
        --height "${size}" \
        --steps 100 \
        --warmup "${WARMUP}" \
        --runs "${RUNS}" \
        --variant blocked \
        --neighbors 8 \
        --threads 1 \
        --out "${OUT}/profiling/erosion_${size}"
done

echo "== Profilage rendu seul =="
for size in 512 1024 2048; do
    "${RENDER_EXE}" bench render \
        --terrain "${TERRAIN}" \
        --width "${size}" \
        --height "${size}" \
        --frames 300 \
        --warmup "${WARMUP}" \
        --lod on \
        --culling on \
        --sync dirty \
        --gpu-timer off \
        --out "${OUT}/profiling/render_${size}"
done

echo "== Interaction rendu / simulation =="
for size in 512 1024 2048; do
    "${RENDER_EXE}" bench interaction \
        --terrain "${TERRAIN}" \
        --width "${size}" \
        --height "${size}" \
        --frames 120 \
        --warmup "${WARMUP}" \
        --lod on \
        --culling on \
        --sync dirty \
        --out "${OUT}/profiling/interaction_${size}"
done

echo "== Optimisations de rendu =="
declare -A render_cases=(
    [baseline]="--lod off --culling off --sync full"
    [lod]="--lod on --culling off --sync full"
    [culling]="--lod off --culling on --sync full"
    [lod_culling]="--lod on --culling on --sync full"
    [lod_culling_dirty_sync]="--lod on --culling on --sync dirty"
)

for name in "${!render_cases[@]}"; do
    "${RENDER_EXE}" bench render \
        --terrain "${TERRAIN}" \
        --width 1024 \
        --height 1024 \
        --frames 300 \
        --warmup "${WARMUP}" \
        ${render_cases[$name]} \
        --gpu-timer off \
        --out "${OUT}/render/${name}"
done

echo "== Erosion locale / OpenMP =="
"${EXE}" bench erosion \
    --terrain "${TERRAIN}" \
    --width 1024 \
    --height 1024 \
    --steps 100 \
    --warmup "${WARMUP}" \
    --runs "${RUNS}" \
    --variant all \
    --neighbors-list 4,8 \
    --threads-list 1,2,4,8 \
    --out "${OUT}/erosion"

echo "Matrice locale terminee. CSV dans ${OUT}"
