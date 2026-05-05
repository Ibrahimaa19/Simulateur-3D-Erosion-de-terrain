#!/usr/bin/env bash

set -euo pipefail

mkdir -p results

awk 'FNR==1 && NR!=1 {next} {print}' results/*.csv > results/all_results.csv

echo "Wrote results/all_results.csv"
