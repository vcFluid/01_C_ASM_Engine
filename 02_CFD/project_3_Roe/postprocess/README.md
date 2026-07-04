# Project 3 Roe Postprocess Mapping

## Keep The Same Output Contract

Project 3 should write the same Tecplot variables as Project 2:

```text
"x", "rho", "u", "p", "E", "q1", "q2", "q3"
```

If this contract is kept, the following Project 2 postprocess ideas remain valid:

- read numerical/exact Tecplot ASCII files;
- crop or interpolate exact solution to the numerical grid;
- compute `L1`, `L2`, and `Linf` on the numerical grid;
- export Tecplot macro comparison plots;
- build transient datasets from numerical/exact snapshots.

## Direct Reuse

`tecplot_compare.py` is almost directly reusable if names and paths are changed:

```text
numerical.dat
numerical_exact.dat
```

The expected fields are only:

```text
x, rho, u, p
```

## Project 2 Scripts To Roe Versions

```text
Project 2 script                  Roe-version meaning
---------------------------------------------------------------------------
tecplot_compare.py                same numerical vs exact comparison
tecplot_animate.py                same snapshot animation workflow
beta_sweep.py                     entropy_fix_sweep.py
parameter_matrix.py               roe_parameter_matrix.py
beta_slider.py                    entropy_slider.py, optional
PARAMETER_MATRIX.md               ROE_PARAMETER_MATRIX.md, optional
```

## Replace The Parameter Axes

Project 2's main parameter axes:

```text
viscosity on/off
beta
sensor = rho/u/p
CFL
Nx
```

Project 3 Roe's cleaner axes:

```text
entropy_fix = off/Harten
entropy_factor = e.g. 0.00, 0.05, 0.10, 0.20
entropy_delta = optional absolute floor
CFL = e.g. 0.2, 0.5, 0.8
Nx = e.g. 101, 201, 501, 1001
case_id = 1..7
```

For the first report-grade pass, keep the matrix smaller:

```text
case    = 1 Sod
entropy = off, Harten
factor  = 0.05, 0.10, 0.20
CFL     = 0.2, 0.5, 0.8
Nx      = 101, 201, 501
```

Then test cases 2-7 one by one for stability and qualitative behavior.

## Metrics

Reuse Project 2's definitions:

```text
L1   = integral |q_num - q_exact| dx
L2   = sqrt(integral (q_num - q_exact)^2 dx)
Linf = max |q_num - q_exact|
```

Keep the same extra diagnostics from the current Project 2 automation:

```text
TV
undershoot
overshoot
status = completed/unstable/failed
solution_time
```

For Roe, `unstable` should include:

```text
rho <= rho_floor
p <= p_floor
NaN/Inf
step limit reached before t_final
exact solver failure
missing numerical/exact output
```

## Interpretation Notes

- Do not describe entropy fix as artificial viscosity beta.
- Roe baseline should be shown with entropy fix off and on, if both complete.
- If entropy-fix-off fails on expansion cases, that is a meaningful method result.
- `Linf` remains sensitive near discontinuities; use `L1` for trends and `Linf` for worst local error.
- Keep Tecplot final plots as the report figures; Matplotlib/slider views are inspection tools.
