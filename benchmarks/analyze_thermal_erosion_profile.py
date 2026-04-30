#!/usr/bin/env python3
"""Resume statistique des mesures CPU d'erosion thermique."""

import argparse
from pathlib import Path

import numpy as np
import pandas as pd


METRICS = [
    "iteration_ms",
    "modified_cells",
    "mass_error",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Calcule un resume statistique par taille et voisinage."
    )
    parser.add_argument(
        "--input",
        default="benchmarks/results/thermal_erosion_profile.csv",
        help="CSV brut produit par benchmark_thermal_erosion.",
    )
    parser.add_argument(
        "--output",
        default="benchmarks/results/thermal_erosion_profile_summary.csv",
        help="CSV resume a produire.",
    )
    return parser.parse_args()


def confidence_interval_95(values: pd.Series) -> float:
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
    required = ["terrain_size", "neighbors", *METRICS]
    missing = [column for column in required if column not in raw.columns]
    if missing:
        raise ValueError(f"Colonnes manquantes dans le CSV brut: {', '.join(missing)}")

    group_columns = ["terrain_size", "neighbors"]
    if "terrain_mode" in raw.columns:
        group_columns = ["terrain_mode", *group_columns]

    # Une ligne par couple mode/taille/voisinage/metrique facilite les comparaisons.
    for keys, group in raw.groupby(group_columns, sort=True):
        if not isinstance(keys, tuple):
            keys = (keys,)
        key_values = dict(zip(group_columns, keys))

        for metric in METRICS:
            rows.append(
                {
                    **key_values,
                    "metric": metric,
                    **summarize_metric(group, metric),
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
    summary.to_csv(output_path, index=False, float_format="%.10g")
    print(f"Resume ecrit dans {output_path}")


if __name__ == "__main__":
    main()
