#!/usr/bin/env python3
"""Run the Roe FVM cases used by the Project 3 result report."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
import subprocess
import sys


DEFAULT_CFLS = ("0.5",)
DEFAULT_NXS = ("501",)
CASE_NAMES = {
    1: "Sod",
    2: "LAX",
    3: "Subsonic_Double_Expansion",
    4: "Sjogreen_Supersonic_Expansion",
    5: "Contact_Double_Expansion",
    6: "Contact_Double_Shock",
    7: "Pure_Contact",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run Roe FVM numerical solutions and exact-solution comparisons."
    )
    parser.add_argument("--case", type=int, choices=range(1, 8), default=1)
    parser.add_argument("--all", action="store_true")
    parser.add_argument("--cfls", nargs="+", default=DEFAULT_CFLS)
    parser.add_argument("--nxs", nargs="+", default=DEFAULT_NXS)
    parser.add_argument(
        "--output-root",
        type=Path,
        default=Path("runs") / "ppt_formula_exact_only",
    )
    parser.add_argument("--backend", choices=("none", "tecplot-macro"), default="none")
    parser.add_argument("--dry-run", action="store_true")
    return parser.parse_args()


def print_matrix_size(args: argparse.Namespace) -> None:
    print("[matrix size]")
    print(f"CFL count : {len(args.cfls)}")
    print(f"Nx count  : {len(args.nxs)}")
    print(f"total per case: {len(args.cfls) * len(args.nxs)}")


def build_command(case_id: int, args: argparse.Namespace) -> list[str]:
    script_dir = Path(__file__).resolve().parent
    project_dir = script_dir.parent
    output_dir = project_dir / args.output_root / f"case_{case_id:02d}"
    command = [
        sys.executable,
        str(script_dir / "targeted_matrix.py"),
        "--case",
        str(case_id),
        "--cfls",
        *args.cfls,
        "--nxs",
        *args.nxs,
        "--backend",
        args.backend,
        "--output-dir",
        str(output_dir),
    ]
    if args.dry_run:
        command.append("--dry-run")
    return command


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8-sig", newline="") as stream:
        return list(csv.DictReader(stream))


def write_csv(path: Path, rows: list[dict[str, str]]) -> None:
    if not rows:
        return
    columns = list(rows[0].keys())
    with path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=columns, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(rows)


def same_float(left: str, right: str) -> bool:
    return abs(float(left) - float(right)) <= 1.0e-12


def aggregate_results(project_dir: Path, args: argparse.Namespace, cases: list[int]) -> None:
    output_root = project_dir / args.output_root
    all_rows: list[dict[str, str]] = []
    for case_id in cases:
        path = output_root / f"case_{case_id:02d}" / "matrix_results.csv"
        if not path.is_file():
            continue
        for row in read_csv(path):
            enriched = {
                "case_no": f"{case_id:02d}",
                "case_title": CASE_NAMES[case_id],
            }
            enriched.update(row)
            all_rows.append(enriched)

    write_csv(output_root / "overall_results.csv", all_rows)

    baseline_cfl = args.cfls[0]
    baseline_nx = str(int(args.nxs[0]))
    baseline_rows = [
        row
        for row in all_rows
        if same_float(row["cfl"], baseline_cfl) and str(int(row["nx"])) == baseline_nx
    ]
    write_csv(output_root / "baseline_exact_comparison.csv", baseline_rows)


def main() -> int:
    args = parse_args()
    cases = list(range(1, 8)) if args.all else [args.case]
    print_matrix_size(args)
    for case_id in cases:
        command = build_command(case_id, args)
        print("\n[command]")
        print(" ".join(command))
        if not args.dry_run:
            subprocess.run(command, check=True)

    if not args.dry_run:
        script_dir = Path(__file__).resolve().parent
        aggregate_results(script_dir.parent, args, cases)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
