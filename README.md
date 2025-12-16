# Simulateur-3D-Erosion-de-terrain
Projet de Programmation Numérique dans le cadre du M1 CHPS [UVSQ - Paris Saclay]

## Equipe du projet
- Ibrahima DIALLO
- Aboubacar-Bonfing SY
- Papa Moussa NIANG
- Amar LECHANI

**Encadrant:** Mathys JAM


## Prérequis

- Système hôte : **Linux avec X11**
- Docker installé
- Une session graphique active (`DISPLAY` défini)

### 1. Créer l'image docker
Dans le dossier source du répertoire :

```bash
docker build -t erosion .
```

### 2. Autoriser les connexions X11 locales (sur l’hôte)

Sur la machine hôte, exécuter :

```bash
xhost +local:
```

Remarque : Cette autorisation est temporaire et sera réinitialisée au redémarrage.

### 3. Lancer le conteneur

```bash
docker run -it -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix erosion
```


## Si vous n'utiliser pas X11 ou n'arrivez pas a utiliser docker
Vous devez installer les dépendances suivantes : 
- libglew2.2 libglew-dev
- libglfw3 libglfw3-dev
- libgl1-mesa-dri 
- libx11-6 


## 1. Lancement des tests de validations

Le script de validations doit être lancer depuis le dossier source du projet
```bash
./validation/validation.sh <terrain> <start> <end> <step>
```
Où **terrain** représente les différents types de terrain qui sont : 
- loadHeighmap : le terrain chargé depuis iceland_heightmap.png
- faultFormation : le terrain généré par l'agorithme The Fault Formation
- midpointDisplacement :  le terrain généré par l'algorithme MidPoint Displacement
- perlinNoise :  le terrain généré par l'algorithme Perlin Noise

Et **start,end** représente respectivement la première borne, la dernière borne des tests. Le **step** spécifie l'évolution entre les bornes.


## 2. Lancement du projet

Le lancement du projet, s'effectue depuis le build/.

### Lancement graphique
```bash
./erosion render
```

### Lancement de validation
```bash
./erosion test <terrain> <step>
```
Où **terrain** représente les différents types de terrain qui sont : 
- loadHeighmap : le terrain chargé depuis iceland_heightmap.png
- faultFormation : le terrain généré par l'agorithme The Fault Formation
- midpointDisplacement :  le terrain généré par l'algorithme MidPoint Displacement
- perlinNoise :  le terrain généré par l'algorithme Perlin Noise

Et **step** représente le nombre d'itération d'érosion a tester.
