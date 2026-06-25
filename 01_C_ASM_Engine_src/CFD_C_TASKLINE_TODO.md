# CFD-oriented C 学习任务线（TODO 草案）

> 这份文件保留原始规划。可执行版已经拆分到
> [`40_CFD_C_Questline/`](40_CFD_C_Questline/README.md)，当前进度见
> [`40_CFD_C_Questline/PROGRESS.md`](40_CFD_C_Questline/PROGRESS.md)。

目标：把 C 语言能力逐步连接到 incompressible viscous CFD solver 的实现。

建议节奏：每天完成一个 30–60 分钟的小任务。每个任务都应留下一个可编译的最小程序，必要时附一页以内的笔记。

状态：

- `[ ]` 未开始
- `[x]` 已完成
- `[!]` 能运行，但数学或数值验证尚未完成

## Lv.0 工具链与可复现运行

- [ ] 编译并运行一个最小 `main()`，记录实际 compiler 和 compile command。
- [ ] 学会使用 warning：GCC/Clang `-Wall -Wextra -Wpedantic`，MSVC `/W4`。
- [ ] 分离 source、header 和 build output。
- [ ] 用 debugger 查看一个 scalar、array 和 pointer 的值。
- [ ] 建立最小验证模板：输入、输出、expected value、absolute/relative error。

通关条件：能从命令行稳定编译、运行和定位一个简单错误。

## Lv.1 CFD 所需 C 基础

- [ ] 用 `double` 实现 Reynolds number 计算并检查量纲。
- [ ] 用 function 封装 density、kinematic viscosity 等简单关系。
- [ ] 比较整数除法与浮点除法对 `dt = T / N` 的影响。
- [ ] 用 `fabs(error) < tolerance` 替代浮点数直接相等判断。
- [ ] 编写安全 macro，并观察参数重复求值的副作用。
- [ ] 用 `struct` 表示二维 point、cell 或物性参数。

通关条件：能解释 type、scope、function、macro 和 `struct` 对 solver 代码的影响。

## Lv.2 Array、pointer 与 field memory

- [ ] 验证一维 `double field[N]` 的连续内存布局。
- [ ] 分别用 array index 和 pointer arithmetic 遍历 field。
- [ ] 实现 `clear_field(double *field, size_t n)`。
- [ ] 实现二维索引映射：`id = i * nx + j`。
- [ ] 用一维数组保存二维 scalar field，并打印指定 stencil。
- [ ] 检查边界访问：`i-1`、`i+1` 在 domain edge 的风险。
- [ ] 使用 `malloc/calloc/free` 创建动态 field，并用工具检查 leak。

通关条件：能安全地表达结构化网格 field，并明确 ownership、size 和 boundary。

## Lv.3 离散算子训练场

- [ ] 在 uniform 1D grid 上实现 first-order forward difference。
- [ ] 实现 second-order central difference，并用解析函数验证 order of accuracy。
- [ ] 实现 second derivative stencil。
- [ ] 在二维 uniform grid 上实现 gradient。
- [ ] 实现 divergence，并检查 divergence-free 解析速度场。
- [ ] 实现 Laplacian，并用 `u(x,y)=sin(x)sin(y)` 验证。
- [ ] 做一次 grid refinement study，计算 observed order：

```text
p = log(e_h / e_h2) / log(2)
```

通关条件：每个 operator 都有解析解、误差范数和网格收敛证据。

## Lv.4 Boundary condition 与 ghost cell

- [ ] 为 1D diffusion 实现 Dirichlet boundary condition。
- [ ] 实现 Neumann boundary condition，并明确一阶/二阶离散方式。
- [ ] 使用 ghost cell 统一 interior 与 boundary stencil。
- [ ] 实现 periodic boundary condition。
- [ ] 检查 corner cell 的更新顺序和条件冲突。
- [ ] 写一个 boundary-only test，不启动完整 solver。

通关条件：边界实现与离散 stencil 数学一致，不依赖越界访问。

## Lv.5 Linear algebra 与 Poisson 基础

- [ ] 实现 vector operations：copy、scale、AXPY、dot product、L2 norm。
- [ ] 实现 dense matrix-vector multiplication，理解接口后停止扩展 dense solver。
- [ ] 用 5-point stencil 直接实现 matrix-free Poisson operator。
- [ ] 实现 Jacobi iteration。
- [ ] 实现 Gauss–Seidel iteration。
- [ ] 记录 residual norm，而不只记录 solution change。
- [ ] 验证 Poisson solver 的 boundary condition、compatibility 和 convergence。
- [ ] 阅读并复现 `youxianyuan.c` 中 elimination/fill-in 的数据结构逻辑。

通关条件：能区分 algebraic residual、discretization error 和 iteration error。

## Lv.6 时间推进与黏性扩散

- [ ] 实现 1D linear advection 的 Forward Euler + upwind。
- [ ] 扫描 CFL number，观察 stability boundary。
- [ ] 实现 1D diffusion 的 explicit scheme。
- [ ] 验证扩散稳定性限制 `ν Δt / Δx²`。
- [ ] 实现 implicit diffusion，并调用迭代 linear solver。
- [ ] 比较 temporal error 与 spatial error。

通关条件：能从 amplification/stability 角度解释 timestep 限制。

## Lv.7 Incompressible projection method

- [ ] 建立二维 staggered grid 的 `u`、`v`、`p` 索引约定。
- [ ] 计算 tentative velocity，不含 pressure correction。
- [ ] 从离散 continuity 推导 pressure Poisson equation。
- [ ] 求解 pressure correction。
- [ ] 修正 velocity，并检查 divergence norm 是否下降。
- [ ] 检查 pressure null space 与 reference pressure。
- [ ] 完成 lid-driven cavity 的低 Reynolds number 最小版本。
- [ ] 比较 centerline velocity、mass conservation 和 grid dependence。

通关条件：solver 不仅“画出流场”，还通过离散连续性和基准结果检查。

## Lv.8 工程化与性能

- [ ] 将 field、grid、boundary 和 solver 参数拆成清晰模块。
- [ ] 为关键函数添加 unit test。
- [ ] 添加 AddressSanitizer 或等价 memory check。
- [ ] 记录 compiler optimization 对结果与性能的影响。
- [ ] 比较 AoS 与 SoA 的 field layout。
- [ ] 计时 stencil loop，识别 memory bandwidth 限制。
- [ ] 只在 serial reference solver 正确后考虑 OpenMP/MPI。

通关条件：有一个正确、可测试、可 profiling 的 serial reference implementation。

## Boss 战候选

- [ ] 2D lid-driven cavity：projection method。
- [ ] 2D Taylor–Green vortex：空间和时间收敛验证。
- [ ] 2D Poiseuille flow：解析解、wall shear 和 pressure gradient 验证。
- [ ] 2D backward-facing step：先做 laminar，再讨论更复杂模型。
- [ ] 将一个已验证的离散算子与 Basilisk 实现进行结果对照。

## 后续完善项

- [ ] 为每个任务指定目标文件路径和 prerequisite。
- [ ] 给任务增加 XP、预计时间和难度。
- [ ] 关联可靠教材、论文或官方文档。
- [ ] 建立 weekly checkpoint 和复盘模板。
- [ ] 根据 Gravity 当前 C 水平删减已掌握任务。
