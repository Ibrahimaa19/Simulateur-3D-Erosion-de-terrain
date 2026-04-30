# Benchmark de simulation thermique

Ce benchmark mesure uniquement le cout CPU de l'algorithme d'erosion thermique.
Il ne cree aucune fenetre, n'initialise pas OpenGL et n'execute aucun rendu.

La base courante contient uniquement le voisinage 8 voisins. L'option
`--neighbors 4` est reservee pour une future implementation et echoue
explicitement tant que ce mode n'est pas disponible.

Le mode de terrain par defaut est `indexed`. Il fait dependre les hauteurs des
indices `x` et `z`, ce qui garde des variations locales comparables entre les
tailles 512, 1024, 2048 et 4096. Le mode `normalized` conserve l'ancien
generateur base sur des coordonnees ramenees dans `[0, 1]`.

## Compilation

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Executable :

```bash
./build/benchmarks/benchmark_thermal_erosion
```

## Benchmark brut

```bash
./build/benchmarks/benchmark_thermal_erosion \
  --terrain indexed \
  --sizes 512 1024 2048 4096 \
  --iterations 100 \
  --warmup 5 \
  --neighbors 8 \
  --output benchmarks/results/thermal_erosion_profile.csv
```

Colonnes produites :

```text
terrain_mode,terrain_size,neighbors,iteration,iteration_ms,total_elapsed_ms,total_cells,modified_cells,mass_before,mass_after,mass_error
```

Le terrain est genere une fois par taille avant les mesures. Les iterations de
warm-up modifient le terrain mais ne sont pas ecrites dans le CSV. Les colonnes
`mass_before`, `mass_after` et `mass_error` permettent de suivre la conservation
de masse a chaque iteration mesuree.

## Analyse statistique

```bash
python3 benchmarks/analyze_thermal_erosion_profile.py \
  --input benchmarks/results/thermal_erosion_profile.csv \
  --output benchmarks/results/thermal_erosion_profile_summary.csv
```

Le resume contient une ligne par taille, voisinage et metrique, avec :
`count`, `mean`, `median`, `std`, `min`, `q25`, `q75`, `max`, `ci95`.

## Figures

```bash
python3 benchmarks/plot_thermal_erosion_profile.py \
  --input benchmarks/results/thermal_erosion_profile_summary.csv \
  --output-dir benchmarks/figures_simulation
```

Figures produites :

- `thermal_iteration_time_vs_size.png`
- `thermal_total_time_vs_size.png`
- `thermal_modified_cells_vs_size.png`
- `thermal_mass_error_vs_size.png`

## Profilage MAQAO, perf et Hotspot

Compiler en Release avec symboles de debug :

```bash
cmake -S . -B build-profile \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo

cmake --build build-profile -j
```

Lancer un benchmark court mais representatif :

```bash
./build-profile/benchmarks/benchmark_thermal_erosion \
  --terrain indexed \
  --sizes 2048 \
  --iterations 100 \
  --warmup 5 \
  --neighbors 8 \
  --output benchmarks/results/thermal_erosion_profile_for_profiling.csv
```

Profilage MAQAO :

```bash
maqao oneview -R1 -- ./build-profile/benchmarks/benchmark_thermal_erosion \
  --terrain indexed \
  --sizes 2048 \
  --iterations 100 \
  --warmup 5 \
  --neighbors 8 \
  --output benchmarks/results/thermal_erosion_profile_for_profiling.csv
```

Mesures globales avec perf :

```bash
perf stat -d ./build-profile/benchmarks/benchmark_thermal_erosion \
  --terrain indexed \
  --sizes 2048 \
  --iterations 100 \
  --warmup 5 \
  --neighbors 8 \
  --output benchmarks/results/thermal_erosion_profile_for_profiling.csv
```

Profilage avec graphe d'appels :

```bash
perf record -g ./build-profile/benchmarks/benchmark_thermal_erosion \
  --terrain indexed \
  --sizes 2048 \
  --iterations 100 \
  --warmup 5 \
  --neighbors 8 \
  --output benchmarks/results/thermal_erosion_profile_for_profiling.csv

perf report
```

Pour Hotspot, lancer d'abord `perf record -g`, puis ouvrir le fichier
`perf.data` genere dans Hotspot.

Metriques importantes a regarder :

- temps passe dans `runThermalErosionStep` ;
- cache misses et localite memoire dans la boucle interne ;
- taux de branches ratees ;
- vectorisation possible ou absente ;
- hotspots de boucles ;
- efficacite d'acces memoire ;
- cout du voisinage 8 voisins par rapport au 4 voisins si les deux existent ;
- appels ou allocations inutiles dans la boucle interne.
