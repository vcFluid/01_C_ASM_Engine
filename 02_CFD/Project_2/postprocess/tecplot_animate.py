#!/usr/bin/env python3
"""Build Tecplot transient data and export numerical/exact animations."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess

from tecplot_compare import (
    Y_LABELS,
    all_rows,
    padded_range,
    read_tecplot_ascii,
    tecplot_path,
)


TIME_PATTERN = re.compile(r'ZONE\s+T="t=([0-9Ee+\-.]+)"', re.IGNORECASE)


def parse_args() -> argparse.Namespace:
    project_dir = Path(__file__).resolve().parents[1]
    default_run = project_dir / "runs" / "Solution_01_Sod"

    parser = argparse.ArgumentParser(
        description="Combine paired snapshots and export Tecplot AVI animations."
    )
    parser.add_argument(
        "--numerical",
        type=Path,
        default=default_run / "numerical.dat",
        help="final numerical file; step snapshots are found beside it",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=None,
        help="animation output directory",
    )
    parser.add_argument("--width", type=int, default=1200)
    parser.add_argument("--padding", type=float, default=0.05)
    return parser.parse_args()


def read_solution_time(path: Path) -> float:
    with path.open("r", encoding="utf-8-sig") as stream:
        for line in stream:
            match = TIME_PATTERN.search(line)
            if match:
                return float(match.group(1))
    raise ValueError(f"No solution time found in {path}")


def exact_path_for(numerical_path: Path) -> Path:
    return numerical_path.with_name(
        numerical_path.stem + "_exact" + numerical_path.suffix
    )


def collect_frames(final_numerical: Path) -> list[tuple[float, Path, Path]]:
    directory = final_numerical.parent
    step_pattern = final_numerical.stem + "_step_*" + final_numerical.suffix
    candidates = sorted(
        path
        for path in directory.glob(step_pattern)
        if not path.stem.endswith("_exact")
    )
    candidates.append(final_numerical)

    frames_by_time: dict[float, tuple[Path, Path]] = {}
    for numerical_path in candidates:
        exact_path = exact_path_for(numerical_path)
        if not exact_path.is_file():
            raise FileNotFoundError(
                f"Missing exact snapshot for {numerical_path.name}: {exact_path.name}"
            )
        time = read_solution_time(numerical_path)
        frames_by_time[time] = (numerical_path, exact_path)

    return [
        (time, *frames_by_time[time])
        for time in sorted(frames_by_time)
    ]


def write_transient_dataset(
    output_path: Path,
    frames: list[tuple[float, Path, Path]],
    padding_fraction: float,
) -> tuple[dict[str, tuple[float, float]], float, float]:
    all_values = {"rho": [], "u": [], "p": []}
    xmin = float("inf")
    xmax = float("-inf")

    with output_path.open("w", encoding="ascii", newline="\n") as stream:
        stream.write('TITLE = "Riemann numerical vs exact transient"\n')
        stream.write('VARIABLES = "x", "rho", "u", "p"\n')

        for frame_index, (time, numerical_path, exact_path) in enumerate(frames):
            numerical = read_tecplot_ascii(numerical_path)
            exact = read_tecplot_ascii(exact_path)
            numerical_rows = all_rows(numerical)
            exact_rows = all_rows(exact)

            if len(numerical_rows) != len(exact_rows):
                raise ValueError(
                    f"Grid size mismatch at t={time}: "
                    f"{len(numerical_rows)} != {len(exact_rows)}"
                )

            for numerical_row, exact_row in zip(numerical_rows, exact_rows):
                if abs(numerical_row[0] - exact_row[0]) > 1.0e-10:
                    raise ValueError(f"Grid coordinate mismatch at t={time}")

            xmin = min(xmin, numerical_rows[0][0])
            xmax = max(xmax, numerical_rows[-1][0])
            for name, index in (("rho", 1), ("u", 2), ("p", 3)):
                all_values[name].extend(row[index] for row in numerical_rows)
                all_values[name].extend(row[index] for row in exact_rows)

            for zone_name, strand_id, rows in (
                ("Numerical", 1, numerical_rows),
                ("Exact", 2, exact_rows),
            ):
                stream.write(
                    f'ZONE T="{zone_name} t={time:.10f}", '
                    f"STRANDID={strand_id}, SOLUTIONTIME={time:.15e}, "
                    f"I={len(rows)}, F=POINT\n"
                )
                for row in rows:
                    stream.write(
                        " ".join(f"{value:.15e}" for value in row) + "\n"
                    )

    ranges = {
        name: padded_range(
            values,
            padding_fraction=padding_fraction,
            nonnegative=name in {"rho", "p"},
        )
        for name, values in all_values.items()
    }
    return ranges, xmin, xmax


def write_animation_macro(
    macro_path: Path,
    transient_path: Path,
    output_dir: Path,
    frames: list[tuple[float, Path, Path]],
    ranges: dict[str, tuple[float, float]],
    xmin: float,
    xmax: float,
    width: int,
) -> None:
    lines = [
        "#!MC 1410",
        "",
        f"$!READDATASET  '\"{tecplot_path(transient_path)}\"'",
        "  INITIALPLOTTYPE = XYLINE",
        "  INCLUDETEXT = NO",
        "  INCLUDEGEOM = NO",
        "$!PLOTTYPE = XYLINE",
        "$!DELETELINEMAPS",
        "$!GLOBALLINEPLOT LEGEND{SHOW = YES}",
        "$!XYLINEAXIS XDETAIL 1 {TITLE{TITLEMODE = USETEXT}}",
        "$!XYLINEAXIS XDETAIL 1 {TITLE{TEXT = 'x'}}",
    ]

    variable_numbers = {"rho": 2, "u": 3, "p": 4}
    map_count = 2 * len(frames)

    for variable_name in ("rho", "u", "p"):
        lines.append("$!DELETELINEMAPS")
        for frame_index in range(len(frames)):
            numerical_zone = 2 * frame_index + 1
            exact_zone = numerical_zone + 1
            numerical_map = numerical_zone
            exact_map = exact_zone

            lines.extend(
                [
                    "$!CREATELINEMAP",
                    f"$!LINEMAP [{numerical_map}] NAME = 'MacCormack'",
                    f"$!LINEMAP [{numerical_map}] ASSIGN{{ZONE = {numerical_zone}}}",
                    f"$!LINEMAP [{numerical_map}] ASSIGN{{XAXISVAR = 1}}",
                    f"$!LINEMAP [{numerical_map}] ASSIGN{{YAXISVAR = {variable_numbers[variable_name]}}}",
                    f"$!LINEMAP [{numerical_map}] LINES{{COLOR = RED}}",
                    f"$!LINEMAP [{numerical_map}] LINES{{LINETHICKNESS = 0.4}}",
                    "$!CREATELINEMAP",
                    f"$!LINEMAP [{exact_map}] NAME = 'Exact'",
                    f"$!LINEMAP [{exact_map}] ASSIGN{{ZONE = {exact_zone}}}",
                    f"$!LINEMAP [{exact_map}] ASSIGN{{XAXISVAR = 1}}",
                    f"$!LINEMAP [{exact_map}] ASSIGN{{YAXISVAR = {variable_numbers[variable_name]}}}",
                    f"$!LINEMAP [{exact_map}] LINES{{COLOR = BLACK}}",
                    f"$!LINEMAP [{exact_map}] LINES{{LINETHICKNESS = 0.5}}",
                ]
            )

        y_min, y_max = ranges[variable_name]
        movie_path = output_dir / f"tecplot_{variable_name}_animation.avi"
        lines.extend(
            [
                f"$!XYLINEAXIS YDETAIL 1 {{TITLE{{TITLEMODE = USETEXT}}}}",
                f"$!XYLINEAXIS YDETAIL 1 {{TITLE{{TEXT = '{Y_LABELS[variable_name]}'}}}}",
                f"$!XYLINEAXIS XDETAIL 1 {{RANGEMIN = {xmin:.15g}}}",
                f"$!XYLINEAXIS XDETAIL 1 {{RANGEMAX = {xmax:.15g}}}",
                f"$!XYLINEAXIS YDETAIL 1 {{RANGEMIN = {y_min:.15g}}}",
                f"$!XYLINEAXIS YDETAIL 1 {{RANGEMAX = {y_max:.15g}}}",
                "$!EXPORTSETUP",
                "  EXPORTFORMAT = AVI",
                f'  EXPORTFNAME = "{tecplot_path(movie_path)}"',
                f"  IMAGEWIDTH = {width}",
            ]
        )

        for frame_index, (time, _, _) in enumerate(frames):
            first_map = 2 * frame_index + 1
            second_map = first_map + 1
            lines.extend(
                [
                    f"$!ACTIVELINEMAPS = [{first_map}-{second_map}]",
                    f"$!GLOBALTIME SOLUTIONTIME = {time:.15e}",
                    "$!REDRAWALL",
                ]
            )
            if frame_index == 0:
                lines.extend(
                    [
                        "$!EXPORTSTART",
                        "  EXPORTREGION = CURRENTFRAME",
                    ]
                )
            else:
                lines.append("$!EXPORTNEXTFRAME")

        lines.extend(
            [
                "$!EXPORTFINISH",
                f"$!ACTIVELINEMAPS = [1-{map_count}]",
            ]
        )

    macro_path.write_text("\n".join(lines) + "\n", encoding="ascii")


def main() -> int:
    args = parse_args()
    numerical_path = args.numerical.resolve()
    output_dir = (
        args.output_dir.resolve()
        if args.output_dir is not None
        else numerical_path.parent / "animation"
    )
    output_dir.mkdir(parents=True, exist_ok=True)

    frames = collect_frames(numerical_path)
    if len(frames) < 2:
        raise ValueError("At least two snapshot times are required for animation")

    transient_path = output_dir / "numerical_exact_transient.dat"
    ranges, xmin, xmax = write_transient_dataset(
        transient_path,
        frames,
        args.padding,
    )

    macro_path = output_dir / "export_animation_with_tecplot.mcr"
    write_animation_macro(
        macro_path,
        transient_path,
        output_dir,
        frames,
        ranges,
        xmin,
        xmax,
        args.width,
    )

    tec360 = shutil.which("tec360.exe") or shutil.which("tec360")
    if not tec360:
        raise FileNotFoundError("tec360.exe was not found in PATH")

    print(f"[animation] frames: {len(frames)}")
    print(f"[animation] time range: {frames[0][0]} -> {frames[-1][0]}")
    print(f"[animation] transient data: {transient_path}")
    print(f"[animation] macro: {macro_path}")

    subprocess.run([tec360, "-b", os.fspath(macro_path)], check=True)

    for variable_name in ("rho", "u", "p"):
        candidates = sorted(output_dir.glob(f"tecplot_{variable_name}_animation*.avi"))
        if not candidates:
            raise RuntimeError(f"Tecplot did not export {variable_name} animation")
        print(f"[export] {candidates[-1]}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
