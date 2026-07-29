#!/usr/bin/env python3
"""Create comparison plots for numerical and exact Riemann data."""

from __future__ import annotations

import argparse
import math
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
from typing import Iterable


VARIABLE_ALIASES = {
    "x": {"x", "coordinatex"},
    "rho": {"rho", "density"},
    "u": {"u", "velocity"},
    "p": {"p", "pressure"},
}

Y_LABELS = {
    "rho": "Density, rho",
    "u": "Velocity, u",
    "p": "Pressure, p",
}


def parse_args() -> argparse.Namespace:
    project_dir = Path(__file__).resolve().parents[1]
    baseline_dir = project_dir / "runs" / "sod_baseline"

    parser = argparse.ArgumentParser(
        description=(
            "Load numerical and exact Tecplot ASCII data, normalize their "
            "variables, and export rho/u/p comparison plots."
        )
    )
    parser.add_argument(
        "--numerical",
        type=Path,
        default=baseline_dir / "numerical.dat",
        help="numerical Tecplot ASCII file",
    )
    parser.add_argument(
        "--exact",
        type=Path,
        default=baseline_dir / "exact.dat",
        help="exact-solution Tecplot ASCII file",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=baseline_dir / "plots",
        help="directory for normalized data and exported images",
    )
    parser.add_argument(
        "--backend",
        choices=("matplotlib", "tecplot-macro", "pytecplot"),
        default="matplotlib",
        help=(
            "rendering backend; tecplot-macro uses Tecplot without TecPLUS, "
            "while pytecplot requires TecPLUS"
        ),
    )
    parser.add_argument(
        "--connect",
        action="store_true",
        help="connect to a running Tecplot GUI instead of using batch engine",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=7600,
        help="TecUtil Server port used with --connect",
    )
    parser.add_argument(
        "--width",
        type=int,
        default=1400,
        help="PNG width in pixels",
    )
    parser.add_argument(
        "--supersample",
        type=int,
        default=2,
        help="Tecplot export supersampling factor",
    )
    parser.add_argument(
        "--padding",
        type=float,
        default=0.05,
        help="fractional y-axis padding",
    )
    parser.add_argument(
        "--keep-layout",
        action="store_true",
        help="leave the final plot visible when connected to Tecplot GUI",
    )
    return parser.parse_args()


def canonical_name(name: str) -> str | None:
    compact = re.sub(r"[^a-z0-9]", "", name.lower())
    for canonical, aliases in VARIABLE_ALIASES.items():
        if compact in aliases:
            return canonical
    return None


def parse_variable_names(line: str) -> list[str]:
    quoted = re.findall(r'"([^"]+)"', line)
    if quoted:
        return quoted

    _, _, raw_names = line.partition("=")
    return [part.strip() for part in raw_names.split(",") if part.strip()]


def read_tecplot_ascii(path: Path) -> dict[str, list[float]]:
    if not path.is_file():
        raise FileNotFoundError(path)

    variable_names: list[str] | None = None
    rows: list[list[float]] = []
    data_started = False

    with path.open("r", encoding="utf-8-sig") as stream:
        for raw_line in stream:
            line = raw_line.strip()
            if not line:
                continue

            upper = line.upper()
            if upper.startswith("VARIABLES"):
                variable_names = parse_variable_names(line)
                continue
            if upper.startswith("ZONE"):
                data_started = True
                continue
            if not data_started or line.startswith(("#", "!")):
                continue

            try:
                row = [float(value) for value in line.replace(",", " ").split()]
            except ValueError as exc:
                raise ValueError(f"Cannot parse data row in {path}: {line}") from exc
            rows.append(row)

    if not variable_names:
        raise ValueError(f"No VARIABLES header found in {path}")
    if not rows:
        raise ValueError(f"No data rows found in {path}")
    if any(len(row) < len(variable_names) for row in rows):
        raise ValueError(f"A data row has fewer columns than VARIABLES in {path}")

    columns: dict[str, list[float]] = {}
    for index, source_name in enumerate(variable_names):
        canonical = canonical_name(source_name)
        if canonical is not None and canonical not in columns:
            columns[canonical] = [row[index] for row in rows]

    missing = [name for name in VARIABLE_ALIASES if name not in columns]
    if missing:
        raise ValueError(f"{path} is missing required variables: {', '.join(missing)}")

    order = sorted(range(len(columns["x"])), key=columns["x"].__getitem__)
    return {
        name: [values[index] for index in order]
        for name, values in columns.items()
    }


def rows_in_domain(
    data: dict[str, list[float]],
    xmin: float,
    xmax: float,
) -> list[tuple[float, float, float, float]]:
    rows = zip(data["x"], data["rho"], data["u"], data["p"])
    selected = [row for row in rows if xmin <= row[0] <= xmax]
    if len(selected) < 2:
        raise ValueError("Exact data has fewer than two points in numerical x-domain")
    return selected


def all_rows(
    data: dict[str, list[float]],
) -> list[tuple[float, float, float, float]]:
    return list(zip(data["x"], data["rho"], data["u"], data["p"]))


def write_combined_dataset(
    path: Path,
    numerical_rows: list[tuple[float, float, float, float]],
    exact_rows: list[tuple[float, float, float, float]],
) -> None:
    with path.open("w", encoding="ascii", newline="\n") as stream:
        stream.write('TITLE = "Riemann numerical vs exact"\n')
        stream.write('VARIABLES = "x", "rho", "u", "p"\n')
        for zone_name, rows in (
            ("Numerical", numerical_rows),
            ("Exact", exact_rows),
        ):
            stream.write(f'ZONE T="{zone_name}", I={len(rows)}, F=POINT\n')
            for row in rows:
                stream.write(" ".join(f"{value:.15e}" for value in row) + "\n")


def padded_range(
    values: Iterable[float],
    padding_fraction: float,
    nonnegative: bool,
) -> tuple[float, float]:
    finite = [value for value in values if math.isfinite(value)]
    if not finite:
        raise ValueError("Cannot scale axis: no finite values")

    lower = min(finite)
    upper = max(finite)
    span = upper - lower
    if span <= 0.0:
        reference = max(abs(lower), 1.0)
        span = 0.1 * reference

    padding = max(0.0, padding_fraction) * span
    scaled_lower = lower - padding
    scaled_upper = upper + padding

    if nonnegative and lower >= 0.0:
        scaled_lower = max(0.0, scaled_lower)
    return scaled_lower, scaled_upper


def locate_pytecplot() -> None:
    try:
        import tecplot  # noqa: F401
        return
    except ModuleNotFoundError:
        pass

    candidates: list[Path] = []
    tec360 = shutil.which("tec360.exe") or shutil.which("tec360")
    if tec360:
        candidates.append(Path(tec360).resolve().parent.parent / "pytecplot")

    candidates.append(
        Path(
            r"C:\WorkStation\_Basement\Tecplot 2024"
            r"\Tecplot 360 EX 2024 R1\pytecplot"
        )
    )

    for candidate in candidates:
        if (candidate / "tecplot").is_dir():
            sys.path.insert(0, os.fspath(candidate))
            return

    raise ModuleNotFoundError(
        "PyTecplot was not found. Add its directory to PYTHONPATH."
    )


def render_plots(
    combined_path: Path,
    output_dir: Path,
    ranges: dict[str, tuple[float, float]],
    xmin: float,
    xmax: float,
    args: argparse.Namespace,
) -> None:
    locate_pytecplot()

    import tecplot as tp
    from tecplot.constant import (
        AxisTitleMode,
        Color,
        FillMode,
        GeomShape,
        PlotType,
    )

    if args.connect:
        tp.session.connect(port=args.port)

    tp.new_layout()
    dataset = tp.data.load_tecplot(os.fspath(combined_path))
    frame = tp.active_frame()
    frame.plot_type = PlotType.XYLine
    frame.show_border = False
    plot = frame.plot()

    numerical_zone = dataset.zone("Numerical")
    exact_zone = dataset.zone("Exact")
    x_variable = dataset.variable("x")

    for variable_name in ("rho", "u", "p"):
        plot.delete_linemaps()
        y_variable = dataset.variable(variable_name)

        exact_map = plot.add_linemap(
            "Exact", exact_zone, x=x_variable, y=y_variable
        )
        exact_map.line.color = Color.Black
        exact_map.line.line_thickness = 0.8
        exact_map.symbols.show = False

        numerical_map = plot.add_linemap(
            "Roe", numerical_zone, x=x_variable, y=y_variable
        )
        numerical_map.line.color = Color.DeepRed
        numerical_map.line.line_thickness = 0.35
        numerical_map.symbols.show = True
        numerical_map.symbols.symbol().shape = GeomShape.Circle
        numerical_map.symbols.size = 1.2
        numerical_map.symbols.color = Color.DeepRed
        numerical_map.symbols.fill_mode = FillMode.UseLineColor

        x_axis = plot.axes.x_axis
        y_axis = plot.axes.y_axis
        x_axis.min = xmin
        x_axis.max = xmax
        y_axis.min, y_axis.max = ranges[variable_name]

        x_axis.title.title_mode = AxisTitleMode.UseText
        x_axis.title.text = "x"
        y_axis.title.title_mode = AxisTitleMode.UseText
        y_axis.title.text = Y_LABELS[variable_name]

        plot.legend.show = True
        plot.view.fit()

        output_path = output_dir / f"{variable_name}.png"
        tp.export.save_png(
            os.fspath(output_path),
            width=args.width,
            supersample=args.supersample,
        )
        print(
            f"[export] {output_path} "
            f"x=[{xmin:.6g}, {xmax:.6g}] "
            f"y=[{y_axis.min:.6g}, {y_axis.max:.6g}]"
        )

    if args.connect and args.keep_layout:
        print("[tecplot] Final plot left open in connected GUI.")


def render_plots_matplotlib(
    numerical_rows: list[tuple[float, float, float, float]],
    exact_rows: list[tuple[float, float, float, float]],
    output_dir: Path,
    ranges: dict[str, tuple[float, float]],
    xmin: float,
    xmax: float,
    args: argparse.Namespace,
) -> None:
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    numerical_x = [row[0] for row in numerical_rows]
    exact_x = [row[0] for row in exact_rows]
    column_indices = {"rho": 1, "u": 2, "p": 3}

    dpi = 140
    figure_width = max(args.width / dpi, 6.0)
    figure_height = figure_width * 0.50

    for variable_name in ("rho", "u", "p"):
        index = column_indices[variable_name]
        numerical_y = [row[index] for row in numerical_rows]
        exact_y = [row[index] for row in exact_rows]

        figure, axis = plt.subplots(
            figsize=(figure_width, figure_height),
            constrained_layout=True,
        )
        axis.plot(
            exact_x,
            exact_y,
            color="black",
            linewidth=0.9,
            label="Exact",
            zorder=2,
        )
        axis.plot(
            numerical_x,
            numerical_y,
            color="#c43c39",
            linewidth=0.8,
            marker="o",
            markersize=1.8,
            markerfacecolor="white",
            markeredgewidth=0.55,
            markevery=max(1, len(numerical_x) // 120),
            label="Roe",
            zorder=3,
        )

        axis.set_xlim(xmin, xmax)
        axis.set_ylim(*ranges[variable_name])
        axis.set_xlabel("x")
        axis.set_ylabel(Y_LABELS[variable_name])
        axis.grid(True, color="#e2e2e2", linewidth=0.5, alpha=0.8)
        axis.legend(frameon=False, loc="best", fontsize=9)
        axis.tick_params(direction="in", top=True, right=True)

        for spine in axis.spines.values():
            spine.set_linewidth(0.8)

        output_path = output_dir / f"{variable_name}.png"
        figure.savefig(output_path, dpi=dpi)
        plt.close(figure)
        print(
            f"[export] {output_path} "
            f"x=[{xmin:.6g}, {xmax:.6g}] "
            f"y=[{ranges[variable_name][0]:.6g}, "
            f"{ranges[variable_name][1]:.6g}]"
        )


def tecplot_path(path: Path) -> str:
    return os.fspath(path.resolve()).replace("/", "\\")


def write_tecplot_macro(
    macro_path: Path,
    combined_path: Path,
    output_dir: Path,
    ranges: dict[str, tuple[float, float]],
    xmin: float,
    xmax: float,
    width: int,
) -> None:
    variable_numbers = {"rho": 2, "u": 3, "p": 4}
    macro_lines = [
        "#!MC 1410",
        "",
        f"$!READDATASET  '\"{tecplot_path(combined_path)}\"'",
        "  INITIALPLOTTYPE = XYLINE",
        "  INCLUDETEXT = NO",
        "  INCLUDEGEOM = NO",
        "",
        "$!PLOTTYPE = XYLINE",
        "$!DELETELINEMAPS",
        "$!CREATELINEMAP",
        "$!LINEMAP [1] NAME = 'Roe'",
        "$!LINEMAP [1] ASSIGN{ZONE = 1}",
        "$!LINEMAP [1] ASSIGN{XAXISVAR = 1}",
        "$!LINEMAP [1] LINES{COLOR = RED}",
        "$!LINEMAP [1] LINES{LINETHICKNESS = 0.28}",
        "$!LINEMAP [1] SYMBOLS{SHOW = YES}",
        "$!LINEMAP [1] SYMBOLS{COLOR = RED}",
        "$!LINEMAP [1] SYMBOLS{SIZE = 0.65}",
        "$!CREATELINEMAP",
        "$!LINEMAP [2] NAME = 'Exact'",
        "$!LINEMAP [2] ASSIGN{ZONE = 2}",
        "$!LINEMAP [2] ASSIGN{XAXISVAR = 1}",
        "$!LINEMAP [2] LINES{COLOR = BLACK}",
        "$!LINEMAP [2] LINES{LINETHICKNESS = 0.35}",
        "$!LINEMAP [2] SYMBOLS{SHOW = NO}",
        "$!ACTIVELINEMAPS = [1-2]",
        "$!GLOBALLINEPLOT LEGEND{SHOW = YES}",
        "$!GLOBALLINEPLOT LEGEND{XYPOS{X = 34 Y = 91}}",
        "$!GLOBALLINEPLOT LEGEND{TEXTSHAPE{SIZEUNITS = POINT}}",
        "$!GLOBALLINEPLOT LEGEND{TEXTSHAPE{HEIGHT = 8}}",
        "$!XYLINEAXIS XDETAIL 1 {TITLE{TITLEMODE = USETEXT}}",
        "$!XYLINEAXIS XDETAIL 1 {TITLE{TEXT = 'x'}}",
        "$!EXPORTSETUP EXPORTFORMAT = PNG",
        f"$!EXPORTSETUP IMAGEWIDTH = {width}",
    ]

    for variable_name in ("rho", "u", "p"):
        y_min, y_max = ranges[variable_name]
        output_path = output_dir / f"tecplot_{variable_name}.png"
        macro_lines.extend(
            [
                "",
                f"$!LINEMAP [1] ASSIGN{{YAXISVAR = {variable_numbers[variable_name]}}}",
                f"$!LINEMAP [2] ASSIGN{{YAXISVAR = {variable_numbers[variable_name]}}}",
                "$!XYLINEAXIS YDETAIL 1 {TITLE{TITLEMODE = USETEXT}}",
                f"$!XYLINEAXIS YDETAIL 1 {{TITLE{{TEXT = '{Y_LABELS[variable_name]}'}}}}",
                "$!VIEW DATAFIT",
                f"$!XYLINEAXIS XDETAIL 1 {{RANGEMIN = {xmin:.15g}}}",
                f"$!XYLINEAXIS XDETAIL 1 {{RANGEMAX = {xmax:.15g}}}",
                f"$!XYLINEAXIS YDETAIL 1 {{RANGEMIN = {y_min:.15g}}}",
                f"$!XYLINEAXIS YDETAIL 1 {{RANGEMAX = {y_max:.15g}}}",
                f'$!EXPORTSETUP EXPORTFNAME = "{tecplot_path(output_path)}"',
                "$!EXPORTSTART",
                "$!EXPORTFINISH",
            ]
        )

    macro_path.write_text("\n".join(macro_lines) + "\n", encoding="ascii")


def render_plots_tecplot_macro(
    combined_path: Path,
    output_dir: Path,
    ranges: dict[str, tuple[float, float]],
    xmin: float,
    xmax: float,
    args: argparse.Namespace,
) -> None:
    tec360 = shutil.which("tec360.exe") or shutil.which("tec360")
    if not tec360:
        raise FileNotFoundError("tec360.exe was not found in PATH")

    macro_path = output_dir / "export_with_tecplot.mcr"
    write_tecplot_macro(
        macro_path,
        combined_path,
        output_dir,
        ranges,
        xmin,
        xmax,
        args.width,
    )
    print(f"[tecplot] macro: {macro_path.resolve()}")

    subprocess.run(
        [tec360, "-b", os.fspath(macro_path.resolve())],
        check=True,
    )

    for variable_name in ("rho", "u", "p"):
        candidates = sorted(output_dir.glob(f"tecplot_{variable_name}*.png"))
        if not candidates:
            raise RuntimeError(f"Tecplot did not export {variable_name}.png")
        print(f"[export] {candidates[-1]}")


def main() -> int:
    args = parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)

    numerical = read_tecplot_ascii(args.numerical.resolve())
    exact = read_tecplot_ascii(args.exact.resolve())

    xmin = min(numerical["x"])
    xmax = max(numerical["x"])
    numerical_rows = all_rows(numerical)
    exact_rows = rows_in_domain(exact, xmin, xmax)

    combined_path = args.output_dir / "numerical_exact_combined.dat"
    write_combined_dataset(combined_path, numerical_rows, exact_rows)

    ranges: dict[str, tuple[float, float]] = {}
    for variable_name in ("rho", "u", "p"):
        column_index = {"rho": 1, "u": 2, "p": 3}[variable_name]
        values = [row[column_index] for row in numerical_rows]
        values.extend(row[column_index] for row in exact_rows)
        ranges[variable_name] = padded_range(
            values,
            padding_fraction=args.padding,
            nonnegative=variable_name in {"rho", "p"},
        )

    print(f"[data] numerical: {args.numerical.resolve()}")
    print(f"[data] exact:     {args.exact.resolve()}")
    print(f"[data] combined:  {combined_path.resolve()}")
    print(f"[scale] x=[{xmin:.6g}, {xmax:.6g}] from numerical domain")

    if args.backend == "matplotlib":
        render_plots_matplotlib(
            numerical_rows,
            exact_rows,
            args.output_dir,
            ranges,
            xmin,
            xmax,
            args,
        )
    elif args.backend == "tecplot-macro":
        try:
            render_plots_tecplot_macro(
                combined_path,
                args.output_dir,
                ranges,
                xmin,
                xmax,
                args,
            )
        except Exception as exc:
            print(f"[error] Tecplot macro rendering failed: {exc}", file=sys.stderr)
            return 1
    else:
        try:
            render_plots(combined_path, args.output_dir, ranges, xmin, xmax, args)
        except Exception as exc:
            print(f"[error] PyTecplot rendering failed: {exc}", file=sys.stderr)
            print(
                "[hint] TecUtil Server and PyTecplot require a valid TecPLUS "
                "service. Use the default matplotlib backend to bypass it.",
                file=sys.stderr,
            )
            return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
