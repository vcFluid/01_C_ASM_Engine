# Project 3 Postprocess Notes

Project 3 can reuse the Project 2 postprocess logic if the numerical output keeps the same Tecplot variables:

```text
"x", "rho", "u", "p", "E", "q1", "q2", "q3"
```

The comparison scripts only need numerical and exact files with compatible `x, rho, u, p` fields.

Recommended workflow:

```text
1. Run Roe solver -> runs/case_01_roe_numerical.dat
2. Run exact solver at same t -> runs/case_01_roe_numerical_exact.dat
3. Use Project 2 postprocess script or copy it here after the Roe solver is real
4. Compare rho/u/p and compute L1, L2, Linf
```

Keep Project 2 scripts unchanged until Project 3's solver output is stable.
