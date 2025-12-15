#!/bin/bash

if [ "$#" -lt 4 ]; then
    echo "Le nombre d'arguments est invalide, usage : validation.sh <type terrain> <start> <end> <step>"
    exit
fi 

# Variables
terrain=$1
debut=$2
fin=$3
step=$4

# Remplir le tableau
tableau=()
for ((i=debut; i<=fin; i+=step)); do
    tableau+=($i)
done

echo "Étapes: ${tableau[@]}"

for step in "${tableau[@]}"; do
    ./build/erosion test $terrain $step
done

