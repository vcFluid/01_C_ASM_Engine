#!/usr/bin/env python3
"""Run a parameter-matrix study and summarize numerical errors.

This script is intentionally a thin automation layer around the existing
teaching solver.  The C program remains interactive; the matrix driver feeds
the same answers through stdin, then reads the resulting Tecplot ASCII files.
"""

from __future__ import annotations

import argparse
import csv
from dataclasses import dataclass
import itertools
import json
import math
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

from beta_sweep import (
    CASE_FINAL_TIMES,
    VARIABLES,
    beta_text,
    compile_program,
    compute_metrics,
    read_tecplot_ascii,
    solution_directory,
    tecplot_path,
    validate_numerical_solution,
)
from tecplot_compare import Y_LABELS, padded_range


LINE_COLORS = ("RED", "BLUE", "GREEN", "PURPLE", "CYAN", "YELLOW", "BLACK")
METRICS = ("L1", "L2", "Linf")


@dataclass(frozen=True)
class MatrixCase:
    case_id: int
    xmin: float
    xmax: float
    x0: float
    nx: int
    gamma: float
    cfl: float
    t_max: float
    use_viscosity: bool
    beta: float
    sensor: str
    mode: int

    @property
    def viscosity_label(self) -> str:
        return "on" if self.use_viscosity else "off"

    @property
    def case_name(self) -> str:
        beta_token = beta_text(self.beta).replace("-", "m").replace(".", "p")
        return (
            f"visc_{self.viscosity_label}"
            f"_sensor_{self.sensor}"
            f"_beta_{beta_token}"
            f"_cfl_{str(self.cfl).replace('.', 'p')}"
            f"_nx_{self.nx}"
            f"_mode_{self.mode}"
        )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Run a Sod-first targeted matrix: artificial viscosity on/off, "
            "beta, sensor variable, CFL, and grid resolution.  The same driver "
            "can later be reused for the other built-in Riemann cases."
        )
    )
    parser.add_argument("--case", type=int, choices=range(1, 8), default=1)
    parser.add_argument("--xmin", type=float, default=0.0)
    parser.add_argument("--xmax", type=float, default=1.0)
    parser.add_argument("--x0", type=float, default=0.5)
    parser.add_argument("--gamma", type=float, default=1.4)
    parser.add_argument(
        "--t-max",
        type=float,
        default=None,
        help="final time; defaults to the selected built-in case value",
    )
    parser.add_argument(
        "--betas",
        type=float,
        nargs="+",
        default=(0.05, 0.10, 0.20, 0.25, 0.50),
        help="positive beta values used when artificial viscosity is on",
    )
    parser.add_argument(
        "--sensors",
        choices=("rho", "u", "p"),
        nargs="+",
        default=("rho", "u", "p"),
    )
    parser.add_argument(
        "--cfls",
        type=float,
        nargs="+",
        default=(0.2, 0.5, 0.8),
    )
    parser.add_argument(
        "--nxs",
        type=int,
        nargs="+",
        default=(101, 201, 501, 1001),
    )
    parser.add_argument(
        "--skip-viscosity-off",
        action="store_true",
        help="omit explicit artificial-viscosity-off baseline cases",
    )
    parser.add_argument(
        "--mode",
        type=int,
        choices=range(1, 5),
        default=1,
        help="MacCormack mode used by 1-D_Riemann_NM_MacC_02.c",
    )
    parser.add_argument(
        "--reference-beta",
        type=float,
        default=0.25,
        help="beta used for CFL and grid one-axis summaries",
    )
    parser.add_argument(
        "--reference-sensor",
        choices=("rho", "u", "p"),
        default="rho",
    )
    parser.add_argument("--reference-cfl", type=float, default=0.5)
    parser.add_argument("--reference-nx", type=int, default=501)
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=None,
        help="defaults to runs/Solution_XX_CaseName/targeted_matrix",
    )
    parser.add_argument(
        "--backend",
        choices=("tecplot-macro", "none"),
        default="tecplot-macro",
    )
    parser.add_argument("--width", type=int, default=1400)
    parser.add_argument("--padding", type=float, default=0.05)
    parser.add_argument("--keep-build", action="store_true")
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="print the matrix size and write no solver output",
    )
    return parser.parse_args()


def unique_sorted(values: list[float] | tuple[float, ...]) -> list[float]:
    result = sorted(set(float(value) for value in values))
    if not result or any(not math.isfinite(value) for value in result):
        raise ValueError("Parameter lists must contain finite values")
    return result


def unique_sorted_int(values: list[int] | tuple[int, ...]) -> list[int]:
    result = sorted(set(int(value) for value in values))
    if not result:
        raise ValueError("At least one grid size is required")
    return result


def validate_args(args: argparse.Namespace) -> None:
    if args.xmax <= args.xmin:
        raise ValueError("xmax must be greater than xmin")
    if not args.xmin <= args.x0 <= args.xmax:
        raise ValueError("x0 must lie inside the computational domain")
    if args.gamma <= 1.0:
        raise ValueError("gamma must be greater than 1")
    if args.t_max < 0.0:
        raise ValueError("t_max must be nonnegative")
    if any(beta <= 0.0 for beta in args.betas):
        raise ValueError(
            "targeted_matrix.py treats beta=0 as viscosity off; "
            "use positive beta values for --betas"
        )
    if any(cfl <= 0.0 for cfl in args.cfls):
        raise ValueError("CFL values must be positive")
    if any(nx < 5 for nx in args.nxs):
        raise ValueError("nx must be at least 5")


def build_matrix(args: argparse.Namespace) -> list[MatrixCase]:
    cases: list[MatrixCase] = []
    for beta, sensor, cfl, nx in itertools.product(
        args.betas,
        args.sensors,
        args.cfls,
        args.nxs,
    ):
        cases.append(
            MatrixCase(
                case_id=args.case,
                xmin=args.xmin,
                xmax=args.xmax,
                x0=args.x0,
                nx=nx,
                gamma=args.gamma,
                cfl=cfl,
                t_max=args.t_max,
                use_viscosity=True,
                beta=beta,
                sensor=sensor,
                mode=args.mode,
            )
        )

    if not args.skip_viscosity_off:
        for cfl, nx in itertools.product(args.cfls, args.nxs):
            cases.append(
                MatrixCase(
                    case_id=args.case,
                    xmin=args.xmin,
                    xmax=args.xmax,
                    x0=args.x0,
                    nx=nx,
                    gamma=args.gamma,
                    cfl=cfl,
                    t_max=args.t_max,
                    use_viscosity=False,
                    beta=0.0,
                    sensor="none",
                    mode=args.mode,
                )
            )
    return cases


def build_solver_input(case: MatrixCase, output_path: Path) -> str:
    lines = [
        str(case.case_id),
        f"{case.xmin:.17g}",
        f"{case.xmax:.17g}",
        f"{case.x0:.17g}",
        str(case.nx),
        f"{case.gamma:.17g}",
        f"{case.cfl:.17g}",
        f"{case.t_max:.17g}",
        str(case.mode),
    ]
    if case.use_viscosity:
        sensor_index = {"rho": 1, "u": 2, "p": 3}[case.sensor]
        lines.extend(
            [
                "y",
                f"{case.beta:.17g}",
                str(sensor_index),
            ]
        )
    else:
        lines.append("n")
    lines.extend(["0", os.fspath(output_path)])
    return "\n".join(lines) + "\n"


def run_matrix_case(
    executable: Path,
    project_dir: Path,
    output_dir: Path,
    case: MatrixCase,
    run_index: int,
) -> tuple[list[dict[str, str | int | float | None]], dict[str, str | int | float]]:
    run_id = f"run_{run_index:04d}"
    case_dir = output_dir / run_id
    case_dir.mkdir(parents=True, exist_ok=True)
    numerical_path = case_dir / "numerical.dat"
    exact_path = case_dir / "numerical_exact.dat"
    try:
        solver_numerical_path = numerical_path.relative_to(project_dir)
    except ValueError:
        solver_numerical_path = numerical_path
    input_text = build_solver_input(case, solver_numerical_path)

    print(
        "[run] "
        f"{case.viscosity_label}, sensor={case.sensor}, "
        f"beta={beta_text(case.beta)}, CFL={case.cfl:g}, Nx={case.nx}"
    )
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

    base = {
        "run_id": run_id,
        "case_name": case.case_name,
        "case": case.case_id,
        "viscosity": case.viscosity_label,
        "sensor": case.sensor,
        "beta": case.beta,
        "cfl": case.cfl,
        "nx": case.nx,
        "mode": case.mode,
        "t_max": case.t_max,
    }
    config = {
        **base,
        "xmin": case.xmin,
        "xmax": case.xmax,
        "x0": case.x0,
        "gamma": case.gamma,
    }
    (case_dir / "config.json").write_text(
        json.dumps(config, indent=2) + "\n",
        encoding="ascii",
    )

    if result.returncode != 0:
        reason = f"solver returned {result.returncode}"
        case_row = {**base, "status": "failed", "reason": reason, "solution_time": None}
        return make_empty_metric_rows(base, "failed", reason, None), case_row
    if not numerical_path.is_file() or not exact_path.is_file():
        reason = "missing numerical.dat or numerical_exact.dat"
        case_row = {**base, "status": "failed", "reason": reason, "solution_time": None}
        return make_empty_metric_rows(base, "failed", reason, None), case_row

    try:
        numerical = read_tecplot_ascii(numerical_path)
        exact = read_tecplot_ascii(exact_path)
        is_valid, reason, solution_time = validate_numerical_solution(
            numerical,
            numerical_path,
            case.t_max,
        )
        if not is_valid:
            case_row = {
                **base,
                "status": "unstable",
                "reason": reason,
                "solution_time": solution_time,
            }
            return (
                make_empty_metric_rows(base, "unstable", reason, solution_time),
                case_row,
            )

        metric_rows = []
        for row in compute_metrics(numerical, exact):
            metric_rows.append(
                {
                    **base,
                    "status": "completed",
                    "reason": reason,
                    "solution_time": solution_time,
                    **row,
                }
            )
        case_row = {
            **base,
            "status": "completed",
            "reason": reason,
            "solution_time": solution_time,
        }
        return metric_rows, case_row
    except (OSError, ValueError) as exc:
        reason = str(exc)
        case_row = {**base, "status": "failed", "reason": reason, "solution_time": None}
        return make_empty_metric_rows(base, "failed", reason, None), case_row


def make_empty_metric_rows(
    base: dict[str, str | int | float],
    status: str,
    reason: str,
    solution_time: float | None,
) -> list[dict[str, str | int | float | None]]:
    rows: list[dict[str, str | int | float | None]] = []
    for variable in VARIABLES:
        rows.append(
            {
                **base,
                "status": status,
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
    return rows


def write_csv(path: Path, rows: list[dict[str, object]], fieldnames: tuple[str, ...]) -> None:
    with path.open("w", encoding="ascii", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fieldnames, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(rows)


def completed_metric_rows(
    rows: list[dict[str, str | int | float | None]],
) -> list[dict[str, str | int | float]]:
    return [
        row  # type: ignore[list-item]
        for row in rows
        if row["status"] == "completed"
    ]


def find_metric_row(
    rows: list[dict[str, str | int | float]],
    *,
    viscosity: str,
    sensor: str,
    beta: float,
    cfl: float,
    nx: int,
    variable: str,
) -> dict[str, str | int | float] | None:
    for row in rows:
        if (
            row["viscosity"] == viscosity
            and row["sensor"] == sensor
            and math.isclose(float(row["beta"]), beta, rel_tol=0.0, abs_tol=1.0e-12)
            and math.isclose(float(row["cfl"]), cfl, rel_tol=0.0, abs_tol=1.0e-12)
            and int(row["nx"]) == nx
            and row["variable"] == variable
        ):
            return row
    return None


def write_best_summary(
    path: Path,
    rows: list[dict[str, str | int | float]],
) -> None:
    summary_rows = []
    for variable in VARIABLES:
        variable_rows = [row for row in rows if row["variable"] == variable]
        if not variable_rows:
            continue
        for metric in METRICS:
            best = min(variable_rows, key=lambda row: float(row[metric]))
            summary_rows.append(
                {
                    "variable": variable,
                    "metric": metric,
                    "value": best[metric],
                    "viscosity": best["viscosity"],
                    "sensor": best["sensor"],
                    "beta": best["beta"],
                    "cfl": best["cfl"],
                    "nx": best["nx"],
                    "mode": best["mode"],
                    "run_id": best["run_id"],
                    "case_name": best["case_name"],
                }
            )
    write_csv(
        path,
        summary_rows,
        (
            "variable",
            "metric",
            "value",
            "viscosity",
            "sensor",
            "beta",
            "cfl",
            "nx",
            "mode",
            "run_id",
            "case_name",
        ),
    )


def slice_rows(
    rows: list[dict[str, str | int | float]],
    *,
    axis: str,
    fixed: dict[str, str | int | float],
    group_field: str,
) -> dict[str, list[dict[str, str | int | float]]]:
    groups: dict[str, list[dict[str, str | int | float]]] = {}
    for row in rows:
        if row["variable"] != "rho":
            continue
        if not matches_fixed(row, fixed):
            continue
        group = str(row[group_field])
        groups.setdefault(group, [])
    for group in groups:
        axis_values = sorted(
            {
                float(row[axis])
                for row in rows
                if matches_fixed(row, fixed)
                and str(row[group_field]) == group
                and row["variable"] == "rho"
            }
        )
        for axis_value in axis_values:
            complete = True
            combined: dict[str, str | int | float] = {
                axis: axis_value,
                group_field: group,
            }
            for variable in VARIABLES:
                candidate = next(
                    (
                        row
                        for row in rows
                        if matches_fixed(row, fixed)
                        and str(row[group_field]) == group
                        and row["variable"] == variable
                        and math.isclose(
                            float(row[axis]),
                            axis_value,
                            rel_tol=0.0,
                            abs_tol=1.0e-12,
                        )
                    ),
                    None,
                )
                if candidate is None:
                    complete = False
                    break
                for metric in METRICS:
                    combined[f"{variable}_{metric}"] = candidate[metric]
            if complete:
                groups[group].append(combined)

    return {
        group: sorted(group_rows, key=lambda row: float(row[axis]))
        for group, group_rows in groups.items()
        if group_rows
    }


def matches_fixed(row: dict[str, str | int | float], fixed: dict[str, str | int | float]) -> bool:
    for key, expected in fixed.items():
        value = row[key]
        if isinstance(expected, float):
            if not math.isclose(float(value), expected, rel_tol=0.0, abs_tol=1.0e-12):
                return False
        else:
            if value != expected:
                return False
    return True


def write_slice_dataset(
    path: Path,
    *,
    title: str,
    axis: str,
    groups: dict[str, list[dict[str, str | int | float]]],
) -> None:
    variables = [axis]
    for variable in VARIABLES:
        for metric in METRICS:
            variables.append(f"{variable}_{metric}")

    with path.open("w", encoding="ascii", newline="\n") as stream:
        stream.write(f'TITLE = "{title}"\n')
        stream.write(
            "VARIABLES = "
            + ", ".join(f'"{variable}"' for variable in variables)
            + "\n"
        )
        for group, group_rows in groups.items():
            stream.write(f'ZONE T="{group}", I={len(group_rows)}, F=POINT\n')
            for row in group_rows:
                values = [float(row[axis])]
                values.extend(float(row[name]) for name in variables[1:])
                stream.write(" ".join(f"{value:.15e}" for value in values) + "\n")


def write_slice_macro(
    macro_path: Path,
    datasets: list[tuple[str, Path, str]],
    output_dir: Path,
    width: int,
    padding: float,
) -> None:
    lines = [
        "#!MC 1410",
        "$!EXPORTSETUP EXPORTFORMAT = PNG",
        f"$!EXPORTSETUP IMAGEWIDTH = {width}",
    ]
    yvar_numbers = {
        f"{variable}_{metric}": 2 + variable_index * len(METRICS) + metric_index
        for variable_index, variable in enumerate(VARIABLES)
        for metric_index, metric in enumerate(METRICS)
    }

    for stem, dataset_path, axis_label in datasets:
        values_by_yvar = read_slice_numeric_values(dataset_path)
        for variable in VARIABLES:
            for metric in METRICS:
                yvar_name = f"{variable}_{metric}"
                ranges = values_by_yvar.get(yvar_name)
                if not ranges:
                    continue
                y_min, y_max = padded_range(
                    ranges,
                    padding_fraction=padding,
                    nonnegative=True,
                )
                output_path = output_dir / f"tecplot_matrix_{stem}_{metric}_{variable}.png"
                lines.extend(
                    [
                        "",
                        "$!NEWLAYOUT",
                        f"$!READDATASET  '\"{tecplot_path(dataset_path)}\"'",
                        "  INITIALPLOTTYPE = XYLINE",
                        "  INCLUDETEXT = NO",
                        "  INCLUDEGEOM = NO",
                        "$!PLOTTYPE = XYLINE",
                        "$!DELETELINEMAPS",
                    ]
                )
                names = zone_names(dataset_path)
                zone_count = len(names)
                for zone_index, zone_name in enumerate(names, start=1):
                    color = LINE_COLORS[(zone_index - 1) % len(LINE_COLORS)]
                    lines.extend(
                        [
                            "$!CREATELINEMAP",
                            f"$!LINEMAP [{zone_index}] NAME = '{zone_name}'",
                            f"$!LINEMAP [{zone_index}] ASSIGN{{ZONE = {zone_index}}}",
                            f"$!LINEMAP [{zone_index}] ASSIGN{{XAXISVAR = 1}}",
                            f"$!LINEMAP [{zone_index}] ASSIGN{{YAXISVAR = {yvar_numbers[yvar_name]}}}",
                            f"$!LINEMAP [{zone_index}] LINES{{COLOR = {color}}}",
                            f"$!LINEMAP [{zone_index}] LINES{{LINETHICKNESS = 0.45}}",
                            f"$!LINEMAP [{zone_index}] SYMBOLS{{SHOW = YES}}",
                            f"$!LINEMAP [{zone_index}] SYMBOLS{{COLOR = {color}}}",
                            f"$!LINEMAP [{zone_index}] SYMBOLS{{SIZE = 0.9}}",
                        ]
                    )
                lines.extend(
                    [
                        f"$!ACTIVELINEMAPS = [1-{zone_count}]",
                        "$!GLOBALLINEPLOT LEGEND{SHOW = YES}",
                        "$!GLOBALLINEPLOT LEGEND{TEXTSHAPE{SIZEUNITS = POINT}}",
                        "$!GLOBALLINEPLOT LEGEND{TEXTSHAPE{HEIGHT = 8}}",
                        "$!XYLINEAXIS XDETAIL 1 {TITLE{TITLEMODE = USETEXT}}",
                        f"$!XYLINEAXIS XDETAIL 1 {{TITLE{{TEXT = '{axis_label}'}}}}",
                        "$!XYLINEAXIS YDETAIL 1 {TITLE{TITLEMODE = USETEXT}}",
                        f"$!XYLINEAXIS YDETAIL 1 {{TITLE{{TEXT = '{metric} error of {Y_LABELS[variable]}'}}}}",
                        "$!VIEW DATAFIT",
                        f"$!XYLINEAXIS YDETAIL 1 {{RANGEMIN = {y_min:.15g}}}",
                        f"$!XYLINEAXIS YDETAIL 1 {{RANGEMAX = {y_max:.15g}}}",
                        f'$!EXPORTSETUP EXPORTFNAME = "{tecplot_path(output_path)}"',
                        "$!EXPORTSTART",
                        "$!EXPORTFINISH",
                    ]
                )
    macro_path.write_text("\n".join(lines) + "\n", encoding="ascii")


def zone_names(path: Path) -> list[str]:
    names: list[str] = []
    pattern = re.compile(r'ZONE\s+T\s*=\s*"([^"]+)"', re.IGNORECASE)
    with path.open("r", encoding="ascii") as stream:
        for line in stream:
            match = pattern.search(line)
            if match:
                names.append(match.group(1))
    return names


def read_slice_numeric_values(path: Path) -> dict[str, list[float]]:
    names: list[str] | None = None
    values: dict[str, list[float]] = {}
    with path.open("r", encoding="ascii") as stream:
        for raw_line in stream:
            line = raw_line.strip()
            if line.startswith("VARIABLES"):
                names = [part.strip().strip('"') for part in line.partition("=")[2].split(",")]
                values = {name: [] for name in names[1:]}
                continue
            if not line or line.upper().startswith(("TITLE", "ZONE")):
                continue
            if names is None:
                continue
            row = [float(part) for part in line.split()]
            for name, value in zip(names[1:], row[1:]):
                values[name].append(value)
    return values


def render_tecplot(macro_path: Path, output_dir: Path) -> None:
    tec360 = shutil.which("tec360.exe") or shutil.which("tec360")
    if not tec360:
        raise FileNotFoundError("tec360.exe was not found in PATH")
    print(f"[tecplot] macro: {macro_path}")
    subprocess.run([tec360, "-b", os.fspath(macro_path.resolve())], cwd=output_dir, check=True)


def write_slices(
    output_dir: Path,
    rows: list[dict[str, str | int | float]],
    args: argparse.Namespace,
) -> list[tuple[str, Path, str]]:
    slices_dir = output_dir / "slices"
    slices_dir.mkdir(exist_ok=True)
    datasets: list[tuple[str, Path, str]] = []

    beta_groups = slice_rows(
        rows,
        axis="beta",
        fixed={
            "viscosity": "on",
            "cfl": args.reference_cfl,
            "nx": args.reference_nx,
        },
        group_field="sensor",
    )
    if beta_groups:
        path = slices_dir / "beta_by_sensor.dat"
        write_slice_dataset(
            path,
            title="Error versus beta grouped by sensor",
            axis="beta",
            groups=beta_groups,
        )
        datasets.append(("beta_by_sensor", path, "Artificial viscosity beta"))

    cfl_groups = slice_rows(
        rows,
        axis="cfl",
        fixed={
            "viscosity": "on",
            "sensor": args.reference_sensor,
            "beta": args.reference_beta,
            "nx": args.reference_nx,
        },
        group_field="sensor",
    )
    if cfl_groups:
        path = slices_dir / "cfl_reference.dat"
        write_slice_dataset(
            path,
            title="Error versus CFL at reference beta, sensor, and grid",
            axis="cfl",
            groups=cfl_groups,
        )
        datasets.append(("cfl_reference", path, "CFL"))

    grid_groups = slice_rows(
        rows,
        axis="nx",
        fixed={
            "viscosity": "on",
            "sensor": args.reference_sensor,
            "beta": args.reference_beta,
            "cfl": args.reference_cfl,
        },
        group_field="sensor",
    )
    if grid_groups:
        path = slices_dir / "grid_reference.dat"
        write_slice_dataset(
            path,
            title="Error versus grid size at reference beta, sensor, and CFL",
            axis="nx",
            groups=grid_groups,
        )
        datasets.append(("grid_reference", path, "Nx"))

    return datasets


def print_summary(
    cases: list[MatrixCase],
    case_rows: list[dict[str, str | int | float]],
    best_path: Path,
) -> None:
    completed = sum(1 for row in case_rows if row["status"] == "completed")
    unstable = sum(1 for row in case_rows if row["status"] == "unstable")
    failed = sum(1 for row in case_rows if row["status"] == "failed")
    print("\n[summary]")
    print(f"total cases : {len(cases)}")
    print(f"completed   : {completed}")
    print(f"unstable    : {unstable}")
    print(f"failed      : {failed}")
    print(f"best table  : {best_path}")


def main() -> int:
    args = parse_args()
    project_dir = Path(__file__).resolve().parents[1]
    if args.output_dir is None:
        args.output_dir = solution_directory(project_dir, args.case) / "targeted_matrix"
    args.output_dir = args.output_dir.resolve()
    args.t_max = CASE_FINAL_TIMES[args.case] if args.t_max is None else args.t_max
    args.betas = unique_sorted(args.betas)
    args.cfls = unique_sorted(args.cfls)
    args.nxs = unique_sorted_int(args.nxs)

    try:
        validate_args(args)
        cases = build_matrix(args)
        print(f"[matrix] case={args.case}, combinations={len(cases)}")
        if args.dry_run:
            return 0

        args.output_dir.mkdir(parents=True, exist_ok=True)
        build_dir = args.output_dir / "_build"
        build_dir.mkdir(exist_ok=True)
        solver_executable = build_dir / "riemann_macc_matrix.exe"
        exact_executable = project_dir / "_Analysical_Solution_Solver" / "riemann_exact.exe"

        compile_program(
            project_dir / "_Analysical_Solution_Solver" / "1-D_Riemann_AM.c",
            exact_executable,
            project_dir,
        )
        compile_program(
            project_dir / "1-D_Riemann_NM_MacC_02.c",
            solver_executable,
            project_dir,
        )

        all_metric_rows: list[dict[str, str | int | float | None]] = []
        case_rows: list[dict[str, str | int | float]] = []
        for run_index, case in enumerate(cases, start=1):
            metric_rows, case_row = run_matrix_case(
                solver_executable,
                project_dir,
                args.output_dir,
                case,
                run_index,
            )
            all_metric_rows.extend(metric_rows)
            case_rows.append(case_row)

        metrics_path = args.output_dir / "errors.csv"
        cases_path = args.output_dir / "matrix_cases.csv"
        best_path = args.output_dir / "best.csv"
        write_csv(
            metrics_path,
            all_metric_rows,
            (
                "run_id",
                "case_name",
                "case",
                "viscosity",
                "sensor",
                "beta",
                "cfl",
                "nx",
                "mode",
                "t_max",
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
            ),
        )
        write_csv(
            cases_path,
            case_rows,
            (
                "run_id",
                "case_name",
                "case",
                "viscosity",
                "sensor",
                "beta",
                "cfl",
                "nx",
                "mode",
                "t_max",
                "status",
                "reason",
                "solution_time",
            ),
        )

        stable_rows = completed_metric_rows(all_metric_rows)
        if stable_rows:
            write_best_summary(best_path, stable_rows)
            datasets = write_slices(args.output_dir, stable_rows, args)
        else:
            datasets = []

        config = {
            "case": args.case,
            "xmin": args.xmin,
            "xmax": args.xmax,
            "x0": args.x0,
            "gamma": args.gamma,
            "t_max": args.t_max,
            "mode": args.mode,
            "betas": args.betas,
            "sensors": args.sensors,
            "cfls": args.cfls,
            "nxs": args.nxs,
            "include_viscosity_off": not args.skip_viscosity_off,
            "reference": {
                "beta": args.reference_beta,
                "sensor": args.reference_sensor,
                "cfl": args.reference_cfl,
                "nx": args.reference_nx,
            },
            "error_definition": {
                "L1": "trapezoidal integral of absolute pointwise error",
                "L2": "sqrt(trapezoidal integral of squared pointwise error)",
                "Linf": "maximum absolute pointwise error",
            },
        }
        (args.output_dir / "targeted_matrix_config.json").write_text(
            json.dumps(config, indent=2) + "\n",
            encoding="ascii",
        )

        if args.backend == "tecplot-macro" and datasets:
            macro_path = args.output_dir / "export_targeted_matrix_with_tecplot.mcr"
            write_slice_macro(
                macro_path,
                datasets,
                args.output_dir,
                args.width,
                args.padding,
            )
            render_tecplot(macro_path, args.output_dir)

        if not args.keep_build:
            shutil.rmtree(build_dir)

        print_summary(cases, case_rows, best_path)
        print(f"[data] {metrics_path}")
        print(f"[done] targeted matrix output: {args.output_dir}")
        return 0
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as exc:
        print(f"[error] {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
