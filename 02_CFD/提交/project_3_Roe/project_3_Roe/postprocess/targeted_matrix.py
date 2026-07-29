#!/usr/bin/env python3
"""Run Roe FVM cases and summarize errors against the exact solution."""

from __future__ import annotations

import argparse
from bisect import bisect_left
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

from tecplot_compare import read_tecplot_ascii


VARIABLES = ("rho", "u", "p")
METRICS = ("L1", "L2", "Linf")
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

    @property
    def case_name(self) -> str:
        return f"roe_cfl_{token(self.cfl)}_nx_{self.nx}"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Run Roe FVM experiments, compare the numerical Tecplot output with "
            "the exact Riemann solution, and write error tables."
        )
    )
    parser.add_argument("--case", type=int, choices=range(1, 8), default=1)
    parser.add_argument("--xmin", type=float, default=0.0)
    parser.add_argument("--xmax", type=float, default=1.0)
    parser.add_argument("--x0", type=float, default=0.5)
    parser.add_argument("--gamma", type=float, default=1.4)
    parser.add_argument("--t-max", type=float, default=None)
    parser.add_argument("--cfls", type=float, nargs="+", default=(0.5,))
    parser.add_argument("--nxs", type=int, nargs="+", default=(501,))
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=None,
        help="defaults to runs/Solution_XX_CaseName/roe_targeted_matrix",
    )
    parser.add_argument(
        "--backend",
        choices=("none", "tecplot-macro"),
        default="none",
        help="reserved to match the Project 2 postprocess CLI",
    )
    parser.add_argument("--keep-build", action="store_true")
    parser.add_argument("--dry-run", action="store_true")
    return parser.parse_args()


def token(value: float) -> str:
    return f"{value:.10g}".replace("-", "m").replace(".", "p")


def solution_directory(project_dir: Path, case_id: int) -> Path:
    return project_dir / "runs" / f"Solution_{case_id:02d}_{CASE_NAMES[case_id]}"


def unique_sorted_float(values: list[float] | tuple[float, ...]) -> list[float]:
    result = sorted(set(float(value) for value in values))
    if not result or any(not math.isfinite(value) for value in result):
        raise ValueError("floating-point parameter lists must be finite and nonempty")
    return result


def unique_sorted_int(values: list[int] | tuple[int, ...]) -> list[int]:
    result = sorted(set(int(value) for value in values))
    if not result:
        raise ValueError("grid-size list must be nonempty")
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
    if any(value <= 0.0 or value >= 1.0 for value in args.cfls):
        raise ValueError("CFL values must satisfy 0 < CFL < 1")
    if any(value < 5 for value in args.nxs):
        raise ValueError("nx must be at least 5")


def build_matrix(args: argparse.Namespace) -> list[MatrixCase]:
    return [
        MatrixCase(
            case_id=args.case,
            xmin=args.xmin,
            xmax=args.xmax,
            x0=args.x0,
            nx=nx,
            gamma=args.gamma,
            cfl=cfl,
            t_max=args.t_max,
        )
        for cfl, nx in itertools.product(args.cfls, args.nxs)
    ]


def compile_program(source: Path, output: Path, cwd: Path) -> None:
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
        "0",
        os.fspath(output_path),
    ]
    return "\n".join(lines) + "\n"


def run_case(executable: Path, project_dir: Path, case: MatrixCase, case_dir: Path) -> dict[str, object]:
    case_dir.mkdir(parents=True, exist_ok=True)
    numerical_path = case_dir / "numerical.dat"
    exact_path = case_dir / "numerical_exact.dat"
    try:
        solver_output = numerical_path.relative_to(project_dir)
    except ValueError:
        solver_output = numerical_path

    result = subprocess.run(
        [os.fspath(executable.resolve())],
        cwd=project_dir,
        input=build_solver_input(case, solver_output),
        text=True,
        encoding="utf-8",
        errors="replace",
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    (case_dir / "run.log").write_text(result.stdout, encoding="utf-8")
    (case_dir / "config.json").write_text(
        json.dumps(case.__dict__, indent=2) + "\n",
        encoding="ascii",
    )

    status = "completed"
    reason = ""
    if result.returncode != 0:
        status = "unstable" if re.search(r"Nonpositive|Nonphysical|NaN|Inf", result.stdout, re.I) else "failed"
        reason = "solver_returncode"
    elif not numerical_path.is_file() or not exact_path.is_file():
        status = "failed"
        reason = "missing_output"

    record: dict[str, object] = {
        "status": status,
        "reason": reason,
        "numerical": numerical_path,
        "exact": exact_path,
    }
    if status != "completed":
        return record

    try:
        numerical = read_tecplot_ascii(numerical_path)
        exact = read_tecplot_ascii(exact_path)
        record.update(compute_all_metrics(numerical, exact))
        record.update(solution_diagnostics(numerical, exact))
    except (OSError, ValueError, RuntimeError) as exc:
        record["status"] = "failed"
        record["reason"] = f"postprocess:{exc}"
    return record


def interpolate_to_grid(source_x: list[float], source_values: list[float], target_x: list[float]) -> list[float]:
    if len(source_x) != len(source_values) or len(source_x) < 2:
        raise ValueError("invalid interpolation source")
    result: list[float] = []
    tolerance = 1.0e-12 * max(1.0, abs(source_x[0]), abs(source_x[-1]))
    for x_value in target_x:
        index = bisect_left(source_x, x_value)
        if index < len(source_x) and abs(source_x[index] - x_value) <= tolerance:
            result.append(source_values[index])
        elif index > 0 and abs(source_x[index - 1] - x_value) <= tolerance:
            result.append(source_values[index - 1])
        elif index == 0 or index == len(source_x):
            raise ValueError("target grid lies outside exact solution domain")
        else:
            x0 = source_x[index - 1]
            x1 = source_x[index]
            y0 = source_values[index - 1]
            y1 = source_values[index]
            weight = (x_value - x0) / (x1 - x0)
            result.append(y0 + weight * (y1 - y0))
    return result


def compute_metrics(x: list[float], numerical: list[float], exact: list[float]) -> dict[str, float]:
    if len(x) != len(numerical) or len(x) != len(exact) or len(x) < 2:
        raise ValueError("metric arrays must have the same length >= 2")
    abs_error = [abs(a - b) for a, b in zip(numerical, exact)]
    sq_error = [(a - b) * (a - b) for a, b in zip(numerical, exact)]
    l1 = 0.0
    l2 = 0.0
    for i in range(len(x) - 1):
        dx = x[i + 1] - x[i]
        l1 += 0.5 * (abs_error[i] + abs_error[i + 1]) * dx
        l2 += 0.5 * (sq_error[i] + sq_error[i + 1]) * dx
    return {
        "L1": l1,
        "L2": math.sqrt(max(l2, 0.0)),
        "Linf": max(abs_error),
    }


def compute_all_metrics(numerical: dict[str, list[float]], exact: dict[str, list[float]]) -> dict[str, float]:
    result: dict[str, float] = {}
    x = numerical["x"]
    exact_x = exact["x"]
    for variable in VARIABLES:
        exact_values = interpolate_to_grid(exact_x, exact[variable], x)
        metrics = compute_metrics(x, numerical[variable], exact_values)
        for metric_name, value in metrics.items():
            result[f"{variable}_{metric_name}"] = value
    return result


def total_variation(values: list[float]) -> float:
    return sum(abs(values[i + 1] - values[i]) for i in range(len(values) - 1))


def solution_diagnostics(numerical: dict[str, list[float]], exact: dict[str, list[float]]) -> dict[str, float]:
    result: dict[str, float] = {}
    x = numerical["x"]
    exact_x = exact["x"]
    for variable in VARIABLES:
        exact_values = interpolate_to_grid(exact_x, exact[variable], x)
        values = numerical[variable]
        e_min = min(exact_values)
        e_max = max(exact_values)
        result[f"{variable}_min"] = min(values)
        result[f"{variable}_max"] = max(values)
        result[f"{variable}_TV"] = total_variation(values)
        result[f"{variable}_undershoot"] = max(0.0, e_min - min(values))
        result[f"{variable}_overshoot"] = max(0.0, max(values) - e_max)
    return result


def write_results_csv(path: Path, rows: list[dict[str, object]]) -> None:
    columns = [
        "case_id",
        "case_name",
        "status",
        "reason",
        "cfl",
        "nx",
        "gamma",
        "t_max",
    ]
    for variable in VARIABLES:
        columns.extend(f"{variable}_{metric}" for metric in METRICS)
        columns.extend(
            [
                f"{variable}_min",
                f"{variable}_max",
                f"{variable}_TV",
                f"{variable}_undershoot",
                f"{variable}_overshoot",
            ]
        )
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=columns, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(rows)


def write_errors_dat(path: Path, rows: list[dict[str, object]]) -> None:
    columns = ["case_id", "CFL", "Nx", "variable_id", "L1", "L2", "Linf"]
    completed_rows = [row for row in rows if row["status"] == "completed"]
    with path.open("w", encoding="ascii") as stream:
        stream.write('TITLE = "Roe FVM errors against exact solution"\n')
        stream.write("VARIABLES = " + ", ".join(f'"{name}"' for name in columns) + "\n")
        stream.write(f'ZONE T="completed", I={len(completed_rows) * len(VARIABLES)}, F=POINT\n')
        for row in completed_rows:
            for variable_id, variable in enumerate(VARIABLES, start=1):
                stream.write(
                    " ".join(
                        [
                            str(row["case_id"]),
                            f'{float(row["cfl"]):.15e}',
                            str(row["nx"]),
                            str(variable_id),
                            f'{float(row[f"{variable}_L1"]):.15e}',
                            f'{float(row[f"{variable}_L2"]):.15e}',
                            f'{float(row[f"{variable}_Linf"]):.15e}',
                        ]
                    )
                    + "\n"
                )


def summarize_best(rows: list[dict[str, object]]) -> str:
    completed_rows = [row for row in rows if row["status"] == "completed"]
    failed_rows = [row for row in rows if row["status"] != "completed"]
    lines = [
        "# Roe FVM Summary",
        "",
        f"- completed: {len(completed_rows)}",
        f"- unstable/failed: {len(failed_rows)}",
        "",
    ]
    for variable in VARIABLES:
        if completed_rows:
            best = min(completed_rows, key=lambda row: float(row[f"{variable}_L1"]))
            lines.append(
                f"- best {variable} L1: {float(best[f'{variable}_L1']):.6e} "
                f"(CFL={best['cfl']}, Nx={best['nx']})"
            )
    if failed_rows:
        lines.extend(["", "## Unstable Or Failed Cases", ""])
        for row in failed_rows[:30]:
            lines.append(
                f"- {row['case_name']}: status={row['status']}, reason={row['reason']}"
            )
        if len(failed_rows) > 30:
            lines.append(f"- ... {len(failed_rows) - 30} more")
    return "\n".join(lines) + "\n"


def row_from_case(case: MatrixCase, record: dict[str, object]) -> dict[str, object]:
    row: dict[str, object] = {
        "case_id": case.case_id,
        "case_name": case.case_name,
        "status": record.get("status", "failed"),
        "reason": record.get("reason", ""),
        "cfl": case.cfl,
        "nx": case.nx,
        "gamma": case.gamma,
        "t_max": case.t_max,
    }
    row.update({key: value for key, value in record.items() if isinstance(value, (int, float))})
    return row


def main() -> int:
    args = parse_args()
    args.cfls = unique_sorted_float(args.cfls)
    args.nxs = unique_sorted_int(args.nxs)
    if args.t_max is None:
        args.t_max = CASE_FINAL_TIMES[args.case]
    validate_args(args)

    script_dir = Path(__file__).resolve().parent
    project_dir = script_dir.parent
    if args.output_dir is None:
        args.output_dir = solution_directory(project_dir, args.case) / "roe_targeted_matrix"
    args.output_dir.mkdir(parents=True, exist_ok=True)

    cases = build_matrix(args)
    print(f"[matrix] cases = {len(cases)}")
    if args.dry_run:
        for case in cases:
            print(case.case_name)
        return 0

    build_dir = args.output_dir / "build"
    build_dir.mkdir(parents=True, exist_ok=True)
    executable = build_dir / "riemann_roe_matrix.exe"
    compile_program(project_dir / "1-D_Riemann_Roe.c", executable, project_dir)

    rows: list[dict[str, object]] = []
    for index, case in enumerate(cases, start=1):
        case_dir = args.output_dir / case.case_name
        print(f"[run {index}/{len(cases)}] {case.case_name}")
        record = run_case(executable, project_dir, case, case_dir)
        rows.append(row_from_case(case, record))

    write_results_csv(args.output_dir / "matrix_results.csv", rows)
    write_errors_dat(args.output_dir / "matrix_errors.dat", rows)
    (args.output_dir / "SUMMARY.md").write_text(summarize_best(rows), encoding="utf-8")
    (args.output_dir / "matrix_config.json").write_text(
        json.dumps(
            {
                "case": args.case,
                "cfls": args.cfls,
                "nxs": args.nxs,
                "backend": args.backend,
            },
            indent=2,
        )
        + "\n",
        encoding="ascii",
    )

    if not args.keep_build:
        shutil.rmtree(build_dir, ignore_errors=True)

    completed = sum(1 for row in rows if row["status"] == "completed")
    print(f"[done] completed {completed}/{len(rows)}")
    print(f"[data] {args.output_dir.resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
