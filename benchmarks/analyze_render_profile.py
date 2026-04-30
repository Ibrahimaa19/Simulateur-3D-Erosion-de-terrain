#!/usr/bin/env python3
"""Resume statistique des mesures brutes du benchmark de rendu."""

import argparse
from pathlib import Path

import numpy as np
import pandas as pd


METRICS = [
    "render_ms",
    "buffer_update_ms",
    "draw_ms",
    "fps",
    "total_vertices",
    "total_triangles",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Calcule un resume statistique par taille de terrain."
    )
    parser.add_argument(
        "--input",
        default="benchmarks/results/render_profile.csv",
        help="CSV brut produit par benchmark_render.",
    )
    parser.add_argument(
        "--output",
        default="benchmarks/results/render_profile_summary.csv",
        help="CSV resume a produire.",
    )
    return parser.parse_args()


def confidence_interval_95(values: pd.Series) -> float:
    """Retourne l'erreur de confiance a 95 %, si elle est estimable."""
    clean = pd.to_numeric(values, errors="coerce").dropna()
    if len(clean) < 2:
        return np.nan
    return 1.96 * clean.std(ddof=1) / np.sqrt(len(clean))


def summarize_metric(group: pd.DataFrame, metric: str) -> dict:
    values = pd.to_numeric(group[metric], errors="coerce").dropna()

    if values.empty:
        return {
            "count": 0,
            "mean": np.nan,
            "median": np.nan,
            "std": np.nan,
            "min": np.nan,
            "q25": np.nan,
            "q75": np.nan,
            "max": np.nan,
            "ci95": np.nan,
        }

    return {
        "count": int(values.count()),
        "mean": values.mean(),
        "median": values.median(),
        "std": values.std(ddof=1) if len(values) > 1 else np.nan,
        "min": values.min(),
        "q25": values.quantile(0.25),
        "q75": values.quantile(0.75),
        "max": values.max(),
        "ci95": confidence_interval_95(values),
    }


def build_summary(raw: pd.DataFrame) -> pd.DataFrame:
    rows = []

    missing = [column for column in ["terrain_size", *METRICS] if column not in raw.columns]
    if missing:
        raise ValueError(f"Colonnes manquantes dans le CSV brut: {', '.join(missing)}")

    # Une ligne par couple taille/metrique garde le CSV compact et facile a tracer.
    for terrain_size, group in raw.groupby("terrain_size", sort=True):
        for metric in METRICS:
            stats = summarize_metric(group, metric)
            rows.append(
                {
                    "terrain_size": terrain_size,
                    "metric": metric,
                    **stats,
                }
            )

    return pd.DataFrame(rows)


def main() -> None:
    args = parse_args()
    input_path = Path(args.input)
    output_path = Path(args.output)

    raw = pd.read_csv(input_path)
    summary = build_summary(raw)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    summary.to_csv(output_path, index=False, float_format="%.6f")
    print(f"Resume ecrit dans {output_path}")


if __name__ == "__main__":
    main()
