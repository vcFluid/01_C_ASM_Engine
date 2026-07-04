# Project 3 Roe Solver Skeleton

## 核心判断

Project 3 应该保持和当前 Project 2 同构的程序架构，但数值方法核心必须更换。

Project 2 中可以复用的架构是：

- `solver` struct 负责管理全局参数、数组和方法指针。
- 守恒变量 `Q = [rho, rho*u, rho*E]` 仍然作为主状态变量。
- 原始变量 `W = [rho, u, p]` 每一步都从 `Q` 同步反算。
- CFL 时间步仍然使用谱半径 `|u| + a`。
- 7 个内置 Riemann 算例仍然在内存分配前选择。
- solver 仍然输出包含 `x, rho, u, p, E, q1, q2, q3` 的 Tecplot ASCII 数据。
- Project 0 exact solver 仍然作为外部 batch process 调用，并传入相同的左右状态、计算域、网格、`gamma` 和时间。
- numerical snapshots 和 exact snapshots 可以配对生成动画。
- 后处理脚本读取 numerical/exact Tecplot 文件，并计算 `L1/L2/Linf`。

Project 2 中不能直接复用的核心是：

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

因此 Project 3 真正需要实现的工作集中在：

```text
physical_flux_from_q()
roe_flux()
compute_all_roe_fluxes()
step_roe()
check_physical_state()
```

## 当前 Project 2 到 Project 3 的映射

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

## Roe 公式

对于一维 Euler 方程：

```text
dQ/dt + dF(Q)/dx = 0
Q = [rho, rho*u, rho*E]
F = [rho*u, rho*u*u + p, u*(rho*E + p)]
```

Roe 有限体积更新为：

```text
Q_i^{n+1} = Q_i^n - dt/dx * (Fhat_{i+1/2} - Fhat_{i-1/2})
```

Roe 界面通量为：

```text
Fhat_{i+1/2}
  = 0.5 * (F_L + F_R)
  - 0.5 * sum_k abs(lambda_tilde_k) * alpha_k * r_tilde_k
```

## 推荐实现顺序

1. 先迁移 Project 2 的骨架，但去掉 MacCormack 的 predictor 数组。
2. 先保持相同的输入/输出接口。
3. 实现 `pressure_from_q`、`total_energy_density`、`primitive_from_q` 和 `physical_flux_from_q`。
4. 在 `roe_flux` 中实现 Roe 平均量、特征值、波强度和右特征向量。
5. 实现 `compute_all_roe_fluxes`。
6. 实现 `step_roe`。
7. 加入 entropy fix。
8. 加入针对 `rho <= 0`、`p <= 0`、`NaN` 和 `Inf` 的物理状态诊断。
9. 复用 exact comparison 和 Tecplot 输出。
10. 只有在 Sod 算例稳定后，再创建 Roe 版本的 sweep/matrix 脚本。

## Roe 专属实验参数轴

Project 2 的正式参数研究围绕 artificial viscosity：

```text
viscosity on/off, beta, sensor, CFL, Nx
```

对于 Project 3 Roe，更干净的参数研究应改为：

```text
entropy_fix on/off
entropy_delta or entropy_factor
CFL
Nx
case_id
positivity failure/stability status
```

不要在报告中把 Roe 的 entropy fix 称为 `"artificial viscosity beta"`。二者都可能带来稳定化/耗散效果，但它们进入数值格式的位置不同。

## 风险

- Roe flux 是界面通量，不能通过简单替换 Project 2 的 `compute_flux()` 实现。
- 不加 entropy fix 的 Roe 可能在跨声速稀疏波中产生违反熵条件的 expansion shock。
- Roe 不保证 positivity。即使 Sod 可以正常运行，强膨胀和近真空算例仍可能失败。
- shock/contact 附近的 `Linf` 对网格对齐很敏感。整体趋势讨论应优先使用 `L1`。
- 如果输出变量或文件命名偏离 Project 2，后处理 pipeline 会断。

## 文件

```text
project_3_Roe/
  README.md
  1-D_Riemann_Roe_PSEUDOCODE.c
  postprocess/README.md
  runs/.gitignore
```

当前 C 文件仍然是伪代码。它是从当前 Project 2 映射到 Roe 实现计划的说明，不是可编译 solver。
