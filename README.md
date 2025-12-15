# Simulateur-3D-Erosion-de-terrain
Projet de Programmation Numérique dans le cadre du M1 CHPS [UVSQ - Paris Saclay]

## Equipe du projet
- Ibrahima DIALLO
- Aboubacar-Bonfing SY
- Papa Moussa NIANG
- Amar LECHANI

**Encadrant:** Mathys JAM


## 0. Dépendance
Pour lancer le projet il est nécéssaire d'avoir sur sa machine les dépendance suivante : 
- 
- 
- 
-



## 1. Lancement des tests de validations

Le script de validations doit être lancer depuis le dossier source du projet
```bash
./validation/validation.sh <terrain> <start> <end> <step>
```
Où **terrain** représente les différents types de terrain qui sont : 
- loadHeighmap : le terrain chargé depuis iceland_heightmap.png
- faulFormation : le terrain généré par l'agorithme The Fault Formation
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
- faulFormation : le terrain généré par l'agorithme The Fault Formation
- midpointDisplacement :  le terrain généré par l'algorithme MidPoint Displacement
- perlinNoise :  le terrain généré par l'algorithme Perlin Noise

Et **step** représente le nombre d'itération d'érosion a tester.
