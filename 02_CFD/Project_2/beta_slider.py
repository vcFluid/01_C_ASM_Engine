#!/usr/bin/env python3
"""Interactive beta slider for Project 2 beta-sweep results.

This is a lightweight exploration tool.  It does not replace Tecplot final
figures; it helps inspect how the numerical solution changes with beta.
"""

from __future__ import annotations

import argparse
import csv
from dataclasses import dataclass
import os
from pathlib import Path
import sys


PROJECT_DIR = Path(__file__).resolve().parent
POSTPROCESS_DIR = PROJECT_DIR / "postprocess"
if os.fspath(POSTPROCESS_DIR) not in sys.path:
    sys.path.insert(0, os.fspath(POSTPROCESS_DIR))

from tecplot_compare import Y_LABELS, padded_range, read_tecplot_ascii  # noqa: E402


VARIABLES = ("rho", "u", "p")


@dataclass(frozen=True)
class BetaFrame:
    beta: float
    status: str
    reason: str
    solution_time: float | None
    directory: Path
    numerical: dict[str, list[float]]
    exact: dict[str, list[float]]
    metrics: dict[str, dict[str, float]]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Open an interactive Matplotlib slider for existing beta-sweep "
            "data.  The slider is discrete: every stop corresponds to one "
            "available beta directory."
        )
    )
    parser.add_argument(
        "--sweep-dir",
        type=Path,
        default=PROJECT_DIR / "runs" / "Solution_01_Sod" / "beta_sweep",
        help="directory produced by postprocess/beta_sweep.py",
    )
    parser.add_argument(
        "--include-unstable",
        action="store_true",
        help="also show unstable beta runs when their data files exist",
    )
    parser.add_argument(
        "--save-preview",
        type=Path,
        default=None,
        help="save the first slider state as PNG and exit instead of opening GUI",
    )
    parser.add_argument("--line-width-exact", type=float, default=0.9)
    parser.add_argument("--line-width-num", type=float, default=0.9)
    parser.add_argument("--symbol-size", type=float, default=2.0)
    parser.add_argument("--padding", type=float, default=0.05)
    return parser.parse_args()


def beta_dir_name(beta: float) -> str:
    token = f"{beta:.10g}".replace("-", "m").replace(".", "p")
    return f"beta_{token}"


def read_error_table(path: Path) -> dict[float, dict[str, object]]:
    if not path.is_file():
        raise FileNotFoundError(path)

    table: dict[float, dict[str, object]] = {}
    with path.open("r", encoding="ascii", newline="") as stream:
        reader = csv.DictReader(stream)
        for row in reader:
            beta = float(row["beta"])
            entry = table.setdefault(
                beta,
                {
                    "status": row["status"],
                    "reason": row["reason"],
                    "solution_time": (
                        float(row["solution_time"])
                        if row["solution_time"]
                        else None
                    ),
                    "metrics": {},
                },
            )
            if row["status"] == "completed":
                entry["metrics"][row["variable"]] = {
                    "L1": float(row["L1"]),
                    "L2": float(row["L2"]),
                    "Linf": float(row["Linf"]),
                    "TV": float(row["TV"]),
                    "undershoot": float(row["undershoot"]),
                    "overshoot": float(row["overshoot"]),
                }
    return table


def load_frames(sweep_dir: Path, include_unstable: bool) -> list[BetaFrame]:
    sweep_dir = sweep_dir.resolve()
    error_table = read_error_table(sweep_dir / "errors.csv")
    frames: list[BetaFrame] = []

    for beta in sorted(error_table):
        entry = error_table[beta]
        status = str(entry["status"])
        if status != "completed" and not include_unstable:
            continue

        directory = sweep_dir / beta_dir_name(beta)
        numerical_path = directory / "numerical.dat"
        exact_path = directory / "numerical_exact.dat"
        if not numerical_path.is_file() or not exact_path.is_file():
            if status == "completed":
                raise FileNotFoundError(f"Missing data files in {directory}")
            continue

        frames.append(
            BetaFrame(
                beta=beta,
                status=status,
                reason=str(entry["reason"]),
                solution_time=entry["solution_time"],
                directory=directory,
                numerical=read_tecplot_ascii(numerical_path),
                exact=read_tecplot_ascii(exact_path),
                metrics=entry["metrics"],
            )
        )

    if not frames:
        raise RuntimeError(f"No plottable beta frames found in {sweep_dir}")
    return frames


def global_ranges(
    frames: list[BetaFrame],
    padding: float,
) -> dict[str, tuple[float, float]]:
    ranges: dict[str, tuple[float, float]] = {}
    for variable in VARIABLES:
        values: list[float] = []
        for frame in frames:
            values.extend(frame.numerical[variable])
            values.extend(frame.exact[variable])
        ranges[variable] = padded_range(
            values,
            padding_fraction=padding,
            nonnegative=variable in {"rho", "p"},
        )
    return ranges


def format_metrics(frame: BetaFrame) -> str:
    if frame.status != "completed":
        return f"status={frame.status}, {frame.reason}"

    parts = []
    for variable in VARIABLES:
        metric = frame.metrics.get(variable)
        if metric is None:
            continue
        parts.append(
            f"{variable}: L1={metric['L1']:.3e}, "
            f"L2={metric['L2']:.3e}, Linf={metric['Linf']:.3e}"
        )
    return " | ".join(parts)


def make_figure(frames: list[BetaFrame], padding: float, args: argparse.Namespace):
    import matplotlib.pyplot as plt
    from matplotlib.widgets import Slider

    ranges = global_ranges(frames, padding)
    first = frames[0]

    fig, axes = plt.subplots(3, 1, figsize=(12.8, 8.2), sharex=True)
    fig.subplots_adjust(left=0.08, right=0.98, top=0.90, bottom=0.18, hspace=0.18)
    title = fig.suptitle("")
    info_text = fig.text(0.08, 0.06, "", ha="left", va="center", fontsize=9)

    exact_lines = {}
    numerical_lines = {}
    for axis, variable in zip(axes, VARIABLES):
        (exact_line,) = axis.plot(
            first.exact["x"],
            first.exact[variable],
            color="black",
            linewidth=args.line_width_exact,
            label="Exact",
        )
        (numerical_line,) = axis.plot(
            first.numerical["x"],
            first.numerical[variable],
            color="#d62728",
            linewidth=args.line_width_num,
            marker="o",
            markersize=args.symbol_size,
            markerfacecolor="white",
            markeredgewidth=0.55,
            markevery=max(1, len(first.numerical["x"]) // 90),
            label="Numerical",
        )
        axis.set_ylabel(Y_LABELS[variable])
        axis.set_ylim(*ranges[variable])
        axis.grid(True, color="0.90", linewidth=0.55)
        axis.legend(loc="best", frameon=False, fontsize=9)
        axis.tick_params(direction="in", top=True, right=True)
        exact_lines[variable] = exact_line
        numerical_lines[variable] = numerical_line
    axes[-1].set_xlabel("x")

    x_values = []
    for frame in frames:
        x_values.extend(frame.numerical["x"])
        x_values.extend(frame.exact["x"])
    axes[-1].set_xlim(min(x_values), max(x_values))

    slider_axis = fig.add_axes([0.14, 0.115, 0.74, 0.025])
    beta_slider = Slider(
        ax=slider_axis,
        label="beta index",
        valmin=0,
        valmax=len(frames) - 1,
        valinit=0,
        valstep=1,
    )

    def apply_frame(index: int) -> None:
        frame = frames[index]
        for variable in VARIABLES:
            exact_lines[variable].set_data(frame.exact["x"], frame.exact[variable])
            numerical_lines[variable].set_data(
                frame.numerical["x"],
                frame.numerical[variable],
            )
        time_text = (
            "unknown"
            if frame.solution_time is None
            else f"{frame.solution_time:.8g}"
        )
        title.set_text(
            f"Sod beta sweep: beta={frame.beta:.10g}, "
            f"status={frame.status}, t={time_text}"
        )
        info_text.set_text(format_metrics(frame))
        fig.canvas.draw_idle()

    def on_slider(value: float) -> None:
        apply_frame(int(value))

    beta_slider.on_changed(on_slider)
    apply_frame(0)
    return fig


def main() -> int:
    args = parse_args()
    try:
        if args.save_preview is not None:
            import matplotlib

            matplotlib.use("Agg")

        frames = load_frames(args.sweep_dir, args.include_unstable)
        fig = make_figure(frames, args.padding, args)

        if args.save_preview is not None:
            args.save_preview.parent.mkdir(parents=True, exist_ok=True)
            fig.savefig(args.save_preview, dpi=160)
            print(f"[preview] {args.save_preview}")
        else:
            import matplotlib.pyplot as plt

            plt.show()
        return 0
    except (OSError, ValueError, RuntimeError) as exc:
        print(f"[error] {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
