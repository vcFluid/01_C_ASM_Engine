# Lv.5 — PDE Prototypes

| ID | Mission | 核心能力 | XP |
|---|---|---|---:|
| PY5-M01 | 1D linear advection | CFL 与数值耗散 | 125 |
| PY5-M02 | 1D diffusion | Fourier number 与 stability | 125 |
| PY5-M03 | 1D Burgers equation | nonlinear flux | 150 |
| PY5-M04 | 2D Poisson Jacobi/Gauss-Seidel | residual 与 algebraic error | 175 |
| PY5-M05 | sparse Poisson with SciPy | CSR 与 iterative solver | 175 |
| PY5-M06 | projection-method prototype | discrete divergence control | 200 |
| PY5-M07 | cavity verification notebook/script | benchmark 与 grid dependence | 150 |

原则：Python prototype 用于验证数学与数据布局；确认后再把稳定 API 和 kernel 迁移到 C/C++。

