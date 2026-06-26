# Project 2 TODO

## 当前目标

先完整跑通 Sod shock tube，验证 MacCormack solver、精确解、误差计算、
Tecplot 批量绘图和动画输出组成的后处理流程。

## 任务边界

- C solver 只负责计算并输出守恒量、原始量和时间信息。
- Project 0 exact solver 负责生成同一初值、同一时刻下的参考解。
- 独立 post-processing 程序负责数据对齐、误差计算、绘图任务生成和批处理。
- Tecplot 负责最终曲线、布局、静态图和动画渲染，不负责误差计算。

## 数据对齐要求

- [ ] 数值解和精确解必须使用相同的物理坐标系。
- [ ] 统一 `xmin`、`xmax`、`x0`、`t` 和 `gamma`。
- [ ] 统一变量含义和名称：`x, rho, u, p`。
- [ ] 精确解应覆盖数值计算域，但不应反向扩大数值图的显示范围。
- [ ] 将精确解插值到数值网格点后计算误差。
- [ ] 间断点附近需说明插值和点值误差对 `L_inf` 的敏感性。

## 第一个验证算例：Sod

基准配置：

- `domain = [0, 1]`
- `x0 = 0.5`
- `gamma = 1.4`
- `Nx = 501`
- `CFL = 0.5`
- `t_final = 0.2`
- left state: `(rho, u, p) = (1.0, 0.0, 1.0)`
- right state: `(rho, u, p) = (0.125, 0.0, 0.1)`

执行顺序：

- [x] 编译并运行 MacCormack solver，生成最终数值解。
- [x] 运行 exact solver，生成 `t = 0.2` 的参考解。
- [ ] 将 exact solution 转换到 `[0,1]` 且间断位置为 `x0 = 0.5`。
- [x] 检查两份数据的时刻、坐标域和初值是否完全一致。
- [x] 计算 `rho/u/p` 的 `L1`、`L2` 和 `L_inf` 误差。
- [x] 输出叠加曲线：exact solution 为背景线，numerical solution 为点或前景线。
- [x] 保存本次运行参数、误差表和图片。

当前输出：

```text
runs/sod_baseline/numerical.dat
runs/sod_baseline/exact.dat
```

Sod exact solver 验证值：

```text
p_star = 0.303130
u_star = 0.927453
```

## 自动化方案

### 第一阶段：可重复的单算例流水线

- [x] 新建 post-processing 脚本，优先使用 Python。
- [ ] 用配置文件或命令行参数描述算例，避免反复交互输入。
- [x] Project 2 通过 OS process 自动运行 exact solver。
- [x] 自动读取 Tecplot ASCII `.dat`。
- [x] 自动完成同网格检查、必要插值和误差计算。
- [x] 自动生成 Tecplot macro。
- [x] 调用 `tec360.exe` batch mode 导出 PNG。
- [x] 将运行日志、数据、误差和图片保存到独立 case 目录。

建议目录：

```text
runs/
  sod_baseline/
    config.json
    numerical.dat
    exact.dat
    errors.csv
    plot.mcr
    rho.png
    u.png
    p.png
```

### 第二阶段：参数扫描

- [ ] artificial viscosity: on/off。
- [ ] sensor: `rho/u/p`。
- [x] viscosity coefficient `beta`。
- [ ] CFL: `0.2/0.5/0.8`。
- [ ] grid: `Nx = 101/201/501/1001`。
- [x] 汇总误差、稳定性、total variation、undershoot 和 overshoot。
- [ ] 定义并汇总接触间断与激波的数值厚度。

## Tecplot 自动化

本机已有：

```text
tec360.exe
preplot.exe
PyTecplot 1.6.2
```

计划使用 Tecplot macro/batch mode：

- [x] 自动加载 numerical 和 exact 两个 zone。
- [x] 按变量分别生成 `rho/u/p` 三张图。
- [x] exact zone 使用连续实线。
- [x] numerical zone 使用 symbols 和不同颜色的线。
- [x] 坐标范围由配置中的数值计算域控制，不由 exact 数据范围自动决定。
- [x] 固定画布尺寸、线宽、图例和输出分辨率。
- [ ] 批量导出 PNG；最终报告需要时再导出 PDF/EPS。

当前状态：

- [x] 已验证本机可以导入 `tecplot` Python package。
- [ ] PyTecplot batch engine 暂不可用：当前许可证返回
      `TecplotLicenseError: License Expired`。
- [x] 传统 Tecplot `.mcr` batch mode 可使用基础 Tecplot 许可证，不依赖 TecPLUS。
- [x] 已通过 `tec360.exe -b export_with_tecplot.mcr` 自动导出 `rho/u/p` 图片。
- [x] 在 Tecplot 不可用期间，使用 Python/Matplotlib 完成正式静态图，同时继续输出
      Tecplot-compatible 数据文件。
- [x] 使用 Tecplot transient dataset 和 macro 完成动画输出。

## 动画

数值解动画：

- [x] solver 定期输出 snapshots。
- [x] 每个 snapshot 对应一个时间帧。
- [x] Tecplot 将各帧导出为 AVI。

精确解动画：

- [x] exact solver 对每个 snapshot 时间重新生成参考解。
- [ ] 或利用 Riemann self-similar solution `W(x,t) = W((x-x0)/t)` 批量采样。
- [x] 每个数值帧匹配同一时刻的 exact frame。
- [x] `t = 0` 单独使用初始分段常数状态，避免除以零。

数值解与精确解可以在同一动画中叠加。固定坐标轴范围，避免每帧自动缩放造成
视觉上的虚假运动或振幅变化。

## 误差定义

在 numerical grid 上计算：

```text
L1   = sum_i |q_num[i] - q_exact[i]| * dx
L2   = sqrt(sum_i (q_num[i] - q_exact[i])^2 * dx)
Linf = max_i |q_num[i] - q_exact[i]|
```

- [ ] 明确使用积分型误差还是除以域长后的平均误差。
- [ ] 对 `rho/u/p` 分别报告。
- [ ] 网格收敛分析优先使用 `L1`；间断问题整体收敛阶通常低于光滑区形式阶数。

## 当前已知问题

- [ ] Project 0 当前输出域约为 `[-22.47, 22.47]`，不能直接与 `[0,1]` 数值域叠加。
- [x] Sod `t=0.2` 实际 exact 输出域为约 `[-1.704489, 1.704489]`，仍需裁剪或重采样到 `[0,1]`。
- [ ] Project 0 当前输出文件可能被不同算例覆盖，需要显式指定输出文件名。
- [ ] 两个 solver 都偏交互式，不利于 batch run；后续需增加 non-interactive 参数入口，
      或由 wrapper 稳定地提供 stdin。
- [ ] Result Report 仍是占位模板，待流水线生成正式结果后填写。
- [ ] 检查并统一 Method Report、README 和代码中的算例参数。
