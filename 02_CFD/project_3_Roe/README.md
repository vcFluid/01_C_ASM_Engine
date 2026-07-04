# Project 3 Roe Solver Skeleton

## Core Judgment

Project 3 should stay isomorphic to the current Project 2 architecture, but the numerical-method core must change.

The reusable Project 2 architecture is:

- `solver` struct owns global parameters, arrays, and method pointers.
- Conservative variables `Q = [rho, rho*u, rho*E]` are the primary state.
- Primitive variables `W = [rho, u, p]` are synchronized from `Q` every step.
- CFL uses the spectral radius `|u| + a`.
- Seven built-in Riemann cases are selected before allocation.
- The solver writes Tecplot ASCII data with `x, rho, u, p, E, q1, q2, q3`.
- Project 0 exact solver is called as an external batch process with the same state, domain, grid, gamma, and time.
- Snapshots and exact snapshots can be paired for animation.
- Postprocess scripts read numerical/exact Tecplot files and compute `L1/L2/Linf`.

The non-reusable Project 2 core is:

```text
MacCormack:
    cell physical flux F_i = F(Q_i)
    predictor Q_bar
    corrector Q_next
    optional artificial viscosity

Roe:
    interface numerical flux Fhat_{i+1/2}
    direct finite-volume update Q_next
    entropy fix and positivity diagnostics
```

So Project 3's real work is concentrated in:

```text
physical_flux_from_q()
roe_flux()
compute_all_roe_fluxes()
step_roe()
check_physical_state()
```

## Current Project 2 To Project 3 Map

```text
Project 2 current feature                  Project 3 Roe pseudocode mapping
--------------------------------------------------------------------------------
Riemann_1D_MacC_solver                     Riemann_1D_Roe_solver
q1/q2/q3                                   same
q1_bar/q2_bar/q3_bar                       remove; MacCormack-only
q1_next/q2_next/q3_next                    same
f1/f2/f3 cell flux                         physical_flux_from_q helper only
f1_bar/f2_bar/f3_bar                       remove; MacCormack-only
fhat1/fhat2/fhat3                          add; Roe interface flux
compute_flux(Q arrays)                     physical_flux_from_q(Q)
step_maccormack()                          step_roe()
artificial viscosity beta/sensor           replace by entropy fix controls
rho_floor/p_floor                          keep as physical-state diagnostics
7 built-in cases                           keep
ask_int_range/ask_double_min/ask_yes_no     keep pattern
ask_output_filename                         keep
make_exact_filename                         keep
make_snapshot_filename                      keep
run_exact_solver                            keep
write_tecplot variables                     keep exact variable contract
output_interval snapshots                   keep
step limit 200000                           keep initially
beta_sweep.py                               map to entropy_fix_sweep.py
parameter_matrix.py                         map axes to entropy/CFL/Nx/case
beta_slider.py                              map to entropy-fix slider if useful
tecplot_compare.py                          reusable if output names match
tecplot_animate.py                          reusable if snapshot names match
```

## Roe Formula

For 1-D Euler:

```text
dQ/dt + dF(Q)/dx = 0
Q = [rho, rho*u, rho*E]
F = [rho*u, rho*u*u + p, u*(rho*E + p)]
```

Roe finite-volume update:

```text
Q_i^{n+1} = Q_i^n - dt/dx * (Fhat_{i+1/2} - Fhat_{i-1/2})
```

Roe interface flux:

```text
Fhat_{i+1/2}
  = 0.5 * (F_L + F_R)
  - 0.5 * sum_k abs(lambda_tilde_k) * alpha_k * r_tilde_k
```

## Recommended Implementation Order

1. Port the Project 2 skeleton without MacCormack predictor arrays.
2. Keep the same input/output contract first.
3. Implement `pressure_from_q`, `total_energy_density`, `primitive_from_q`, and `physical_flux_from_q`.
4. Implement Roe averages, eigenvalues, wave strengths, and right eigenvectors inside `roe_flux`.
5. Implement `compute_all_roe_fluxes`.
6. Implement `step_roe`.
7. Add entropy fix.
8. Add physical-state diagnostics for `rho <= 0`, `p <= 0`, `NaN`, and `Inf`.
9. Reuse exact comparison and Tecplot output.
10. Only after Sod is stable, create Roe versions of sweep/matrix scripts.

## Roe-Specific Experiment Axes

Project 2's formal parameter study is about artificial viscosity:

```text
viscosity on/off, beta, sensor, CFL, Nx
```

For Project 3 Roe, the cleaner parameter study is:

```text
entropy_fix on/off
entropy_delta or entropy_factor
CFL
Nx
case_id
positivity failure/stability status
```

Do not call Roe's entropy fix "artificial viscosity beta" in the report. They are both stabilizing/dissipative devices, but they enter the scheme differently.

## Risks

- Roe flux is an interface flux. It cannot be implemented by simply replacing Project 2's `compute_flux()`.
- Roe without entropy fix can produce entropy-violating expansion shocks in transonic rarefactions.
- Roe does not guarantee positivity. Strong expansion and near-vacuum cases may fail even if Sod works.
- `Linf` near shocks and contacts is sensitive to grid alignment. Use `L1` for overall trend discussion first.
- If output variables or file naming drift from Project 2, the postprocess pipeline will break.

## Files

```text
project_3_Roe/
  README.md
  1-D_Riemann_Roe_PSEUDOCODE.c
  postprocess/README.md
  runs/.gitignore
```

The C file is still pseudocode. It is a map from current Project 2 into a Roe implementation plan, not a compilable solver.
