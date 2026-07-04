# Project 2 TODO

## 当前原则

本阶段不再新增求解器功能。代码侧只保留必要的图像样式修正、文档同步和 bug fix。

后续主要工作由用户亲自运行数据，并将结果整理进报告。

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

## Sod 待运行实验

这些实验是 `targeted_matrix.py` 和 `beta_sweep.py` 的用途，不是新增物理目标。

- [ ] numerical vs exact baseline。
- [ ] artificial viscosity on/off 对比。
- [ ] `beta` sensitivity。
- [ ] CFL sensitivity。
- [ ] sensor=`rho/u/p` comparison。
- [ ] no-viscosity + grid refinement：检查单纯加密网格是否能改善色散振荡。
- [ ] 选出报告中使用的 `rho/u/p` 对比图。
- [ ] 选出报告中使用的误差表或误差曲线。

## 其余算例待运行实验

每个算例至少完成：

- [ ] numerical vs exact baseline；
- [ ] 判断是否需要 artificial viscosity；
- [ ] 给出一组可接受的 `beta`；
- [ ] 记录是否稳定推进到目标时间；
- [ ] 记录主要数值现象：振荡、扩散、间断捕捉、负密度/负压等。

具体算例：

- [ ] case 2: Lax shock tube。
- [ ] case 3: subsonic double-expansion。
- [ ] case 4: Sjogreen supersonic expansion。
- [ ] case 5: contact discontinuity with double expansion。
- [ ] case 6: contact discontinuity with double shock。
- [ ] case 7: pure contact discontinuity。

Sjogreen case 需要特别检查：

- [ ] 是否出现真空区或近真空区；
- [ ] Project 0 exact solution 是否仍给出有效参考；
- [ ] Project 2 是否出现负密度、负压或提前失稳；
- [ ] 若失败，将其作为 MacCormack + artificial viscosity 的适用性边界讨论。

## 图像与后处理

- [x] 单算例 numerical/exact 对比图样式已优化。
- [x] beta sweep profile/error 图样式已优化。
- [x] targeted matrix error 图样式已优化。
- [x] beta slider 预览样式已优化。
- [ ] 用户运行正式数据后，人工检查图像是否需要局部放大。
- [ ] 最终报告图需要统一文件名、标题、图例和变量顺序。
- [ ] 如报告需要，再从 Tecplot 导出 PDF/EPS；当前默认导出 PNG。

## 报告待完成

- [ ] Code Report：同步说明 Project 0 process coupling、Tecplot macro batch、后处理职责划分。
- [ ] Method Report：说明 MacCormack 格式、CFL、artificial viscosity 和守恒量推进逻辑。
- [ ] Result Report：填入正式运行后的图、表和讨论。
- [ ] 在结果报告中说明 `L1/L2/Linf` 的定义：当前采用数值网格上的积分型误差，不除以域长。
- [ ] 对 `Linf` 在间断附近的敏感性做文字说明。

## 暂不做

- [ ] 不新增 C solver 的新功能。
- [ ] 不新增数值厚度指标。
- [ ] 不把 PyTecplot/TecPLUS connection 作为当前依赖。
- [ ] 不把 targeted matrix 当作报告中的独立物理目标；它只是自动化运行工具。
## 结果集目录命名约定

正式结果集统一命名为 `runs/Solution_XX_CaseName/`，例如 `Solution_01_Sod`、`Solution_02_LAX`。后续新跑的数据也按这个格式组织。
