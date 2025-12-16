#!/usr/bin/env python3

import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import sys
import os

args = sys.argv
if len(args) != 5:
    print(f"Usage: {args[1]} type_terrain debut fin pas")
    sys.exit(1)

type_terrain = args[1]
debut = int(args[2])
fin = int(args[3])
pas = int(args[4])

steps = list(range(debut, fin + 1, pas))

data_path_cell = f"resultat/{type_terrain}/cellModified{fin}.csv"
data_path_error = f"resultat/{type_terrain}/errorTimeStep.csv"

#Plot
df_cell = pd.read_csv(data_path_cell)
df_error = pd.read_csv(data_path_error)

fig1, ax1 = plt.subplots(figsize=(12, 5))
ax1.plot(df_cell['step'], df_cell['cells_modified'], 'b-', linewidth=2)
ax1.set_xlabel('Step')
ax1.set_ylabel('Cells Modified')
ax1.set_title(f'Cells Modified - {type_terrain}')
ax1.grid(True)
fig1.savefig(f'resultat/{type_terrain}/plot_cells.png')

fig2, ax2 = plt.subplots(figsize=(12, 5))
ax2.plot(df_error['step'], df_error['error'], 'r-', linewidth=2)
ax2.set_xlabel('Step')
ax2.set_ylabel('Error')
ax2.set_title(f'Error Time Step - {type_terrain}')
ax2.grid(True)
fig2.savefig(f'resultat/{type_terrain}/plot_error.png')