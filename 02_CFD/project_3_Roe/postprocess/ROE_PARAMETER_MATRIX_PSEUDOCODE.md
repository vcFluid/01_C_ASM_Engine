# Roe Parameter Matrix Pseudocode

This maps the current Project 2 `parameter_matrix.py` idea to Project 3.

## Matrix Case

```text
MatrixCase:
    case_id
    xmin, xmax, x0
    nx
    gamma
    cfl
    t_max
    entropy_fix_type       # off or Harten
    entropy_fix_factor
    entropy_fix_delta
```

## Build Matrix

```text
cases = []

for entropy_fix_type in [off, Harten]:
    for entropy_fix_factor in factors:
        for cfl in cfls:
            for nx in nxs:
                cases.append(MatrixCase(...))

Special rule:
    if entropy_fix_type == off:
        use factor = 0 and delta = 0 only once
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
enable entropy fix? y/n
entropy_fix_factor
entropy_fix_delta
output_interval
output_file
```

If entropy fix is off:

```text
enable entropy fix? n
output_interval
output_file
```

## Per-Run Outputs

```text
run_XXXX/
    config.json
    run.log
    numerical.dat
    numerical_exact.dat
```

## Summary Outputs

```text
matrix_config.json
matrix_cases.csv
matrix_errors.csv
matrix_best.csv
slices/
    entropy_factor.dat
    cfl_reference.dat
    grid_reference.dat
```

## Status Rules

```text
completed:
    solver exit code is 0
    numerical and exact files exist
    solution_time reaches t_final
    all rho/p values are physical

unstable:
    solver reports nonphysical state
    solution_time < t_final
    NaN/Inf appears in numerical data

failed:
    executable fails to launch
    exact solver fails
    files are missing
    parser cannot read output
```

## Slices

```text
entropy_factor.dat:
    fixed CFL and Nx
    compare error versus entropy_fix_factor

cfl_reference.dat:
    fixed entropy factor and Nx
    compare error versus CFL

grid_reference.dat:
    fixed entropy factor and CFL
    compare error versus Nx
```
