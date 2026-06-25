# Lv.3 — Discrete Operators

目标：实现并用 manufactured/analytic solution 验证 CFD 基本离散算子。总 XP：900。

| ID | Mission | Target | XP | 时间 | Prerequisite |
|---|---|---|---:|---:|---|
| L3-M01 | 1D forward difference | `src/L3-M01_forward_diff.c` | 100 | 60m | Lv.2 |
| L3-M02 | 1D central difference | `src/L3-M02_central_diff.c` | 100 | 60m | M01 |
| L3-M03 | Second derivative | `src/L3-M03_second_diff.c` | 100 | 60m | M02 |
| L3-M04 | 2D gradient | `src/L3-M04_gradient.c` | 125 | 75m | M02 |
| L3-M05 | 2D divergence | `src/L3-M05_divergence.c` | 125 | 75m | M04 |
| L3-M06 | 2D Laplacian | `src/L3-M06_laplacian.c` | 150 | 90m | M03–M04 |
| L3-M07 | Grid refinement study | `src/L3-M07_refinement.c` | 200 | 90m | M01–M06 |

## 验收

- M01：对光滑解析函数观察约一阶误差趋势。
- M02：对 interior points 观察约二阶误差趋势。
- M03：明确 stencil 的 truncation error。
- M04：分别验证 x/y component。
- M05：使用解析 divergence-free velocity field，报告 divergence norm。
- M06：使用 `sin(x)sin(y)` 或等价解析函数验证。
- M07：至少使用 3 组依次减半的 grid spacing，并计算
  `p=log(e_h/e_h2)/log(2)`。

风险：单网格上的“小误差”不能证明 formal order；boundary points 可能降低 global order。

