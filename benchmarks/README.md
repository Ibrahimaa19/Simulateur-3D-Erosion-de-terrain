# Benchmark de rendu graphique

Ce dossier contient un benchmark dedie au rendu OpenGL du terrain. Il ne lance
aucune erosion thermique ou hydraulique pendant les mesures. La boucle mesuree
contient uniquement le rendu, avec une option separee pour forcer le transfert
CPU vers GPU des sommets.

La synchronisation verticale est desactivee dans le benchmark. Les mesures CPU
utilisent `std::chrono` et un `glFinish()` apres la mise a jour GPU optionnelle
et apres le dessin, afin de mesurer un cout de rendu stabilise plutot qu'un
simple cout d'appel asynchrone.

## Compilation

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

L'executable est genere ici :

```bash
./build/benchmarks/benchmark_render
```

## Benchmark brut

Rendu statique, sans mise a jour GPU pendant les frames mesurees :

```bash
./build/benchmarks/benchmark_render \
  --sizes 512 1024 2048 \
  --warmup 50 \
  --frames 500 \
  --output benchmarks/results/render_profile.csv
```

Rendu avec mise a jour forcee du VBO a chaque frame :

```bash
./build/benchmarks/benchmark_render \
  --sizes 512 1024 2048 \
  --warmup 50 \
  --frames 500 \
  --force-buffer-update \
  --output benchmarks/results/render_profile_with_upload.csv
```

Dans ce mode, `buffer_update_ms` mesure l'appel `update_vertices_gpu()`, donc la
reconstruction des sommets cote CPU et le transfert VBO. L'IBO reste statique
car la topologie du terrain ne change pas.

Colonnes produites :

```text
terrain_size,frame_id,render_ms,buffer_update_ms,draw_ms,total_vertices,total_triangles,visible_patches,total_patches,fps
```

Dans l'etat actuel du rendu, il n'y a pas de LOD ni de frustum culling dans le
benchmark : le terrain complet est considere comme un seul patch visible.

## Analyse statistique

```bash
python3 benchmarks/analyze_render_profile.py \
  --input benchmarks/results/render_profile.csv \
  --output benchmarks/results/render_profile_summary.csv
```

Le resume contient une ligne par taille de terrain et par metrique, avec :
`count`, `mean`, `median`, `std`, `min`, `q25`, `q75`, `max`, `ci95`.

## Figures

```bash
python3 benchmarks/plot_render_profile.py \
  --input benchmarks/results/render_profile_summary.csv \
  --output-dir benchmarks/figures
```

Figures produites :

- `render_time_vs_size.png`
- `fps_vs_size.png`
- `triangles_vs_size.png`
- `buffer_update_cost_vs_size.png`

Les figures sont sauvegardees en PNG 300 dpi et peuvent etre inserees
directement dans un rapport LaTeX.
