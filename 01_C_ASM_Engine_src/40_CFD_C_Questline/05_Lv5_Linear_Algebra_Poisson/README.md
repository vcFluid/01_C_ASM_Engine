# Lv.5 — Linear Algebra and Poisson

目标：建立 matrix-free Poisson solver，并区分 iteration、algebraic 与 discretization error。总 XP：1050。

| ID | Mission | Target | XP | 时间 | Prerequisite |
|---|---|---|---:|---:|---|
| L5-M01 | Vector kernels | `src/L5-M01_vector_ops.c` | 100 | 60m | Lv.2 |
| L5-M02 | Dense matvec interface | `src/L5-M02_dense_matvec.c` | 100 | 60m | M01 |
| L5-M03 | Matrix-free Poisson operator | `src/L5-M03_poisson_operator.c` | 125 | 75m | Lv.3–4 |
| L5-M04 | Jacobi solver | `src/L5-M04_jacobi.c` | 125 | 75m | M03 |
| L5-M05 | Gauss–Seidel solver | `src/L5-M05_gauss_seidel.c` | 125 | 75m | M04 |
| L5-M06 | Residual monitor | `src/L5-M06_residual.c` | 125 | 75m | M04–M05 |
| L5-M07 | Manufactured Poisson test | `src/L5-M07_poisson_verify.c` | 200 | 120m | M03–M06 |
| L5-M08 | Elimination and fill-in | `notes/L5-M08_fill_in.md` | 150 | 90m | M02 |

## 验收

- M01：copy、scale、AXPY、dot、L2 norm 均有独立测试。
- M02：验证 dimensions；理解接口后停止扩展 dense CFD solver。
- M03：5-point stencil 与 boundary policy 分离。
- M04–M05：相同问题、相同 initial guess、相同 stopping criterion 下比较收敛。
- M06：直接计算 `r=b-Ax`；不得仅用 consecutive solution change 替代 residual。
- M07：报告 residual history、solution error 和 grid refinement。
- M08：画出 elimination 前后 graph，解释 fill-in 与 sparse storage 成本。

已有参考：[`../../20_Numerical_Logic/youxianyuan.c`](../../20_Numerical_Logic/youxianyuan.c)。

