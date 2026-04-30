#!/usr/bin/env python3
"""Figures du benchmark CPU d'erosion thermique."""

import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd
from matplotlib.ticker import MaxNLocator, ScalarFormatter


COLORS = {
    4: "#c05621",
    8: "#2b6cb0",
}

MODE_COLORS = {
    "indexed": "#2b6cb0",
    "normalized": "#c05621",
}

NEIGHBOR_LINESTYLES = {
    4: "--",
    8: "-",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Produit les figures du benchmark d'erosion thermique."
    )
    parser.add_argument(
        "--input",
        default="benchmarks/results/thermal_erosion_profile_summary.csv",
        help="CSV resume produit par analyze_thermal_erosion_profile.py.",
    )
    parser.add_argument(
        "--output-dir",
        default="benchmarks/figures_simulation",
        help="Dossier de sortie des figures.",
    )
    return parser.parse_args()


def configure_style() -> None:
    try:
        plt.style.use("seaborn-v0_8-whitegrid")
    except OSError:
        plt.style.use("ggplot")

    plt.rcParams.update(
        {
            "figure.figsize": (6.4, 4.0),
            "figure.dpi": 120,
            "savefig.dpi": 300,
            "font.size": 11,
            "axes.labelsize": 11,
            "axes.titlesize": 12,
            "legend.fontsize": 10,
            "lines.linewidth": 2.0,
            "lines.markersize": 6,
        }
    )


def metric_table(summary: pd.DataFrame, metric: str) -> pd.DataFrame:
    table = summary[summary["metric"] == metric].copy()
    if table.empty:
        raise ValueError(f"Metrique absente du resume: {metric}")
    sort_columns = ["neighbors", "terrain_size"]
    if "terrain_mode" in table.columns:
        sort_columns = ["terrain_mode", *sort_columns]
    return table.sort_values(sort_columns)


def series_columns(table: pd.DataFrame) -> list[str]:
    if "terrain_mode" in table.columns:
        return ["terrain_mode", "neighbors"]
    return ["neighbors"]


def series_label(keys: tuple, columns: list[str]) -> str:
    values = dict(zip(columns, keys))
    neighbors = int(values["neighbors"])
    if "terrain_mode" in values:
        return f"{values['terrain_mode']}, {neighbors} voisins"
    return f"{neighbors} voisins"


def series_style(keys: tuple, columns: list[str]) -> dict:
    values = dict(zip(columns, keys))
    neighbors = int(values["neighbors"])
    if "terrain_mode" in values:
        mode = str(values["terrain_mode"])
        return {
            "color": MODE_COLORS.get(mode, None),
            "linestyle": NEIGHBOR_LINESTYLES.get(neighbors, "-"),
        }
    return {
        "color": COLORS.get(neighbors, None),
        "linestyle": NEIGHBOR_LINESTYLES.get(neighbors, "-"),
    }


def plot_metric(summary: pd.DataFrame,
                metric: str,
                output_path: Path,
                title: str,
                ylabel: str,
                use_log_if_small: bool = False) -> None:
    table = metric_table(summary, metric)

    fig, ax = plt.subplots()
    columns = series_columns(table)
    for keys, group in table.groupby(columns, sort=True):
        if not isinstance(keys, tuple):
            keys = (keys,)
        style = series_style(keys, columns)
        ax.errorbar(
            group["terrain_size"],
            group["mean"],
            yerr=group["ci95"].fillna(0.0),
            fmt="o",
            capsize=4,
            color=style["color"],
            linestyle=style["linestyle"],
            label=series_label(keys, columns),
            elinewidth=1.2,
        )

    ax.set_title(title)
    ax.set_xlabel("Taille du terrain")
    ax.set_ylabel(ylabel)
    ax.xaxis.set_major_locator(MaxNLocator(integer=True))
    ax.grid(True, which="major", axis="both", alpha=0.35)

    if metric == "modified_cells":
        formatter = ScalarFormatter(useMathText=True)
        formatter.set_powerlimits((0, 0))
        ax.yaxis.set_major_formatter(formatter)

    if use_log_if_small:
        positive = table["mean"][table["mean"] > 0.0]
        if not positive.empty and positive.max() / positive.min() > 100.0:
            ax.set_yscale("log")

    if table.groupby(columns).ngroups > 1:
        ax.legend(frameon=True)

    fig.tight_layout()
    fig.savefig(output_path, bbox_inches="tight")
    plt.close(fig)
    print(f"Figure ecrite dans {output_path}")


def plot_total_time(summary: pd.DataFrame, output_path: Path) -> None:
    table = metric_table(summary, "iteration_ms")
    table["total_mean_ms"] = table["mean"] * table["count"]
    table["total_ci95_ms"] = table["ci95"].fillna(0.0) * table["count"]

    fig, ax = plt.subplots()
    columns = series_columns(table)
    for keys, group in table.groupby(columns, sort=True):
        if not isinstance(keys, tuple):
            keys = (keys,)
        style = series_style(keys, columns)
        ax.errorbar(
            group["terrain_size"],
            group["total_mean_ms"],
            yerr=group["total_ci95_ms"],
            fmt="o",
            capsize=4,
            color=style["color"],
            linestyle=style["linestyle"],
            label=series_label(keys, columns),
            elinewidth=1.2,
        )

    ax.set_title("Temps total d'erosion thermique")
    ax.set_xlabel("Taille du terrain")
    ax.set_ylabel("Temps total mesure (ms)")
    ax.xaxis.set_major_locator(MaxNLocator(integer=True))
    ax.grid(True, which="major", axis="both", alpha=0.35)

    if table.groupby(columns).ngroups > 1:
        ax.legend(frameon=True)

    fig.tight_layout()
    fig.savefig(output_path, bbox_inches="tight")
    plt.close(fig)
    print(f"Figure ecrite dans {output_path}")


def main() -> None:
    args = parse_args()
    input_path = Path(args.input)
    output_dir = Path(args.output_dir)

    summary = pd.read_csv(input_path)
    required = {"terrain_size", "neighbors", "metric", "count", "mean", "ci95"}
    missing = required.difference(summary.columns)
    if missing:
        raise ValueError(f"Colonnes manquantes dans le resume: {', '.join(sorted(missing))}")

    output_dir.mkdir(parents=True, exist_ok=True)
    configure_style()

    plot_metric(
        summary,
        "iteration_ms",
        output_dir / "thermal_iteration_time_vs_size.png",
        "Temps moyen d'une iteration d'erosion thermique",
        "Temps par iteration (ms)",
    )
    plot_total_time(summary, output_dir / "thermal_total_time_vs_size.png")
    plot_metric(
        summary,
        "modified_cells",
        output_dir / "thermal_modified_cells_vs_size.png",
        "Cellules modifiees par iteration",
        "Cellules modifiees",
    )
    plot_metric(
        summary,
        "mass_error",
        output_dir / "thermal_mass_error_vs_size.png",
        "Erreur relative de conservation de masse",
        "Erreur relative moyenne",
        use_log_if_small=True,
    )


if __name__ == "__main__":
    main()
