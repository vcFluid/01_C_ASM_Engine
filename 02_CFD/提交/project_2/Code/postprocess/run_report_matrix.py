#!/usr/bin/env python3
"""Run the parameter matrix used in the result report.

For one Riemann case, the number of runs is

    viscosity on : 5 beta * 3 sensors * 3 CFL * 4 grids = 180
    viscosity off:             3 CFL * 4 grids = 12

so each case has 192 runs in total.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import subprocess
import sys


BETAS = ("0.25", "0.5", "0.75", "1.0", "1.5")
SENSORS = ("rho", "u", "p")
CFLS = ("0.2", "0.5", "0.8")
NXS = ("101", "201", "501", "1001")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run the 192-case parameter sweep used by the report."
    )
    parser.add_argument("--case", type=int, choices=range(1, 8), default=1)
    parser.add_argument(
        "--all",
        action="store_true",
        help="run all seven Riemann cases",
    )
    parser.add_argument(
        "--mode",
        type=int,
        choices=range(1, 5),
        default=1,
        help="MacCormack mode passed to the C solver",
    )
    parser.add_argument(
        "--output-root",
        type=Path,
        default=Path("runs") / "report_parameter_matrix",
        help="directory used to save the matrix output",
    )
    parser.add_argument(
        "--backend",
        choices=("none", "tecplot-macro"),
        default="none",
        help="plot backend passed to targeted_matrix.py",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="only print the matrix size and commands",
    )
    return parser.parse_args()


def print_matrix_size() -> None:
    visc_on = len(BETAS) * len(SENSORS) * len(CFLS) * len(NXS)
    visc_off = len(CFLS) * len(NXS)
    total = visc_on + visc_off

    print("[matrix size]")
    print(f"viscosity on : {len(BETAS)} * {len(SENSORS)} * {len(CFLS)} * {len(NXS)} = {visc_on}")
    print(f"viscosity off: {len(CFLS)} * {len(NXS)} = {visc_off}")
    print(f"total per case: {visc_on} + {visc_off} = {total}")


def build_command(case_id: int, args: argparse.Namespace) -> list[str]:
    script_dir = Path(__file__).resolve().parent
    project_dir = script_dir.parent
    output_dir = project_dir / args.output_root / f"case_{case_id:02d}"

    command = [
        sys.executable,
        str(script_dir / "targeted_matrix.py"),
        "--case",
        str(case_id),
        "--betas",
        *BETAS,
        "--sensors",
        *SENSORS,
        "--cfls",
        *CFLS,
        "--nxs",
        *NXS,
        "--mode",
        str(args.mode),
        "--reference-beta",
        "0.5",
        "--reference-sensor",
        "rho",
        "--reference-cfl",
        "0.5",
        "--reference-nx",
        "501",
        "--backend",
        args.backend,
        "--output-dir",
        str(output_dir),
    ]
    if args.dry_run:
        command.append("--dry-run")
    return command


def main() -> int:
    args = parse_args()
    cases = range(1, 8) if args.all else (args.case,)

    print_matrix_size()
    for case_id in cases:
        command = build_command(case_id, args)
        print("\n[command]")
        print(" ".join(command))
        if not args.dry_run:
            subprocess.run(command, check=True)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
