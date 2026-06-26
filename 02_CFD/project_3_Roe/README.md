# Project 3 Roe Solver Skeleton

## 核心判断

Project 3 可以和 Project 2 同构，但数值格式层不能同构地照抄 MacCormack。

可以继承的骨架是：

- `solver` struct 管理全局参数、数组和方法指针；
- 主变量仍然使用守恒量 `Q = [rho, rho*u, rho*E]`；
- 每一步由 `Q -> W` 更新原始量；
- CFL 时间步仍然取决于最大特征速度 `|u| + a`；
- 初值、边界、Tecplot 输出、exact solution 对比流程可以沿用；
- 主循环仍然是 `init -> while(t < t_max) step -> write -> exact -> compare`。

必须替换的核心是：

- Project 2: cell physical flux `F_i = F(Q_i)` + MacCormack predictor/corrector；
- Project 3: interface numerical flux `Fhat_{i+1/2}` + finite-volume conservative update。

所以 Project 3 的“血肉”主要集中在三个函数：

```text
compute_physical_flux()
compute_roe_flux()
step_roe()
```

## 和 Project 2 的对应关系

```text
Project 2 MacCormack                  Project 3 Roe
---------------------------------------------------------------
struct Riemann_1D_MacC_solver          struct Riemann_1D_Roe_solver
q1/q2/q3                               q1/q2/q3
q1_bar/q2_bar/q3_bar                   not needed
q1_next/q2_next/q3_next                q1_next/q2_next/q3_next
f1/f2/f3 cell flux                     optional physical flux helper
f1_bar/f2_bar/f3_bar                   not needed
                                       fhat1/fhat2/fhat3 interface flux
compute_flux(Q_i)                      compute_physical_flux(Q)
step_maccormack()                      step_roe()
artificial viscosity                   usually off for Roe baseline
exact solver process                   reusable
Tecplot output                         reusable
postprocess scripts                    reusable after path/name update
```

## Roe update formula

For 1-D Euler equations:

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

where the Roe-averaged quantities are computed from the left and right states.

## 推荐实现顺序

1. 先完成 `pressure_from_q()`、`total_energy_density()`、`compute_physical_flux()`。
2. 完成 `compute_dt()`，确认 `dt = CFL * dx / max(|u| + a)`。
3. 写 `compute_roe_flux()`，只处理一个 interface。
4. 写 `compute_all_roe_fluxes()`，循环处理所有 `i+1/2`。
5. 写 `step_roe()`，用左右界面通量更新 cell average。
6. 加 `entropy_fix()`，避免 transonic rarefaction 里的 entropy violation。
7. 加 positivity check，至少检测 `rho <= 0` 或 `p <= 0` 后停止并输出诊断。
8. 先跑 Sod，再跑 Lax，再考虑双膨胀、近真空等更难算例。

## 常见风险

- Roe 格式不是简单把 `compute_flux()` 换成 Roe。Roe flux 位于 interface，不位于 cell center。
- Roe 本身有 upwind dissipation，baseline 不建议再叠 Project 2 的人工粘性，否则讨论不干净。
- Roe 不保证 positivity。强膨胀或近真空问题可能出现负密度/负压强。
- 不加 entropy fix 时，跨声速稀疏波可能出现非物理解。
- 精确解对比时要保证 `xmin/xmax/x0/t/gamma/nx` 完全一致。

## 文件说明

```text
project_3_Roe/
  README.md
  1-D_Riemann_Roe_PSEUDOCODE.c
  postprocess/README.md
  runs/.gitignore
```

当前 C 文件是伪代码骨架，不是最终可编译版本。你可以按里面的 `TODO(Project 3)` 逐段填实。
