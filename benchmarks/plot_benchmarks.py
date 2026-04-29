#!/usr/bin/env python3

from __future__ import annotations

import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd


def save_if_present(fig, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fig.tight_layout()
    fig.savefig(path, dpi=160)
    plt.close(fig)


def plot_render(results: Path, figures: Path) -> None:
    direct = results / "render" / "render_frames.csv"
    frame_files = [direct] if direct.exists() else sorted((results / "render").glob("*/render_frames.csv"))
    if not frame_files:
        return

    df = pd.read_csv(frame_files[0])

    fig, ax = plt.subplots(figsize=(10, 5))
    ax.plot(df["frame_id"], df["cpu_frame_ms"], label="CPU frame")
    ax.plot(df["frame_id"], df["sync_ms"], label="Synchronisation")
    ax.plot(df["frame_id"], df["draw_ms"], label="Draw/selection")
    if "gpu_frame_ms" in df.columns and df["gpu_frame_ms"].notna().any():
        ax.plot(df["frame_id"], df["gpu_frame_ms"], label="GPU frame")
    ax.set_xlabel("Frame")
    ax.set_ylabel("Temps (ms)")
    ax.set_title("Benchmark rendu")
    ax.grid(True, alpha=0.3)
    ax.legend()
    save_if_present(fig, figures / "render_frame_times.png")

    fig, ax1 = plt.subplots(figsize=(10, 5))
    ax1.plot(df["frame_id"], df["triangles"], color="tab:blue", label="Triangles")
    ax1.set_xlabel("Frame")
    ax1.set_ylabel("Triangles", color="tab:blue")
    ax2 = ax1.twinx()
    ax2.plot(df["frame_id"], df["visible_patches"], color="tab:orange", label="Patches visibles")
    ax2.set_ylabel("Patches visibles", color="tab:orange")
    ax1.grid(True, alpha=0.3)
    fig.suptitle("Charge geometrique rendu")
    save_if_present(fig, figures / "render_geometry_load.png")

    if len(frame_files) > 1:
        rows = []
        for path in frame_files:
            case = path.parent.name
            case_df = pd.read_csv(path)
            rows.append(
                {
                    "case": case,
                    "cpu_frame_ms": case_df["cpu_frame_ms"].mean(),
                    "sync_ms": case_df["sync_ms"].mean(),
                    "draw_ms": case_df["draw_ms"].mean(),
                    "triangles": case_df["triangles"].mean(),
                    "fps": case_df["fps"].mean(),
                }
            )

        comp = pd.DataFrame(rows).sort_values("case")
        fig, ax = plt.subplots(figsize=(10, 5))
        ax.bar(comp["case"], comp["cpu_frame_ms"], label="CPU frame")
        ax.bar(comp["case"], comp["sync_ms"], label="Sync")
        ax.set_ylabel("Temps moyen (ms)")
        ax.set_title("Comparaison des optimisations de rendu")
        ax.tick_params(axis="x", rotation=35)
        ax.grid(True, axis="y", alpha=0.3)
        ax.legend()
        save_if_present(fig, figures / "render_optimization_comparison.png")


def plot_erosion(results: Path, figures: Path) -> None:
    summary_path = results / "erosion" / "summary_stats.csv"
    if not summary_path.exists():
        return

    df = pd.read_csv(summary_path)
    total = df[df["metric"] == "total_time_ms"].copy()
    if total.empty:
        return

    total["label"] = total["variant"].astype(str) + " / " + total["neighbors"].astype(str) + "v / t" + total[
        "threads"
    ].astype(str)
    total = total.sort_values(["variant", "neighbors", "threads"])

    fig, ax = plt.subplots(figsize=(max(10, len(total) * 0.45), 6))
    ax.bar(total["label"], total["mean"], yerr=total["stddev"], capsize=3)
    ax.set_ylabel("Temps total moyen (ms)")
    ax.set_title("Comparaison des variantes d'erosion")
    ax.tick_params(axis="x", rotation=70)
    ax.grid(True, axis="y", alpha=0.3)
    save_if_present(fig, figures / "erosion_variants_time.png")

    speed = total[total["threads"] > 1]
    if not speed.empty:
        fig, ax = plt.subplots(figsize=(10, 5))
        for (variant, neighbors), group in speed.groupby(["variant", "neighbors"]):
            group = group.sort_values("threads")
            ax.plot(group["threads"], group["speedup"], marker="o", label=f"{variant} {neighbors}v")
        ax.set_xlabel("Threads")
        ax.set_ylabel("Speedup")
        ax.set_title("Speedup OpenMP local")
        ax.grid(True, alpha=0.3)
        ax.legend()
        save_if_present(fig, figures / "erosion_openmp_speedup.png")


def plot_mpi(results: Path, figures: Path) -> None:
    summary_path = results / "mpi" / "mpi_summary_stats.csv"
    if not summary_path.exists():
        return

    df = pd.read_csv(summary_path)
    total = df[df["metric"] == "total_time_ms"].copy()
    comm = df[df["metric"] == "communication_ms"].copy()
    compute = df[df["metric"] == "compute_ms"].copy()
    if total.empty:
        return

    fig, ax = plt.subplots(figsize=(8, 5))
    ax.plot(total["ranks"], total["mean"], marker="o", label="Total")
    if not compute.empty:
        ax.plot(compute["ranks"], compute["mean"], marker="o", label="Calcul")
    if not comm.empty:
        ax.plot(comm["ranks"], comm["mean"], marker="o", label="Communication")
    ax.set_xlabel("Rangs MPI")
    ax.set_ylabel("Temps moyen (ms)")
    ax.set_title("Scaling MPI")
    ax.grid(True, alpha=0.3)
    ax.legend()
    save_if_present(fig, figures / "mpi_scaling_times.png")


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate report plots from erosion benchmark CSV files.")
    parser.add_argument("--results", default="benchmarks/results", type=Path)
    parser.add_argument("--figures", default="benchmarks/figures", type=Path)
    args = parser.parse_args()

    plot_render(args.results, args.figures)
    plot_erosion(args.results, args.figures)
    plot_mpi(args.results, args.figures)

    print(f"Figures written to {args.figures}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
