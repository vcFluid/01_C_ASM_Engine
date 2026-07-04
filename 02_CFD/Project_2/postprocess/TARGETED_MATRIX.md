# Targeted Matrix 与误差自动化

本文件记录 `targeted_matrix.py` 的用途和输出结构。当前阶段先以 Sod
shock tube 为验证样例；其它六个内置算例后续只需要修改 `--case` 和必要的
终止时间、网格、CFL 即可复用同一流程。

## 目标

`targeted_matrix.py` 用来批量回答这些问题：

- 是否添加 artificial viscosity；
- artificial viscosity sensor 选 `rho`、`u` 还是 `p`；
- 经验系数 `beta` 如何影响误差、稳定性和振荡；
- CFL 数如何影响误差和稳定性；
- 网格数 `Nx` 如何影响误差。

误差统一在 numerical grid 上计算：

```text
L1   = integral |q_num - q_exact| dx
L2   = sqrt(integral (q_num - q_exact)^2 dx)
Linf = max |q_num - q_exact|
```

其中 exact solution 由 Project 0 外部程序生成，再插值到数值解网格。

## 默认矩阵

默认命令：

```powershell
python .\postprocess\targeted_matrix.py
```

等价于 Sod case 1：

```text
case    = 1
domain  = [0, 1], x0 = 0.5
gamma   = 1.4
t_final = 0.2
betas   = 0.05, 0.10, 0.20, 0.25, 0.50
sensors = rho, u, p
CFLs    = 0.2, 0.5, 0.8
Nx      = 101, 201, 501, 1001
```

组合数为：

```text
5 beta * 3 sensors * 3 CFL * 4 Nx = 180 viscosity-on cases
3 CFL * 4 Nx = 12 explicit viscosity-off cases
total = 192 cases
```

这个默认矩阵适合正式批量实验，不建议在只想检查流程时直接运行。

## 快速验证命令

轻量数据流程：

```powershell
python .\postprocess\targeted_matrix.py `
  --betas 0.25 `
  --sensors rho `
  --cfls 0.5 `
  --nxs 101 `
  --backend none `
  --output-dir .\runs\Solution_01_Sod\targeted_matrix_smoke
```

轻量 Tecplot 出图流程：

```powershell
python .\postprocess\targeted_matrix.py `
  --betas 0.1 0.25 `
  --sensors rho `
  --cfls 0.5 `
  --nxs 101 `
  --reference-nx 101 `
  --backend tecplot-macro `
  --output-dir .\runs\Solution_01_Sod\targeted_matrix_tecplot_smoke
```

## 正式 Sod 参数矩阵

```powershell
python .\postprocess\targeted_matrix.py `
  --case 1 `
  --output-dir .\runs\Solution_01_Sod\targeted_matrix
```

如果只想先生成 CSV 而不调用 Tecplot：

```powershell
python .\postprocess\targeted_matrix.py `
  --case 1 `
  --backend none `
  --output-dir .\runs\Solution_01_Sod\targeted_matrix
```

## 输出结构

```text
runs/Solution_01_Sod/targeted_matrix/
  targeted_matrix_config.json
  matrix_cases.csv
  errors.csv
  best.csv
  export_targeted_matrix_with_tecplot.mcr
  slices/
    beta_by_sensor.dat
    cfl_reference.dat
    grid_reference.dat
  run_0001/
    config.json
    run.log
    numerical.dat
    numerical_exact.dat
  run_0002/
    ...
```

各文件含义：

- `matrix_cases.csv`：每个参数组合一行，记录 completed、unstable 或 failed；
- `errors.csv`：每个参数组合、每个变量一行，记录 `L1/L2/Linf/TV` 等；
- `best.csv`：按 `rho/u/p` 和 `L1/L2/Linf` 自动找出误差最小组合；
- `slices/*.dat`：Tecplot 可读的误差切片数据；
- `tecplot_matrix_*.png`：Tecplot macro 导出的误差曲线图；
- `run_XXXX/config.json`：该组合的完整参数；
- `run_XXXX/run.log`：C solver 交互输出和 Project 0 exact solver 输出。

## 误差切片

当前自动生成三类 Tecplot slice：

- `beta_by_sensor.dat`：固定 `CFL = reference-cfl` 与 `Nx = reference-nx`，
  比较不同 sensor 下 error versus beta；
- `cfl_reference.dat`：固定 `beta/reference-sensor/reference-nx`，
  比较 error versus CFL；
- `grid_reference.dat`：固定 `beta/reference-sensor/reference-cfl`，
  比较 error versus Nx。

默认 reference 设置为：

```text
reference-beta   = 0.25
reference-sensor = rho
reference-cfl    = 0.5
reference-nx     = 501
```

如果轻量测试没有包含这些 reference 参数，需要显式指定对应选项，例如
`--reference-nx 101`。

## 解释注意事项

- `viscosity off` 是显式关闭 artificial viscosity；`beta=0` 不作为默认 beta
  扫描点，以免和显式 off baseline 重复。
- 如果某组参数没有推进到目标时间，脚本标记为 `unstable`，不计算误差图，但保留
  `run.log` 和原始输出。这种失稳本身就是实验结果。
- `Linf` 对 shock/contact 附近的插值和网格位置非常敏感。报告里应优先用 `L1`
  讨论整体趋势，再用 `Linf` 说明局部最大误差。
- 默认矩阵为教学验证服务，组合数不大但会反复启动 Project 0 process；正式运行前
  先用 smoke command 检查 Tecplot 和文件路径。
