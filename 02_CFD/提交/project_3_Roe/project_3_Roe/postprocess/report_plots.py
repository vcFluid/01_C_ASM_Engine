#!/usr/bin/env python3
"""Generate summary figures for the Project 3 Roe result report."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path


VARIABLES = ("rho", "u", "p")
VARIABLE_LABELS = {"rho": r"$\rho$", "u": r"$u$", "p": r"$p$"}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--results-dir",
        type=Path,
        default=Path("runs/ppt_formula_exact_only"),
    )
    parser.add_argument("--output-dir", type=Path, default=None)
    return parser.parse_args()


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8-sig", newline="") as stream:
        return list(csv.DictReader(stream))


def completed(rows: list[dict[str, str]]) -> list[dict[str, str]]:
    return [row for row in rows if row.get("status") == "completed"]


def setup_matplotlib():
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    plt.rcParams.update(
        {
            "font.size": 9,
            "axes.linewidth": 0.8,
            "axes.grid": True,
            "grid.color": "#dedede",
            "grid.linewidth": 0.5,
            "grid.alpha": 0.8,
            "legend.frameon": False,
        }
    )
    return plt


def plot_error_bars(
    plt,
    rows: list[dict[str, str]],
    metric: str,
    output_path: Path,
) -> None:
    labels = [row["case_no"] for row in rows]
    x = list(range(len(labels)))
    width = 0.24
    offsets = {"rho": -width, "u": 0.0, "p": width}
    colors = {"rho": "#3c6ea8", "u": "#b95745", "p": "#4f8a52"}

    fig, ax = plt.subplots(figsize=(7.0, 3.4), constrained_layout=True)
    for variable in VARIABLES:
        values = [float(row[f"{variable}_{metric}"]) for row in rows]
        ax.bar(
            [value + offsets[variable] for value in x],
            values,
            width=width,
            label=VARIABLE_LABELS[variable],
            color=colors[variable],
        )

    ax.set_xticks(x)
    ax.set_xticklabels(labels)
    ax.set_xlabel("case")
    ax.set_ylabel(metric)
    ax.set_yscale("log")
    ax.set_title(f"Roe FVM vs exact: {metric} error")
    ax.legend(ncols=3, loc="upper left")
    fig.savefig(output_path, dpi=180)
    plt.close(fig)


def main() -> int:
    args = parse_args()
    results_dir = args.results_dir
    output_dir = args.output_dir or results_dir / "report_figures"
    output_dir.mkdir(parents=True, exist_ok=True)

    plt = setup_matplotlib()

    baseline = completed(read_csv(results_dir / "baseline_exact_comparison.csv"))
    plot_error_bars(plt, baseline, "L1", output_dir / "baseline_l1_errors.png")
    plot_error_bars(plt, baseline, "Linf", output_dir / "baseline_linf_errors.png")

    print(f"[figures] {output_dir.resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
