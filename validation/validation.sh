#!/bin/bash

set -e
set -o pipefail

if [ "$#" -lt 4 ]; then
    echo "Usage: $0 <typeTerrain> <start> <end> <step>"
    exit 1
fi

terrain=$1
debut=$2
fin=$3
pas=$4

RESULT_DIR="resultat/$terrain"
LOG_FILE="$RESULT_DIR/run_all.log"
ERROR_FINAL="$RESULT_DIR/errorTimeStep.csv"

mkdir -p "$RESULT_DIR"

{
    echo "=== VALIDATION ÉROSION THERMIQUE ==="
    echo "Terrain        : $terrain"
    echo "Steps range    : $debut -> $fin (pas = $pas)"
    echo "Timestamp      : $(date)"
    echo

    if [ ! -f ./build/erosion ]; then
        echo "Error: ./build/erosion introuvable."
        echo "Veuillez compiler le projet avant de lancer la validation."
        exit 1
    fi

    echo "step,error" > "$ERROR_FINAL"

    last_step=0

    for ((s=debut; s<=fin; s+=pas)); do
        echo
        echo ">>> Running: erosion test $terrain $s"

        ./build/erosion test "$terrain" "$s"

        ERROR_FILE="$RESULT_DIR/errorTimeStep${s}.csv"

        if [ ! -f "$ERROR_FILE" ]; then
            echo "Error: fichier $ERROR_FILE manquant."
            exit 1
        fi

        error_value=$(cat "$ERROR_FILE")
        echo "$s,$error_value" >> "$ERROR_FINAL"

        last_step=$s
    done
    
    echo
    echo "Cleaning intermediate files..."

    find "$RESULT_DIR" -maxdepth 1 -type f -name 'errorTimeStep*.csv' \
        ! -name 'errorTimeStep.csv' -delete

    find "$RESULT_DIR" -maxdepth 1 -type f -name 'cellModified*.csv' \
        ! -name "cellModified${last_step}.csv" -delete

    echo
    echo "Validation terminée avec succès."
    echo "Résultats disponibles dans : $RESULT_DIR"

} 2>&1 | tee "$LOG_FILE"
