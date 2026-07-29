# Project 3: 1-D Riemann Problem With Roe Scheme

本项目在 Project 2 的程序组织基础上，改写为一维 Euler 方程的 cell-centered finite-volume Roe solver，并与 exact Riemann solution 对比。

## 数值方法

一维 Euler 方程写成守恒形式：

```text
dQ/dt + dF(Q)/dx = 0
Q = [rho, rho*u, rho*E]
F = [rho*u, rho*u*u + p, u*(rho*E + p)]
```

本项目使用一阶 Godunov 型 FVM：

```text
Q_j^{n+1} = Q_j^n - dt/dx * (Fhat_{j+1/2} - Fhat_{j-1/2})
```

界面数值通量采用课件中的 Roe 近似：

```text
Fhat_{j+1/2}
  = 0.5 * (F(Q_L) + F(Q_R))
  - 0.5 * |A_tilde| * (Q_R - Q_L)
```

其中 `Q_L, Q_R` 来自界面两侧 cell average 的分段常数重构。代码按课件给出的 Roe average 和 `alpha1` 到 `alpha5` 展开式计算耗散项。

## 与 Project 2 的关系

Project 2 的外层骨架可以复用：

- 交互式选择 7 个 Riemann cases。
- 使用守恒变量 `Q = [rho, rho*u, rho*E]` 作为主状态。
- 每一步从守恒变量反算 `rho, u, p, a`。
- CFL 时间步使用最大波速 `|u| + a`。
- 输出 Tecplot ASCII 文件。
- 调用已有 exact solver 生成同一时刻的精确解。
- 后处理读取 numerical/exact 数据并计算 `L1/L2/Linf`。

但数值迭代核心不能直接替换函数名完成：

```text
Project 2 MacCormack:
    cell physical flux F_i = F(Q_i)
    predictor Q_bar
    corrector Q_next
    optional artificial viscosity

Project 3 Roe FVM:
    interface numerical flux Fhat_{i+1/2}
    conservative update by flux difference
    physical-state diagnostics
```

## 主要代码入口

```text
1-D_Riemann_Roe.c
    pressure_from_conserved()
    primitive_from_conserved()
    physical_flux_from_conserved()
    compute_roe_flux()
    compute_all_roe_fluxes()
    advance_one_roe_step()
    run_exact_solver()
    write_tecplot()
```

后处理脚本位于：

```text
postprocess/
    tecplot_compare.py
    tecplot_animate.py
    targeted_matrix.py
    run_report_matrix.py
    report_plots.py
```

## 编译与运行

```powershell
gcc .\1-D_Riemann_Roe.c -std=c11 -O2 -Wall -Wextra -o .\riemann_roe.exe -lm
.\riemann_roe.exe
```

批量运行七组报告数据：

```powershell
python .\postprocess\run_report_matrix.py --all
python .\postprocess\report_plots.py --results-dir .\runs\ppt_formula_exact_only
```

## 输出

每个 run 会生成：

```text
numerical.dat
numerical_exact.dat
run.log
config.json
```

报告级聚合结果：

```text
runs/ppt_formula_exact_only/
    baseline_exact_comparison.csv
    overall_results.csv
    report_figures/
        baseline_l1_errors.png
        baseline_linf_errors.png
```

## 注意点

- Roe flux 是界面数值通量，不能直接等同于 Project 2 中存于 cell/node 的物理通量。
- 当前实现是一阶分段常数 FVM，shock/contact 附近会有明显数值耗散。
- Roe 原式不保证 positivity；强膨胀或近真空算例可能出现负密度或负压力，此时程序会停止并记录为 unstable。
- `Linf` 对间断位置和网格对齐很敏感；讨论整体误差趋势时优先看 `L1`。
