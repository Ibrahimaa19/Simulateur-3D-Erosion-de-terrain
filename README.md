# Simulateur 3D d'érosion de terrain

Projet de Programmation Numérique réalisé dans le cadre du Master 1 CHPS  
**UVSQ - Paris-Saclay**

Ce projet propose un simulateur 3D de terrain avec génération procédurale, érosion thermique, rendu OpenGL interactif, tests de validation et mode d'exécution MPI.

## Équipe du projet

- Ibrahima DIALLO
- Aboubacar-Bonfing SY
- Papa Moussa NIANG
- Amar LECHANI

**Encadrant :** Mathys JAM

---

## Fonctionnalités principales

Le projet permet de :

- charger un terrain depuis une heightmap ;
- générer un terrain avec l'algorithme Fault Formation ;
- générer un terrain avec l'algorithme Midpoint Displacement ;
- générer un terrain avec le bruit de Perlin ;
- appliquer une érosion thermique ;
- visualiser le terrain en 3D avec OpenGL, GLFW, GLEW et ImGui ;
- lancer des tests de validation ;
- exécuter une version MPI lorsque MPI est disponible ;
- compiler le projet avec ou sans rendu graphique.

---

## Prérequis

### Dépendances générales

Le projet nécessite :

- CMake 3.16 ou supérieur ;
- un compilateur C++ compatible C++17 ;
- OpenMP ;
- MPI si le mode MPI est activé ;
- OpenGL, GLFW, GLEW et GLM si le rendu graphique est activé.

### Installation des dépendances sous Ubuntu/Debian

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    libomp-dev \
    openmpi-bin \
    libopenmpi-dev \
    libglew-dev \
    libglfw3-dev \
    libglm-dev \
    libgl1-mesa-dev \
    libx11-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev
```

---

## Compilation

Depuis la racine du projet :

```bash
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j
```

L'exécutable généré est :

```bash
./erosion
```

---

## Options CMake disponibles

Le projet définit plusieurs options dans `CMakeLists.txt`.

| Option | Valeur par défaut | Description |
|---|---:|---|
| `EROSION_ENABLE_RENDERING` | `ON` | Active le rendu OpenGL/GLFW/ImGui |
| `EROSION_ENABLE_MPI` | `ON` | Active le mode MPI |
| `EROSION_ENABLE_TESTS` | `ON` | Active les tests CTest |
| `EROSION_NATIVE_ARCH` | `OFF` | Active `-march=native` pour les benchmarks locaux |

### Compilation complète

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j
```

### Compilation sans rendu graphique

Utile sur une machine sans OpenGL ou sans serveur X11 :

```bash
cmake -DCMAKE_BUILD_TYPE=Release -DEROSION_ENABLE_RENDERING=OFF ..
cmake --build . -j
```

### Compilation sans MPI

Utile si MPI n'est pas installé :

```bash
cmake -DCMAKE_BUILD_TYPE=Release -DEROSION_ENABLE_MPI=OFF ..
cmake --build . -j
```

### Compilation avec optimisations locales

À utiliser pour les mesures de performance sur la machine locale :

```bash
cmake -DCMAKE_BUILD_TYPE=Release -DEROSION_NATIVE_ARCH=ON ..
cmake --build . -j
```

---

## Utilisation de l'exécutable

L'exécutable accepte trois modes :

```bash
./erosion render
./erosion test <typeTerrain> <steps>
./erosion MPI <typeTerrain> <width> <height> <steps>
```

Le mode MPI accepte aussi `mpi` en minuscules.

Les types de terrain disponibles sont :

| Type | Description |
|---|---|
| `loadHeightmap` | Charge le terrain depuis `../src/heightmap/heightmap.png` |
| `faultFormation` | Génère un terrain avec Fault Formation |
| `midpointDisplacement` | Génère un terrain avec Midpoint Displacement |
| `perlinNoise` | Génère un terrain avec le bruit de Perlin |

---

## 1. Lancement du rendu graphique

Depuis le dossier `build/` :

```bash
./erosion render
```

Ce mode lance l'application graphique OpenGL.

Il nécessite que le projet soit configuré avec :

```bash
-DEROSION_ENABLE_RENDERING=ON
```

Si le rendu est désactivé, le programme affiche un message indiquant que le mode rendu n'est pas disponible.

---

## 2. Lancement des tests de validation

Depuis le dossier `build/` :

```bash
./erosion test <typeTerrain> <steps>
```

Exemples :

```bash
./erosion test loadHeightmap 100
./erosion test faultFormation 100
./erosion test midpointDisplacement 100
./erosion test perlinNoise 100
```

Le paramètre `steps` correspond au nombre d'itérations d'érosion thermique à tester.

---

## 3. Lancement avec le script de validation

Le script de validation se lance depuis la racine du projet.

Avant de lancer le script, le projet doit être compilé dans le dossier `build/`, car le script utilise l'exécutable :

```bash
./build/erosion
```

Commande générale :

```bash
./validation/validation.sh <terrain> <start> <end> <step>
```

Exemple :

```bash
./validation/validation.sh perlinNoise 10 100 10
```

Les paramètres sont :

| Paramètre | Description |
|---|---|
| `terrain` | Type de terrain à tester |
| `start` | Première valeur du nombre d'itérations |
| `end` | Dernière valeur du nombre d'itérations |
| `step` | Pas entre deux valeurs testées |

Exemple : avec `start=10`, `end=100` et `step=10`, le script lance les tests pour 10, 20, 30, ..., 100 itérations.

Les résultats sont écrits dans :

```bash
resultat/<terrain>/
```

---

## 4. Lancement du mode MPI

Le mode MPI utilise la forme suivante :

```bash
./erosion MPI <typeTerrain> <width> <height> <steps>
```

Exemple avec 4 processus MPI :

```bash
mpirun -np 4 ./erosion MPI perlinNoise 2048 2048 100
```

Autre exemple :

```bash
mpirun -np 4 ./erosion MPI faultFormation 2048 2048 100
```

Ce mode nécessite que le projet soit configuré avec :

```bash
-DEROSION_ENABLE_MPI=ON
```

Si MPI est désactivé, le programme affiche un message indiquant que le mode MPI n'est pas disponible.

---

## Tests CMake

Lorsque l'option `EROSION_ENABLE_TESTS=ON` est activée, les tests sont disponibles avec CTest.

Depuis le dossier `build/` :

```bash
ctest --output-on-failure
```

---

## Utilisation avec Docker

Le dépôt contient un `Dockerfile` permettant d'installer les dépendances graphiques nécessaires au projet.

### 1. Construire l'image Docker

Depuis la racine du projet :

```bash
docker build -t erosion .
```

### 2. Autoriser les connexions X11 locales

Sur la machine hôte :

```bash
xhost +local:
```

Cette autorisation est temporaire.

### 3. Lancer le conteneur avec X11

```bash
docker run -it \
    -e DISPLAY=$DISPLAY \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    erosion
```

Une fois dans le conteneur, compiler le projet :

```bash
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j
```

Puis lancer le rendu :

```bash
./erosion render
```

---

## Formatage du code

Si `clang-format` est disponible, deux cibles CMake sont ajoutées.

Pour formater les fichiers :

```bash
cmake --build . --target format
```

Pour vérifier le formatage sans modifier les fichiers :

```bash
cmake --build . --target format-check
```

---

## Arborescence générale

```text
.
├── include/              # Fichiers d'en-tête
├── shaders/              # Shaders OpenGL
├── src/                  # Code source principal
│   ├── main.cpp
│   ├── Terrain.cpp
│   ├── ThermalErosion.cpp
│   ├── FaultFormationTerrain.cpp
│   ├── MidpointDisplacement.cpp
│   ├── PerlinNoiseTerrain.cpp
│   ├── TerrainApp.cpp
│   ├── RendererManager.cpp
│   └── ...
├── tests/                # Tests CMake
├── validation/           # Script de validation
├── Dockerfile
├── CMakeLists.txt
├── LICENSE
└── README.md
```

---

## Exemples rapides

Compilation complète :

```bash
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j
```

Lancement graphique :

```bash
./erosion render
```

Validation sur bruit de Perlin :

```bash
./erosion test perlinNoise 100
```

Validation avec script :

```bash
cd ..
./validation/validation.sh perlinNoise 10 100 10
```

Lancement MPI :

```bash
cd build
mpirun -np 4 ./erosion MPI perlinNoise 2048 2048 100
```

---

## Remarques importantes
- Le mode `render` nécessite une session graphique active.
- Le mode `test` peut être utilisé sans rendu graphique si le projet est compilé avec `EROSION_ENABLE_RENDERING=OFF`.
- Le mode `MPI` nécessite MPI et l'option `EROSION_ENABLE_MPI=ON`.
- L'option `EROSION_NATIVE_ARCH=ON` est utile pour les benchmarks locaux, mais elle peut rendre l'exécutable moins portable.