# Roe FVM Batch Matrix Pseudocode

This maps the Project 2 batch-driver idea to Project 3 without adding extra Roe modifications.

## Matrix Case

```text
MatrixCase:
    case_id
    xmin, xmax, x0
    nx
    gamma
    cfl
    t_max
```

## Build Matrix

```text
cases = []

for cfl in cfls:
    for nx in nxs:
        cases.append(MatrixCase(case_id, xmin, xmax, x0, nx, gamma, cfl, t_max))
```

## Feed Interactive Solver

Keep Project 2's stdin-driver idea, but answer Roe prompts:

```text
case_id
xmin
xmax
x0
nx
gamma
cfl
t_max
output_interval
output_file
```

## Per-Run Outputs

```text
roe_cfl_..._nx_.../
    config.json
    run.log
    numerical.dat
    numerical_exact.dat
```

## Summary Outputs

```text
matrix_config.json
matrix_results.csv
matrix_errors.dat
SUMMARY.md
```

## Status Rules

```text
completed:
    solver exit code is 0
    numerical and exact files exist
    all parser checks pass

unstable:
    solver reports nonphysical state
    NaN/Inf appears in numerical data

failed:
    executable fails to launch
    exact solver fails
    files are missing
    parser cannot read output
```

## Aggregation For Report

```text
run_report_matrix.py --all
    for case_id in 1..7:
        call targeted_matrix.py
    collect each matrix_results.csv
    write overall_results.csv
    write baseline_exact_comparison.csv
```
