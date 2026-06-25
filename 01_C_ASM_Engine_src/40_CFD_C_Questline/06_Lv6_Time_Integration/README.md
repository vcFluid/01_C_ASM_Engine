# Lv.6 — Time Integration and Viscous Diffusion

目标：从离散 stability 和误差分离角度理解 timestep。总 XP：800。

| ID | Mission | Target | XP | 时间 | Prerequisite |
|---|---|---|---:|---:|---|
| L6-M01 | Upwind advection | `src/L6-M01_upwind.c` | 125 | 75m | Lv.3–4 |
| L6-M02 | CFL scan | `src/L6-M02_cfl_scan.c` | 125 | 75m | M01 |
| L6-M03 | Explicit diffusion | `src/L6-M03_diffusion_explicit.c` | 125 | 75m | Lv.3–4 |
| L6-M04 | Diffusion stability scan | `src/L6-M04_diffusion_stability.c` | 125 | 75m | M03 |
| L6-M05 | Implicit diffusion | `src/L6-M05_diffusion_implicit.c` | 175 | 120m | Lv.5、M03 |
| L6-M06 | Temporal/spatial error split | `src/L6-M06_error_split.c` | 125 | 90m | M01–M05 |

## 验收

- M01：使用 periodic 或明确的 inflow/outflow BC，报告 mass/shape change。
- M02：扫描 CFL 小于、接近和大于理论限制的 cases。
- M03：验证 1D explicit diffusion 的稳定参数 `νΔt/Δx²`。
- M04：用数值证据区分 stable、oscillatory 和 divergent。
- M05：linear solve residual 达到独立 tolerance。
- M06：固定足够细空间网格研究 temporal error，再固定足够小 `Δt` 研究 spatial error。

风险：同时改变 `Δx` 和 `Δt` 会混合误差来源，难以判断 observed order。

