#!/usr/bin/env python3

import matplotlib.pyplot as plt
import pandas as pd
import sys
import os
from matplotlib.ticker import MultipleLocator
from matplotlib.patches import Rectangle


args = sys.argv
if len(args) != 2:
    print(f"Usage: {args[0]} fin")
    sys.exit(1)

fin = int(args[1])

terrains = ["perlinNoise", "faulformation", "midpointDisplacement"]


# Figure avec 2 subplots : complet + zoom
fig, (ax_full, ax_zoom) = plt.subplots(2, 1, figsize=(14, 10), sharey=False)

# Plage zoom
zoom_start = 0
zoom_end = 40

for terrain in terrains:
    path = f"resultat/{terrain}/cellModified{fin}.csv"

    df = pd.read_csv(path)

    # ----- Plot complet -----
    ax_full.plot(df["step"]+1, df["cells_modified"], linewidth=2, label=terrain)

    # ----- Plot zoomé -----
    zoom_df = df[df["step"].between(zoom_start, zoom_end)]
    ax_zoom.plot(zoom_df["step"]+1, zoom_df["cells_modified"], linewidth=2, label=terrain)

y_min, y_max = ax_full.get_ylim()
rect = Rectangle(
    (zoom_start-30, y_min),   # coin bas gauche
    2*zoom_end, # largeur
    y_max - y_min,         # hauteur
    linewidth=1.5,
    edgecolor='red',
    facecolor='none',
    linestyle='--'
)
ax_full.add_patch(rect)


ax_full.set_title("Evolution du nombre de sommets modifiées en fonction du step")
ax_full.set_xlabel("Step")
ax_full.set_ylabel("Cells Modified")
ax_full.grid(True)
ax_full.legend()

ax_zoom.set_title(f"Evolution du nombre de sommets modifiées - zoom 0 à {zoom_end}")
ax_zoom.set_xlabel("Step")
ax_zoom.set_ylabel("Cells Modified")
ax_zoom.grid(True)
ax_zoom.legend()

fig.tight_layout()
fig.savefig("resultat/plot_cells_all_terrains_with_zoom.png")

# PLOT 2 : Error Time Step
fig2, ax2 = plt.subplots(figsize=(12, 5))

for terrain in terrains:
    path = f"resultat/{terrain}/errorTimeStep.csv"
    df = pd.read_csv(path)

    ax2.plot(
        df["step"],
        df["error"],
        linewidth=2,
        label=terrain
    )

ax2.set_xlabel("Step")
ax2.set_ylabel("Error")
ax2.set_title("Evolution de l'erreur en fonction du nombre de step")

ax2.grid(True)
ax2.legend()
fig2.savefig("resultat/plot_error_all_terrains.png")
