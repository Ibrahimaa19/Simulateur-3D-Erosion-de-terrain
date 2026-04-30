#!/usr/bin/env python3
"""Figures pretes pour le rapport a partir du resume du benchmark de rendu."""

import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd
from matplotlib.ticker import MaxNLocator, ScalarFormatter


PLOTS = [
    (
        "render_ms",
        "render_time_vs_size.png",
        "Temps moyen de rendu",
        "Temps de rendu (ms)",
        True,
    ),
    (
        "fps",
        "fps_vs_size.png",
        "FPS moyen",
        "FPS",
        True,
    ),
    (
        "total_triangles",
        "triangles_vs_size.png",
        "Triangles rendus",
        "Triangles",
        False,
    ),
    (
        "buffer_update_ms",
        "buffer_update_cost_vs_size.png",
        "Cout de mise a jour des buffers GPU",
        "Temps de mise a jour (ms)",
        True,
    ),
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Produit les figures du benchmark de rendu."
    )
    parser.add_argument(
        "--input",
        default="benchmarks/results/render_profile_summary.csv",
        help="CSV resume produit par analyze_render_profile.py.",
    )
    parser.add_argument(
        "--output-dir",
        default="benchmarks/figures",
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
    return table.sort_values("terrain_size")


def plot_metric(summary: pd.DataFrame,
                metric: str,
                filename: str,
                title: str,
                ylabel: str,
                with_error_bars: bool,
                output_dir: Path) -> None:
    table = metric_table(summary, metric)
    x = table["terrain_size"]
    y = table["mean"]
    yerr = table["ci95"].fillna(0.0) if with_error_bars else None

    fig, ax = plt.subplots()
    ax.errorbar(
        x,
        y,
        yerr=yerr,
        fmt="o-",
        capsize=4 if with_error_bars else 0,
        color="#2b6cb0",
        ecolor="#4a5568",
        elinewidth=1.2,
    )

    ax.set_title(title)
    ax.set_xlabel("Taille du terrain")
    ax.set_ylabel(ylabel)
    ax.xaxis.set_major_locator(MaxNLocator(integer=True))
    ax.grid(True, which="major", axis="both", alpha=0.35)

    if metric in {"total_triangles", "total_vertices"}:
        formatter = ScalarFormatter(useMathText=True)
        formatter.set_powerlimits((0, 0))
        ax.yaxis.set_major_formatter(formatter)

    fig.tight_layout()
    output_path = output_dir / filename
    fig.savefig(output_path, bbox_inches="tight")
    plt.close(fig)
    print(f"Figure ecrite dans {output_path}")


def main() -> None:
    args = parse_args()
    input_path = Path(args.input)
    output_dir = Path(args.output_dir)

    summary = pd.read_csv(input_path)
    required = {"terrain_size", "metric", "mean", "ci95"}
    missing = required.difference(summary.columns)
    if missing:
        raise ValueError(f"Colonnes manquantes dans le resume: {', '.join(sorted(missing))}")

    output_dir.mkdir(parents=True, exist_ok=True)
    configure_style()

    for metric, filename, title, ylabel, with_error_bars in PLOTS:
        plot_metric(summary, metric, filename, title, ylabel, with_error_bars, output_dir)


if __name__ == "__main__":
    main()
