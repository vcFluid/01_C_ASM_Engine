#!/usr/bin/env python3
"""Run and post-process an artificial-viscosity beta parameter sweep."""

from __future__ import annotations

import argparse
from bisect import bisect_left
import csv
import json
import math
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
from typing import Iterable

from tecplot_compare import (
    Y_LABELS,
    all_rows,
    padded_range,
    read_tecplot_ascii,
    tecplot_path,
)


VARIABLES = ("rho", "u", "p")
CASE_FINAL_TIMES = {
    1: 0.20,
    2: 0.16,
    3: 0.15,
    4: 0.15,
    5: 0.20,
    6: 0.20,
    7: 0.20,
}
CASE_NAMES = {
    1: "Sod",
    2: "LAX",
    3: "Subsonic_Double_Expansion",
    4: "Sjogreen_Supersonic_Expansion",
    5: "Contact_Double_Expansion",
    6: "Contact_Double_Shock",
    7: "Pure_Contact",
}
LINE_COLORS = ("RED", "BLUE", "GREEN", "PURPLE", "CYAN", "YELLOW")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Compile Project 2, run a fixed-condition beta sweep, compute "
            "errors against Project 0, and export Tecplot comparison figures."
        )
    )
    parser.add_argument(
        "--betas",
        type=float,
        nargs="+",
        default=(0.0, 0.05, 0.10, 0.20, 0.25, 0.50),
        help="artificial-viscosity beta values",
    )
    parser.add_argument("--case", type=int, choices=range(1, 8), default=1)
    parser.add_argument("--xmin", type=float, default=0.0)
    parser.add_argument("--xmax", type=float, default=1.0)
    parser.add_argument("--x0", type=float, default=0.5)
    parser.add_argument("--nx", type=int, default=501)
    parser.add_argument("--gamma", type=float, default=1.4)
    parser.add_argument("--cfl", type=float, default=0.5)
    parser.add_argument(
        "--t-max",
        type=float,
        default=None,
        help="final time; defaults to the selected built-in case value",
    )
    parser.add_argument(
        "--sensor",
        choices=("rho", "u", "p"),
        default="rho",
        help="artificial-viscosity switch variable",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=None,
        help="defaults to runs/Solution_XX_CaseName/beta_sweep",
    )
    parser.add_argument(
        "--backend",
        choices=("tecplot-macro", "none"),
        default="tecplot-macro",
    )
    parser.add_argument("--width", type=int, default=1400)
    parser.add_argument("--padding", type=float, default=0.05)
    parser.add_argument(
        "--keep-build",
        action="store_true",
        help="keep the temporary sweep solver executable",
    )
    return parser.parse_args()


def unique_sorted_nonnegative(values: Iterable[float]) -> list[float]:
    result = sorted(set(values))
    if not result:
        raise ValueError("At least one beta value is required")
    if any(not math.isfinite(value) or value < 0.0 for value in result):
        raise ValueError("Beta values must be finite and nonnegative")
    return result


def beta_text(beta: float) -> str:
    return f"{beta:.10g}"


def beta_directory_name(beta: float) -> str:
    token = beta_text(beta).replace("-", "m").replace(".", "p")
    return f"beta_{token}"


def solution_directory(project_dir: Path, case_id: int) -> Path:
    case_name = CASE_NAMES[case_id]
    return project_dir / "runs" / f"Solution_{case_id:02d}_{case_name}"


def compile_program(
    source: Path,
    output: Path,
    cwd: Path,
) -> None:
    command = [
        "gcc",
        os.fspath(source),
        "-std=c11",
        "-O2",
        "-Wall",
        "-Wextra",
        "-o",
        os.fspath(output),
        "-lm",
    ]
    print(f"[build] {source.name} -> {output}")
    subprocess.run(command, cwd=cwd, check=True)


def build_solver_input(
    args: argparse.Namespace,
    beta: float,
    output_path: Path,
) -> str:
    sensor_index = {"rho": 1, "u": 2, "p": 3}[args.sensor]
    t_max = args.t_max
    lines = [
        str(args.case),
        f"{args.xmin:.17g}",
        f"{args.xmax:.17g}",
        f"{args.x0:.17g}",
        str(args.nx),
        f"{args.gamma:.17g}",
        f"{args.cfl:.17g}",
        f"{t_max:.17g}",
        "y",
        f"{beta:.17g}",
        str(sensor_index),
        "0",
        os.fspath(output_path),
    ]
    return "\n".join(lines) + "\n"


def run_case(
    executable: Path,
    project_dir: Path,
    case_dir: Path,
    args: argparse.Namespace,
    beta: float,
) -> tuple[Path, Path]:
    case_dir.mkdir(parents=True, exist_ok=True)
    numerical_path = case_dir / "numerical.dat"
    exact_path = case_dir / "numerical_exact.dat"
    try:
        solver_numerical_path = numerical_path.relative_to(project_dir)
    except ValueError:
        solver_numerical_path = numerical_path
    input_text = build_solver_input(args, beta, solver_numerical_path)

    print(f"[run] beta={beta_text(beta)}")
    result = subprocess.run(
        [os.fspath(executable.resolve())],
        cwd=project_dir,
        input=input_text,
        text=True,
        encoding="utf-8",
        errors="replace",
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    (case_dir / "run.log").write_text(result.stdout, encoding="utf-8")
    if result.returncode != 0:
        raise RuntimeError(
            f"Solver failed for beta={beta_text(beta)}; see "
            f"{case_dir / 'run.log'}"
        )
    if not numerical_path.is_file() or not exact_path.is_file():
        raise RuntimeError(
            f"Missing numerical or exact output for beta={beta_text(beta)}"
        )

    config = {
        "case": args.case,
        "beta": beta,
        "sensor": args.sensor,
        "xmin": args.xmin,
        "xmax": args.xmax,
        "x0": args.x0,
        "nx": args.nx,
        "gamma": args.gamma,
        "cfl": args.cfl,
        "t_max": args.t_max,
    }
    (case_dir / "config.json").write_text(
        json.dumps(config, indent=2) + "\n",
        encoding="ascii",
    )
    return numerical_path, exact_path


def interpolate_to_grid(
    source_x: list[float],
    source_values: list[float],
    target_x: list[float],
) -> list[float]:
    if len(source_x) != len(source_values) or len(source_x) < 2:
        raise ValueError("Invalid interpolation source data")

    tolerance = 1.0e-12 * max(
        1.0,
        abs(source_x[0]),
        abs(source_x[-1]),
    )
    result: list[float] = []
    for x_value in target_x:
        index = bisect_left(source_x, x_value)
        if index < len(source_x) and abs(source_x[index] - x_value) <= tolerance:
            result.append(source_values[index])
            continue
        if index > 0 and abs(source_x[index - 1] - x_value) <= tolerance:
            result.append(source_values[index - 1])
            continue
        if index == 0 or index == len(source_x):
            raise ValueError(f"Target x={x_value} lies outside exact-solution domain")

        x_left = source_x[index - 1]
        x_right = source_x[index]
        fraction = (x_value - x_left) / (x_right - x_left)
        result.append(
            source_values[index - 1]
            + fraction * (source_values[index] - source_values[index - 1])
        )
    return result


def trapezoidal_integral(x_values: list[float], values: list[float]) -> float:
    return sum(
        0.5
        * (values[index] + values[index + 1])
        * (x_values[index + 1] - x_values[index])
        for index in range(len(x_values) - 1)
    )


def compute_metrics(
    numerical: dict[str, list[float]],
    exact: dict[str, list[float]],
) -> list[dict[str, float | str]]:
    x_values = numerical["x"]
    if len(x_values) < 2:
        raise ValueError("Numerical data requires at least two points")

    metrics: list[dict[str, float | str]] = []
    for variable in VARIABLES:
        exact_values = interpolate_to_grid(
            exact["x"],
            exact[variable],
            x_values,
        )
        numerical_values = numerical[variable]
        errors = [
            numerical_value - exact_value
            for numerical_value, exact_value in zip(
                numerical_values,
                exact_values,
            )
        ]
        absolute_errors = [abs(value) for value in errors]
        squared_errors = [value * value for value in errors]
        total_variation = sum(
            abs(numerical_values[index + 1] - numerical_values[index])
            for index in range(len(numerical_values) - 1)
        )
        metrics.append(
            {
                "variable": variable,
                "L1": trapezoidal_integral(x_values, absolute_errors),
                "L2": math.sqrt(
                    trapezoidal_integral(x_values, squared_errors)
                ),
                "Linf": max(absolute_errors),
                "TV": total_variation,
                "undershoot": max(
                    0.0,
                    min(exact_values) - min(numerical_values),
                ),
                "overshoot": max(
                    0.0,
                    max(numerical_values) - max(exact_values),
                ),
            }
        )
    return metrics


def read_solution_time(path: Path) -> float:
    zone_pattern = re.compile(
        r'ZONE\s+T\s*=\s*"t=([+\-0-9.eE]+)"',
        re.IGNORECASE,
    )
    with path.open("r", encoding="utf-8-sig") as stream:
        for line in stream:
            match = zone_pattern.search(line)
            if match:
                return float(match.group(1))
    raise ValueError(f"No solution time found in {path}")


def validate_numerical_solution(
    data: dict[str, list[float]],
    path: Path,
    requested_time: float,
) -> tuple[bool, str, float]:
    solution_time = read_solution_time(path)
    if not math.isclose(
        solution_time,
        requested_time,
        rel_tol=1.0e-7,
        abs_tol=1.0e-9,
    ):
        return (
            False,
            f"stopped at t={solution_time:.8g}, requested t={requested_time:.8g}",
            solution_time,
        )

    for variable in ("x", *VARIABLES):
        if any(not math.isfinite(value) for value in data[variable]):
            return False, f"non-finite {variable} values", solution_time
    return True, "completed", solution_time


def write_errors_csv(
    path: Path,
    results: list[dict[str, float | str | None]],
) -> None:
    fieldnames = (
        "beta",
        "status",
        "reason",
        "solution_time",
        "variable",
        "L1",
        "L2",
        "Linf",
        "TV",
        "undershoot",
        "overshoot",
    )
    with path.open("w", encoding="ascii", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(results)


def write_profile_dataset(
    path: Path,
    exact: dict[str, list[float]],
    profiles: list[tuple[float, dict[str, list[float]]]],
) -> None:
    with path.open("w", encoding="ascii", newline="\n") as stream:
        stream.write('TITLE = "Artificial-viscosity beta sweep"\n')
        stream.write('VARIABLES = "x", "rho", "u", "p"\n')

        exact_rows = all_rows(exact)
        stream.write(f'ZONE T="Exact", I={len(exact_rows)}, F=POINT\n')
        for row in exact_rows:
            stream.write(" ".join(f"{value:.15e}" for value in row) + "\n")

        for beta, numerical in profiles:
            rows = all_rows(numerical)
            stream.write(
                f'ZONE T="beta={beta_text(beta)}", '
                f'AUXDATA beta="{beta_text(beta)}", '
                f'I={len(rows)}, F=POINT\n'
            )
            for row in rows:
                stream.write(" ".join(f"{value:.15e}" for value in row) + "\n")


def write_error_dataset(
    path: Path,
    betas: list[float],
    results: list[dict[str, float | str]],
) -> None:
    by_key = {
        (float(row["beta"]), str(row["variable"])): row
        for row in results
    }
    metric_names = ("L1", "L2", "Linf", "TV", "undershoot", "overshoot")

    with path.open("w", encoding="ascii", newline="\n") as stream:
        stream.write('TITLE = "Artificial-viscosity beta sweep metrics"\n')
        stream.write(
            'VARIABLES = "beta", "L1", "L2", "Linf", '
            '"TV", "undershoot", "overshoot"\n'
        )
        for variable in VARIABLES:
            stream.write(
                f'ZONE T="{variable}", I={len(betas)}, F=POINT\n'
            )
            for beta in betas:
                row = by_key[(beta, variable)]
                values = [beta]
                values.extend(float(row[name]) for name in metric_names)
                stream.write(
                    " ".join(f"{value:.15e}" for value in values) + "\n"
                )


def global_profile_ranges(
    exact: dict[str, list[float]],
    profiles: list[tuple[float, dict[str, list[float]]]],
    padding: float,
) -> dict[str, tuple[float, float]]:
    ranges: dict[str, tuple[float, float]] = {}
    for variable in VARIABLES:
        values = list(exact[variable])
        for _, numerical in profiles:
            values.extend(numerical[variable])
        ranges[variable] = padded_range(
            values,
            padding_fraction=padding,
            nonnegative=variable in {"rho", "p"},
        )
    return ranges


def metric_ranges(
    results: list[dict[str, float | str]],
    padding: float,
) -> dict[str, tuple[float, float]]:
    ranges: dict[str, tuple[float, float]] = {}
    for metric in ("L1", "L2", "Linf"):
        ranges[metric] = padded_range(
            [float(row[metric]) for row in results],
            padding_fraction=padding,
            nonnegative=True,
        )
    return ranges


def write_tecplot_macro(
    macro_path: Path,
    profile_path: Path,
    error_path: Path,
    output_dir: Path,
    betas: list[float],
    profile_ranges: dict[str, tuple[float, float]],
    error_ranges: dict[str, tuple[float, float]],
    xmin: float,
    xmax: float,
    width: int,
) -> None:
    profile_variable_numbers = {"rho": 2, "u": 3, "p": 4}
    metric_variable_numbers = {"L1": 2, "L2": 3, "Linf": 4}
    macro_lines = [
        "#!MC 1410",
        "",
        f"$!READDATASET  '\"{tecplot_path(profile_path)}\"'",
        "  INITIALPLOTTYPE = XYLINE",
        "  INCLUDETEXT = NO",
        "  INCLUDEGEOM = NO",
        "$!PLOTTYPE = XYLINE",
        "$!DELETELINEMAPS",
        "$!CREATELINEMAP",
        "$!LINEMAP [1] NAME = 'Exact'",
        "$!LINEMAP [1] ASSIGN{ZONE = 1}",
        "$!LINEMAP [1] ASSIGN{XAXISVAR = 1}",
        "$!LINEMAP [1] LINES{COLOR = BLACK}",
        "$!LINEMAP [1] LINES{LINETHICKNESS = 0.35}",
        "$!LINEMAP [1] SYMBOLS{SHOW = NO}",
    ]

    for index, beta in enumerate(betas, start=2):
        color = LINE_COLORS[(index - 2) % len(LINE_COLORS)]
        macro_lines.extend(
            [
                "$!CREATELINEMAP",
                f"$!LINEMAP [{index}] NAME = 'beta={beta_text(beta)}'",
                f"$!LINEMAP [{index}] ASSIGN{{ZONE = {index}}}",
                f"$!LINEMAP [{index}] ASSIGN{{XAXISVAR = 1}}",
                f"$!LINEMAP [{index}] LINES{{COLOR = {color}}}",
                f"$!LINEMAP [{index}] LINES{{LINETHICKNESS = 0.28}}",
                f"$!LINEMAP [{index}] SYMBOLS{{SHOW = YES}}",
                f"$!LINEMAP [{index}] SYMBOLS{{COLOR = {color}}}",
                f"$!LINEMAP [{index}] SYMBOLS{{SIZE = 0.55}}",
            ]
        )

    last_map = len(betas) + 1
    macro_lines.extend(
        [
            f"$!ACTIVELINEMAPS = [1-{last_map}]",
            "$!GLOBALLINEPLOT LEGEND{SHOW = YES}",
            "$!GLOBALLINEPLOT LEGEND{TEXTSHAPE{SIZEUNITS = POINT}}",
            "$!GLOBALLINEPLOT LEGEND{TEXTSHAPE{HEIGHT = 8}}",
            "$!XYLINEAXIS XDETAIL 1 {TITLE{TITLEMODE = USETEXT}}",
            "$!XYLINEAXIS XDETAIL 1 {TITLE{TEXT = 'x'}}",
            "$!EXPORTSETUP EXPORTFORMAT = PNG",
            f"$!EXPORTSETUP IMAGEWIDTH = {width}",
        ]
    )

    for variable in VARIABLES:
        y_min, y_max = profile_ranges[variable]
        variable_number = profile_variable_numbers[variable]
        output_path = output_dir / f"tecplot_beta_{variable}.png"
        for map_index in range(1, last_map + 1):
            macro_lines.append(
                f"$!LINEMAP [{map_index}] "
                f"ASSIGN{{YAXISVAR = {variable_number}}}"
            )
        macro_lines.extend(
            [
                "$!XYLINEAXIS YDETAIL 1 {TITLE{TITLEMODE = USETEXT}}",
                f"$!XYLINEAXIS YDETAIL 1 "
                f"{{TITLE{{TEXT = '{Y_LABELS[variable]}'}}}}",
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

    macro_lines.extend(
        [
            "",
            "$!NEWLAYOUT",
            f"$!READDATASET  '\"{tecplot_path(error_path)}\"'",
            "  INITIALPLOTTYPE = XYLINE",
            "  INCLUDETEXT = NO",
            "  INCLUDEGEOM = NO",
            "$!PLOTTYPE = XYLINE",
            "$!DELETELINEMAPS",
        ]
    )

    for index, variable in enumerate(VARIABLES, start=1):
        color = LINE_COLORS[index - 1]
        macro_lines.extend(
            [
                "$!CREATELINEMAP",
                f"$!LINEMAP [{index}] NAME = '{variable}'",
                f"$!LINEMAP [{index}] ASSIGN{{ZONE = {index}}}",
                f"$!LINEMAP [{index}] ASSIGN{{XAXISVAR = 1}}",
                f"$!LINEMAP [{index}] LINES{{COLOR = {color}}}",
                f"$!LINEMAP [{index}] LINES{{LINETHICKNESS = 0.45}}",
                f"$!LINEMAP [{index}] SYMBOLS{{SHOW = YES}}",
                f"$!LINEMAP [{index}] SYMBOLS{{COLOR = {color}}}",
                f"$!LINEMAP [{index}] SYMBOLS{{SIZE = 0.9}}",
            ]
        )

    beta_min = min(betas)
    beta_max = max(betas)
    if beta_max <= beta_min:
        beta_padding = max(0.05, 0.1 * max(1.0, abs(beta_min)))
        beta_min -= beta_padding
        beta_max += beta_padding

    macro_lines.extend(
        [
            "$!ACTIVELINEMAPS = [1-3]",
            "$!GLOBALLINEPLOT LEGEND{SHOW = YES}",
            "$!GLOBALLINEPLOT LEGEND{TEXTSHAPE{SIZEUNITS = POINT}}",
            "$!GLOBALLINEPLOT LEGEND{TEXTSHAPE{HEIGHT = 8}}",
            "$!XYLINEAXIS XDETAIL 1 {TITLE{TITLEMODE = USETEXT}}",
            "$!XYLINEAXIS XDETAIL 1 {TITLE{TEXT = 'Artificial viscosity beta'}}",
        ]
    )
    for metric in ("L1", "L2", "Linf"):
        y_min, y_max = error_ranges[metric]
        variable_number = metric_variable_numbers[metric]
        output_path = output_dir / f"tecplot_beta_{metric}.png"
        for map_index in range(1, 4):
            macro_lines.append(
                f"$!LINEMAP [{map_index}] "
                f"ASSIGN{{YAXISVAR = {variable_number}}}"
            )
        macro_lines.extend(
            [
                "$!XYLINEAXIS YDETAIL 1 {TITLE{TITLEMODE = USETEXT}}",
                f"$!XYLINEAXIS YDETAIL 1 "
                f"{{TITLE{{TEXT = '{metric} error'}}}}",
                "$!VIEW DATAFIT",
                f"$!XYLINEAXIS XDETAIL 1 {{RANGEMIN = {beta_min:.15g}}}",
                f"$!XYLINEAXIS XDETAIL 1 {{RANGEMAX = {beta_max:.15g}}}",
                f"$!XYLINEAXIS YDETAIL 1 {{RANGEMIN = {y_min:.15g}}}",
                f"$!XYLINEAXIS YDETAIL 1 {{RANGEMAX = {y_max:.15g}}}",
                f'$!EXPORTSETUP EXPORTFNAME = "{tecplot_path(output_path)}"',
                "$!EXPORTSTART",
                "$!EXPORTFINISH",
            ]
        )

    macro_path.write_text("\n".join(macro_lines) + "\n", encoding="ascii")


def render_tecplot(macro_path: Path, output_dir: Path) -> None:
    tec360 = shutil.which("tec360.exe") or shutil.which("tec360")
    if not tec360:
        raise FileNotFoundError("tec360.exe was not found in PATH")

    print(f"[tecplot] macro: {macro_path}")
    subprocess.run(
        [tec360, "-b", os.fspath(macro_path.resolve())],
        cwd=output_dir,
        check=True,
    )

    output_names = (*VARIABLES, "L1", "L2", "Linf")
    exported: list[Path] = []
    missing: list[str] = []
    for name in output_names:
        candidates = sorted(output_dir.glob(f"tecplot_beta_{name}*.png"))
        if candidates:
            exported.append(candidates[-1])
        else:
            missing.append(name)
    if missing:
        raise RuntimeError(
            "Tecplot did not export: " + ", ".join(missing)
        )
    for path in exported:
        print(f"[export] {path}")


def print_error_summary(
    betas: list[float],
    results: list[dict[str, float | str | None]],
) -> None:
    by_key = {
        (float(row["beta"]), str(row["variable"])): row
        for row in results
        if row["status"] == "completed"
    }
    status_by_beta = {
        float(row["beta"]): (str(row["status"]), str(row["reason"]))
        for row in results
    }
    print("\n[summary] L1 error")
    print("beta        rho           u             p")
    for beta in betas:
        status, reason = status_by_beta[beta]
        if status != "completed":
            print(f"{beta:<10.5g} {status}: {reason}")
            continue
        values = [
            float(by_key[(beta, variable)]["L1"])
            for variable in VARIABLES
        ]
        print(
            f"{beta:<10.5g} "
            + "  ".join(f"{value:<12.5e}" for value in values)
        )


def main() -> int:
    args = parse_args()
    project_dir = Path(__file__).resolve().parents[1]
    if args.output_dir is None:
        args.output_dir = solution_directory(project_dir, args.case) / "beta_sweep"
    args.output_dir = args.output_dir.resolve()
    args.t_max = (
        CASE_FINAL_TIMES[args.case]
        if args.t_max is None
        else args.t_max
    )

    try:
        betas = unique_sorted_nonnegative(args.betas)
        if args.xmax <= args.xmin:
            raise ValueError("xmax must be greater than xmin")
        if not args.xmin <= args.x0 <= args.xmax:
            raise ValueError("x0 must lie inside the computational domain")
        if args.nx < 5:
            raise ValueError("nx must be at least 5")
        if args.gamma <= 1.0 or args.cfl <= 0.0 or args.t_max < 0.0:
            raise ValueError("Require gamma > 1, CFL > 0, and t_max >= 0")

        args.output_dir.mkdir(parents=True, exist_ok=True)
        build_dir = args.output_dir / "_build"
        build_dir.mkdir(exist_ok=True)
        solver_executable = build_dir / "riemann_macc_sweep.exe"
        exact_executable = (
            project_dir / "_Analysical_Solution_Solver" / "riemann_exact.exe"
        )

        compile_program(
            project_dir / "_Analysical_Solution_Solver" / "1-D_Riemann_AM.c",
            exact_executable,
            project_dir,
        )
        compile_program(
            project_dir / "1-D_Riemann_NM_MacC.c",
            solver_executable,
            project_dir,
        )

        profiles: list[tuple[float, dict[str, list[float]]]] = []
        stable_betas: list[float] = []
        all_results: list[dict[str, float | str | None]] = []
        stable_metrics: list[dict[str, float | str]] = []
        reference_exact: dict[str, list[float]] | None = None

        for beta in betas:
            case_dir = args.output_dir / beta_directory_name(beta)
            numerical_path, exact_path = run_case(
                solver_executable,
                project_dir,
                case_dir,
                args,
                beta,
            )
            numerical = read_tecplot_ascii(numerical_path)
            exact = read_tecplot_ascii(exact_path)
            is_valid, reason, solution_time = validate_numerical_solution(
                numerical,
                numerical_path,
                args.t_max,
            )
            if not is_valid:
                print(
                    f"[unstable] beta={beta_text(beta)}: {reason}; "
                    "retaining raw data and run.log"
                )
                for variable in VARIABLES:
                    all_results.append(
                        {
                            "beta": beta,
                            "status": "unstable",
                            "reason": reason,
                            "solution_time": solution_time,
                            "variable": variable,
                            "L1": None,
                            "L2": None,
                            "Linf": None,
                            "TV": None,
                            "undershoot": None,
                            "overshoot": None,
                        }
                    )
                continue

            profiles.append((beta, numerical))
            stable_betas.append(beta)
            if reference_exact is None:
                reference_exact = exact

            for row in compute_metrics(numerical, exact):
                stable_row = {"beta": beta, **row}
                stable_metrics.append(stable_row)
                all_results.append(
                    {
                        "beta": beta,
                        "status": "completed",
                        "reason": reason,
                        "solution_time": solution_time,
                        **row,
                    }
                )

        if reference_exact is None:
            raise RuntimeError("All beta cases were unstable; no plots can be generated")
        errors_path = args.output_dir / "errors.csv"
        profile_path = args.output_dir / "beta_profiles.dat"
        error_path = args.output_dir / "beta_errors.dat"
        write_errors_csv(errors_path, all_results)
        write_profile_dataset(profile_path, reference_exact, profiles)
        write_error_dataset(error_path, stable_betas, stable_metrics)

        sweep_config = {
            "case": args.case,
            "betas": betas,
            "completed_betas": stable_betas,
            "unstable_betas": [
                beta for beta in betas if beta not in stable_betas
            ],
            "sensor": args.sensor,
            "xmin": args.xmin,
            "xmax": args.xmax,
            "x0": args.x0,
            "nx": args.nx,
            "gamma": args.gamma,
            "cfl": args.cfl,
            "t_max": args.t_max,
            "error_definition": {
                "L1": "trapezoidal integral of absolute pointwise error",
                "L2": "sqrt(trapezoidal integral of squared pointwise error)",
                "Linf": "maximum absolute pointwise error",
            },
        }
        (args.output_dir / "sweep_config.json").write_text(
            json.dumps(sweep_config, indent=2) + "\n",
            encoding="ascii",
        )

        if args.backend == "tecplot-macro":
            profile_ranges = global_profile_ranges(
                reference_exact,
                profiles,
                args.padding,
            )
            error_ranges = metric_ranges(stable_metrics, args.padding)
            macro_path = args.output_dir / "export_beta_sweep_with_tecplot.mcr"
            write_tecplot_macro(
                macro_path,
                profile_path,
                error_path,
                args.output_dir,
                stable_betas,
                profile_ranges,
                error_ranges,
                args.xmin,
                args.xmax,
                args.width,
            )
            render_tecplot(macro_path, args.output_dir)

        if not args.keep_build:
            shutil.rmtree(build_dir)

        print_error_summary(betas, all_results)
        print(f"\n[data] {errors_path}")
        print(f"[done] beta sweep output: {args.output_dir}")
        return 0
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as exc:
        print(f"[error] {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
