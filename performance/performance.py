import os
import pandas as pd
import matplotlib.pyplot as plt

DATA_DIR = "data"
PLOT_DIR = "plots"

os.makedirs(PLOT_DIR, exist_ok=True)

files = {
    "Fault Formation": "performance_faultFormation.csv",
    "Midpoint Displacement": "performance_midpointDisplacement.csv",
    "Perlin Noise": "performance_perlinNoise.csv"
}

plt.figure(figsize=(7, 5))

for terrain_name, filename in files.items():
    df = pd.read_csv(os.path.join(DATA_DIR, filename))
    plt.plot(
        df["Size"],
        df["Duration_ms"],
        marker='o',
        label=terrain_name
    )

plt.xlabel("Taille du terrain")
plt.ylabel("Temps d'exécution (ms)")
plt.title("Complexité temporelle de Thermal Erosion selon le type de terrain")
plt.legend()
plt.grid(True)

plt.tight_layout()
plt.savefig(os.path.join(PLOT_DIR, "complexity_comparison_all_terrains.png"), dpi=300)
plt.close()

print("Plot généré : plots/complexity_comparison_all_terrains.png")