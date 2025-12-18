import pandas as pd
import matplotlib.pyplot as plt
import os


DATA_DIR = "data"
OUTPUT_DIR = "plots"

os.makedirs(OUTPUT_DIR, exist_ok=True)

terrain_files = {
    "Fault Formation": "performance_faultFormation.csv",
    "Midpoint Displacement": "performance_midpointDisplacement.csv",
    "Perlin Noise": "performance_perlinNoise.csv"
}

# Couleurs cohérentes
colors = {
    512: "tab:blue",
    1024: "tab:orange",
    2048: "tab:green",
}

# Complexité : Temps vs Steps

for terrain, filename in terrain_files.items():
    df = pd.read_csv(os.path.join(DATA_DIR, filename))

    plt.figure(figsize=(8,6))

    for size in sorted(df["Size"].unique()):
        sub = df[df["Size"] == size]
        plt.plot(
            sub["Steps"],
            sub["Duration_ms"],
            marker='o',
            label=f"Size {size}",
            color=colors.get(size, None)
        )

    plt.xlabel("Nombre de steps")
    plt.ylabel("Temps d'exécution (ms)")
    plt.title(f"Complexité temporelle – {terrain}")
    plt.legend()
    plt.grid(True)

    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, f"{terrain.replace(' ', '_')}_complexity.png"))
    plt.close()

# Comparaison entre terrains

# Paramètres fixes pour la comparaison
COMPARE_SIZE = 1024
COMPARE_STEPS = 1000

comparison_data = []

for terrain, filename in terrain_files.items():
    df = pd.read_csv(os.path.join(DATA_DIR, filename))
    row = df[(df["Size"] == COMPARE_SIZE) & (df["Steps"] == COMPARE_STEPS)]

    if not row.empty:
        comparison_data.append({
            "Terrain": terrain,
            "Duration_ms": row["Duration_ms"].values[0]
        })

compare_df = pd.DataFrame(comparison_data)

plt.figure(figsize=(8,5))
plt.bar(compare_df["Terrain"], compare_df["Duration_ms"])

plt.ylabel("Temps d'exécution (ms)")
plt.xlabel("Type de terrain")
plt.title(f"Comparaison des terrains (Size={COMPARE_SIZE}, Steps={COMPARE_STEPS})")
plt.grid(axis='y')

plt.tight_layout()
plt.savefig(os.path.join(OUTPUT_DIR, "terrain_comparison.png"))
plt.close()

print("Tous les plots ont été générés dans le dossier 'plots/'")