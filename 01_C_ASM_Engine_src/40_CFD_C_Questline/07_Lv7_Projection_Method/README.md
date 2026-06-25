# Lv.7 — Incompressible Projection Method

目标：实现满足离散 continuity 的二维 incompressible viscous solver。总 XP：1300。

| ID | Mission | Target | XP | 时间 | Prerequisite |
|---|---|---|---:|---:|---|
| L7-M01 | Staggered-grid indexing | `src/L7-M01_staggered_grid.c` | 125 | 90m | Lv.2–4 |
| L7-M02 | Tentative velocity | `src/L7-M02_tentative_velocity.c` | 150 | 120m | Lv.6、M01 |
| L7-M03 | Derive pressure Poisson equation | `notes/L7-M03_pressure_poisson.md` | 150 | 90m | M01–M02 |
| L7-M04 | Pressure correction solve | `src/L7-M04_pressure_solve.c` | 175 | 120m | Lv.5、M03 |
| L7-M05 | Velocity correction | `src/L7-M05_velocity_correct.c` | 150 | 90m | M04 |
| L7-M06 | Divergence and pressure gauge | `src/L7-M06_constraints.c` | 150 | 90m | M04–M05 |
| L7-M07 | Minimal lid-driven cavity | `src/L7-M07_cavity.c` | 225 | 180m | M01–M06 |
| L7-M08 | Cavity verification | `notes/L7-M08_cavity_verify.md` | 175 | 120m | M07 |

## 验收

- M01：记录 `u(i,j)`、`v(i,j)`、`p(i,j)` 的位置、shape 和有效索引。
- M02：advection、diffusion、forcing 与 BC 的离散形式可追溯。
- M03：从 discrete continuity 推导，不直接照抄连续方程。
- M04：处理 Poisson BC、compatibility、null space 和 reference pressure。
- M05：pressure gradient 与 divergence operator 保持离散一致。
- M06：报告 correction 前后 divergence norms。
- M07：先做低 Reynolds number 和粗网格正确性，不先优化性能。
- M08：比较 centerline velocity、mass conservation、steady criterion 和 grid dependence。

核心风险：若 gradient/divergence pair 不一致，即使 Poisson residual 很小，也可能无法得到足够 divergence-free 的速度场。

