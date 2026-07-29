# Project 3 Roe 后处理流程

本目录类比 Project 2 的后处理组织方式，但当前只服务于 Roe 原式数值解与 exact Riemann solution 的对比。

## 主线流程

```text
Roe solver
-> numerical.dat + numerical_exact.dat
-> tecplot_compare.py
-> targeted_matrix.py / run_report_matrix.py
-> report_plots.py
```

## 输出契约

Roe solver 的 Tecplot ASCII 输出至少包含：

```text
"x", "rho", "u", "p", "E", "rho_conserved", "rhou", "rhoE"
```

后处理脚本只依赖：

```text
x, rho, u, p
```

Roe 数值解的 `x` 是 cell center；exact solver 可能输出另一套采样点。误差统计会先把 exact 解插值到数值网格上。

## 单算例对比

先运行 solver 得到一组 numerical/exact 文件：

```powershell
.\riemann_roe.exe
```

然后调用：

```powershell
python .\postprocess\tecplot_compare.py `
  --numerical .\runs\case_01_roe_numerical.dat `
  --exact .\runs\case_01_roe_numerical_exact.dat `
  --output-dir .\runs\case_01_roe_plots `
  --backend tecplot-macro
```

输出：

```text
numerical_exact_combined.dat
rho.png
u.png
p.png
export_with_tecplot.mcr
```

## 批处理矩阵

`targeted_matrix.py` 的参数轴只有：

```text
case
CFL
Nx
```

快速运行：

```powershell
python .\postprocess\targeted_matrix.py `
  --case 1 `
  --cfls 0.5 `
  --nxs 501 `
  --output-dir .\runs\_roe_matrix_smoke `
  --backend none
```

输出目录：

```text
runs/_roe_matrix_smoke/
    matrix_results.csv
    matrix_errors.dat
    matrix_config.json
    SUMMARY.md
    roe_cfl_0p5_nx_501/
        config.json
        run.log
        numerical.dat
        numerical_exact.dat
```

## 七组报告数据

默认报告线为 `CFL=0.5, Nx=501`：

```powershell
python .\postprocess\run_report_matrix.py --all
```

如果只检查命令：

```powershell
python .\postprocess\run_report_matrix.py --all --dry-run
```

也可以做纯 CFL/Nx 敏感性检查：

```powershell
python .\postprocess\run_report_matrix.py --all --cfls 0.2 0.5 0.8 --nxs 101 201 501
```

聚合文件：

```text
runs/ppt_formula_exact_only/
    overall_results.csv
    baseline_exact_comparison.csv
```

生成报告图：

```powershell
python .\postprocess\report_plots.py --results-dir .\runs\ppt_formula_exact_only
```

## 误差定义

沿用 Project 2：

```text
L1   = integral |q_num - q_exact| dx
L2   = sqrt(integral (q_num - q_exact)^2 dx)
Linf = max |q_num - q_exact|
```

同时输出：

```text
min/max
TV
undershoot
overshoot
status = completed / unstable / failed
```

`unstable` 表示 solver 检测到非物理状态或输出不完整。Roe 原式不保证 positivity，所以强膨胀算例失败本身也是结果讨论的一部分。
