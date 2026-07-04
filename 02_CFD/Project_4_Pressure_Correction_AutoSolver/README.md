# Project 4 Pressure Correction Auto Solver

## Core Goal

This project is reserved for an automatic solver for a correction equation using a self-iteration loop.

Working assumption:

- target CFD direction: incompressible viscous flow;
- correction equation: pressure-correction equation used by SIMPLE / SIMPLEC / PISO-type algorithms;
- "self-loop" means the code repeatedly solves the correction equation, updates velocity / pressure, checks residuals, and continues until convergence without manual intervention.

No solver code is included yet. This README defines the numerical target and implementation boundary first.

## Mathematical Target

For incompressible viscous CFD, the governing equations are usually written as:

```text
div(u) = 0

rho * (du/dt + u . grad(u)) =
    -grad(p) + mu * laplacian(u) + source
```

The pressure field is not advanced by its own physical time equation. Instead, pressure acts as a constraint variable that enforces:

```text
div(u) = 0
```

Therefore, after a provisional velocity field `u*` is obtained from the momentum equation, a correction equation is solved to enforce mass conservation:

```text
p = p_old + alpha_p * p'
u = u* + u'
```

The pressure correction `p'` is chosen so that the corrected velocity satisfies:

```text
div(u) -> 0
```

For a finite-volume method, the correction equation is expected to become a sparse linear system:

```text
A_p * p' = b
```

where `b` is mainly determined by the mass imbalance of the current provisional velocity field.

## Intended Self-Iteration Logic

The future solver should follow this high-level loop:

```text
initialize mesh, fields, material properties, boundary conditions

while global_iteration < max_iteration:
    solve discretized momentum equations -> provisional velocity u*
    assemble pressure-correction equation A_p p' = b
    solve pressure-correction linear system
    correct pressure p and velocity u
    update boundary conditions
    compute residuals:
        continuity residual
        momentum residual
        pressure-correction residual

    if residuals satisfy tolerance:
        stop as converged
    else:
        continue automatically
```

The key feature is not just solving one equation once. The code should manage the whole nonlinear correction cycle by itself.

## First Implementation Scope

Start with the smallest useful problem:

```text
2-D incompressible laminar flow
structured Cartesian grid
finite-volume discretization
collocated or staggered variables, to be decided before coding
steady SIMPLE-like iteration first
constant rho and mu
simple boundary conditions
```

Recommended first validation case:

```text
lid-driven cavity flow
Re = 100
square domain [0, 1] x [0, 1]
no-slip walls
top lid velocity u = 1, v = 0
```

This case is better than starting from a complicated geometry because the correction equation, boundary treatment, and residual behavior can be debugged cleanly.

## Important Numerical Decisions Before Coding

### Variable Arrangement

Two options:

```text
staggered grid:
    pressure at cell centers
    velocity components at cell faces
    stronger pressure-velocity coupling
    less risk of checkerboard pressure

collocated grid:
    all variables at cell centers
    simpler data structure
    requires Rhie-Chow interpolation to avoid checkerboard pressure
```

For a first pressure-correction solver, staggered grid is numerically safer. Collocated grid is more aligned with many modern CFD codes but needs more careful interpolation.

### Linear Solver

The pressure-correction equation is elliptic-like and usually dominates the convergence behavior.

Candidate solvers:

```text
Jacobi:
    easiest to implement
    slow convergence

Gauss-Seidel / SOR:
    still simple
    better first practical choice

Conjugate Gradient:
    good if matrix is symmetric positive definite
    requires cleaner matrix assembly

Multigrid:
    best long-term direction
    not recommended as the first implementation step
```

Recommended first choice:

```text
Gauss-Seidel or SOR for p'
```

### Under-Relaxation

The pressure and momentum updates should support under-relaxation:

```text
p_new = p_old + alpha_p * p'
u_new = u_old + alpha_u * delta_u
```

Typical starting values:

```text
alpha_p = 0.2 to 0.5
alpha_u = 0.5 to 0.8
```

These are not universal constants. They are stability controls for the nonlinear iteration.

## Residuals And Stop Criteria

At minimum, the code should report:

```text
iteration
continuity_residual
u_momentum_residual
v_momentum_residual
pressure_correction_residual
max_divergence
```

Recommended convergence condition:

```text
continuity_residual < continuity_tol
momentum_residuals < momentum_tol
max_divergence < divergence_tol
```

Do not judge convergence only by visual smoothness of the flow field.

## Planned File Structure

The future project can grow into:

```text
Project_4_Pressure_Correction_AutoSolver/
  README.md
  src/
    main.c
    mesh.c
    fields.c
    momentum.c
    pressure_correction.c
    linear_solver.c
    boundary.c
    residual.c
    output.c
  include/
    mesh.h
    fields.h
    solver.h
  runs/
    README.md
  postprocess/
    README.md
```

For now, only this README exists.

## Suggested Implementation Order

1. Define grid, indexing, and field storage.
2. Implement boundary-condition storage and update functions.
3. Implement provisional momentum solve in the simplest stable form.
4. Assemble the pressure-correction equation.
5. Implement Gauss-Seidel / SOR for the correction equation.
6. Correct pressure and velocity.
7. Compute continuity residual and max divergence.
8. Add automatic self-iteration until convergence.
9. Write Tecplot / CSV output.
10. Validate against lid-driven cavity benchmark data.

## Risks And Common Mistakes

- Pressure correction is a constraint-enforcement mechanism, not an independent physical pressure evolution equation.
- A collocated grid without Rhie-Chow interpolation can produce checkerboard pressure.
- Boundary conditions for `p'` are easy to mishandle; they must be consistent with velocity correction and mass conservation.
- A small linear-solver residual does not guarantee the nonlinear SIMPLE loop has converged.
- Too aggressive under-relaxation can make the self-loop diverge even if each sub-equation is discretized correctly.
- For incompressible flow, mass conservation should be checked through face fluxes, not only through cell-center velocity gradients.

## References To Use Later

- Patankar, S. V. (1980). *Numerical Heat Transfer and Fluid Flow*. Hemisphere.
- Ferziger, J. H., Peric, M., and Street, R. L. (2020). *Computational Methods for Fluid Dynamics*. Springer.
- Versteeg, H. K. and Malalasekera, W. (2007). *An Introduction to Computational Fluid Dynamics: The Finite Volume Method*. Pearson.
- Ghia, U., Ghia, K. N., and Shin, C. T. (1982). High-Re solutions for incompressible flow using the Navier-Stokes equations and a multigrid method. *Journal of Computational Physics*, 48(3), 387-411.

