# Project 2 TODO

## 当前原则

本阶段不再新增求解器功能。代码侧只保留必要的图像样式修正、文档同步和 bug fix。

后续主要工作为运行数据，并将结果整理进报告。

## 作业目标

用 MacCormack 格式求解 1-D Riemann 问题，并与 Project 0 半解析解对比。

需要覆盖：

1. Sod shock tube；
2. Lax shock tube；
3. subsonic double-expansion；
4. Sjogreen supersonic expansion；
5. contact discontinuity with double expansion；
6. contact discontinuity with double shock；
7. pure contact discontinuity。

其中 Sod 是流程验证和参数讨论的核心样例。

## 已完成的代码能力

- [x] 7 种 Riemann 初值条件已内置。
- [x] MacCormack solver 已实现。
- [x] 主推进变量为守恒量 `Q = [rho, rho*u, rho*E]`。
- [x] 原始量 `W = [rho, u, p]` 每步由 `Q` 同步反解。
- [x] 用户可设置计算域、间断位置、网格数、`gamma`、CFL、终止时间。
- [x] 支持 artificial viscosity on/off。
- [x] 支持 artificial viscosity 经验系数 `beta`。
- [x] 支持 sensor=`rho/u/p`。
- [x] Project 2 可通过 OS process 调用 Project 0 exact solver。
- [x] Project 0 batch mode 可接收相同左右状态、计算域、网格、`x0`、`gamma` 和输出时刻。
- [x] numerical/exact 输出均为 Tecplot ASCII 数据。
- [x] 后处理脚本可读取数据、对齐变量、计算误差。
- [x] Tecplot macro batch mode 可自动导出 PNG。
- [x] snapshot + exact snapshot 可生成时间动画。
- [x] `beta_sweep.py` 可扫描 `beta`。
- [x] `targeted_matrix.py` 可围绕 Sod 自动组织 viscosity、`beta`、sensor、CFL、Nx 实验。
- [x] `beta_slider.py` 可用 Python slider 浏览已有 beta sweep 数据。
- [x] 出图样式已调整为报告友好：exact 细线、numerical 小点细线、图例和误差图线宽统一。

## 报告待完成

- [ ] Code Report：同步说明 Project 0 process coupling、Tecplot macro batch、后处理职责划分。
- [ ] Method Report：说明 MacCormack 格式、CFL、artificial viscosity 和守恒量推进逻辑。
- [ ] Result Report：填入正式运行后的图、表和讨论。
- [ ] 在结果报告中说明 `L1/L2/Linf` 的定义：当前采用数值网格上的积分型误差，不除以域长。
- [ ] 对 `Linf` 在间断附近的敏感性做文字说明。
