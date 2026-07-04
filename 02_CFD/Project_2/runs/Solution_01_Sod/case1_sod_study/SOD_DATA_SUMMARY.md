# Case 1 Sod 数据汇总

本目录保存 Sod shock tube 的第一轮正式数据。当前只整理数据，不直接填入总报告。

## 1. 目录结构

```text
runs/Solution_01_Sod/case1_sod_study/
  baseline/
    numerical.dat
    numerical_exact.dat
    plots/rho.png
    plots/u.png
    plots/p.png
  beta_sweep_profiles/
    errors.csv
    beta_profiles.dat
    beta_errors.dat
  matrix/
    matrix_cases.csv
    errors.csv
    best.csv
    run_XXXX/
```

`baseline` 用于数值解和精确解对比图；`beta_sweep_profiles` 用于画不同 beta 的多曲线对比；`matrix` 用于统计 beta、sensor、CFL、Nx 和是否加人工粘性的影响。

## 2. 基准算例

基准设置：

```text
case   = 1, Sod
domain = [0, 1], x0 = 0.5
Nx     = 501
CFL    = 0.5
t      = 0.2
gamma  = 1.4
mode   = 1, predictor FTBS / corrector FTFS
sensor = rho
beta   = 0.5
```

基准误差：

| variable | L1 | L2 | Linf |
|---|---:|---:|---:|
| rho | 4.117286e-3 | 1.626706e-2 | 1.405010e-1 |
| u | 1.160388e-2 | 8.886585e-2 | 9.268365e-1 |
| p | 3.580555e-3 | 2.035825e-2 | 2.030486e-1 |

说明：速度的 Linf 明显较大，主要来自激波和接触间断附近的局部位置误差。含间断问题中，Linf 对波位置非常敏感，报告中应优先用 L1 讨论整体趋势。

## 3. 参数矩阵范围

本次矩阵共 192 组：

```text
beta   = 0.25, 0.5, 0.75, 1.0, 1.5
sensor = rho, u, p
CFL    = 0.2, 0.5, 0.8
Nx     = 101, 201, 501, 1001
viscosity off baseline included
```

运行结果：

| status | count |
|---|---:|
| completed | 96 |
| failed | 96 |

失败组不是无效数据。它们说明该参数组合下 MacCormack 推进产生了非物理状态，通常是负压或 NaN 前的保护性停止。

## 4. beta 的影响

固定 `sensor=rho, CFL=0.5, Nx=501`，只改变 beta：

| beta | status | rho L1 | rho L2 | rho Linf |
|---:|---|---:|---:|---:|
| 0.25 | failed | - | - | - |
| 0.50 | completed | 4.117286e-3 | 1.626706e-2 | 1.405010e-1 |
| 0.75 | completed | 4.599238e-3 | 1.730164e-2 | 1.405211e-1 |
| 1.00 | completed | 4.906314e-3 | 1.768589e-2 | 1.404999e-1 |
| 1.50 | completed | 5.286209e-3 | 1.766629e-2 | 1.403473e-1 |

在这组条件下，beta 太小会失稳；beta 增大后可以稳定，但整体 L1 误差上升，说明间断被进一步抹宽。这里 beta=0.5 是比较稳妥的基准值。

## 5. CFL 的影响

固定 `beta=0.5, sensor=rho, Nx=501`：

| CFL | CFL(1-CFL) | status | rho L1 | rho L2 | rho Linf |
|---:|---:|---|---:|---:|---:|
| 0.2 | 0.16 | completed | 4.803853e-3 | 1.807340e-2 | 1.405324e-1 |
| 0.5 | 0.25 | completed | 4.117286e-3 | 1.626706e-2 | 1.405010e-1 |
| 0.8 | 0.16 | failed | - | - | - |

人工粘性系数中有 `CFL(1-CFL)` 因子，它对 CFL 不是单调函数，而是在 CFL=0.5 附近最大。CFL=0.2 和 CFL=0.8 的该因子相同，但 CFL=0.8 仍失败，说明稳定性不只由人工粘性大小决定，还受到时间步长本身影响。

## 6. sensor 的影响

固定 `beta=0.5, CFL=0.5, Nx=501`：

| sensor | status | rho L1 | rho L2 | rho Linf |
|---|---|---:|---:|---:|
| rho | completed | 4.117286e-3 | 1.626706e-2 | 1.405010e-1 |
| u | failed | - | - | - |
| p | completed | 4.024930e-3 | 1.589984e-2 | 1.404865e-1 |

在这个 Sod 算例中，density sensor 和 pressure sensor 更稳定，velocity sensor 在该条件下失败。原因可能是速度场对间断位置的响应方式不同，用速度作为开关函数时没有稳定覆盖需要加粘性的区域。

## 7. 网格加密的影响

固定 `beta=0.5, sensor=rho, CFL=0.5`：

| Nx | status | rho L1 | rho L2 | rho Linf |
|---:|---|---:|---:|---:|
| 101 | completed | 1.079602e-2 | 2.256800e-2 | 1.205454e-1 |
| 201 | completed | 6.751761e-3 | 1.928913e-2 | 1.346955e-1 |
| 501 | completed | 4.117286e-3 | 1.626706e-2 | 1.405010e-1 |
| 1001 | completed | 3.227975e-3 | 1.544320e-2 | 1.405737e-1 |

L1 随网格加密下降，说明整体误差改善；但 Linf 没有单调下降，因为最大误差主要受激波/接触间断附近的局部位置误差控制。

## 8. 无人工粘性 + 加密网格

关闭人工粘性时：

| CFL | Nx=101 | Nx=201 | Nx=501 | Nx=1001 |
|---:|---|---|---|---|
| 0.2 | failed | failed | failed | failed |
| 0.5 | failed | failed | failed | failed |
| 0.8 | failed | failed | failed | failed |

结论：对 Sod 激波管，单纯加密网格不能解决 MacCormack 格式在激波附近的色散和非物理状态问题。色散/振荡是格式处理间断时的固有问题，不能只靠减小 dx 消除。

## 9. 当前可写入报告的判断

1. Sod 问题必须加入人工粘性，否则当前 MacCormack 程序无法稳定推进到 t=0.2。
2. beta 不是越大越好。beta 太小会失稳，beta 太大则增强扩散，抹宽激波和接触间断。
3. CFL 对稳定性和人工粘性都有影响，但人工粘性因子 `CFL(1-CFL)` 本身不是 CFL 的单调函数。
4. density sensor 与 pressure sensor 表现较稳，velocity sensor 在基准条件下失败。
5. 加密网格可以降低开启人工粘性时的整体 L1 误差，但关闭人工粘性时，加密网格并不能保证稳定。
