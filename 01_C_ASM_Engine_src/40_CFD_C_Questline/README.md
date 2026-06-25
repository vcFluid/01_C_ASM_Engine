# CFD C Questline

面向 incompressible viscous CFD 的 C 语言任务线。原则是先建立可信的 serial numerical kernel，再扩展完整 solver 和性能优化。

## 游戏规则

- 每天建议完成 1 个 mission，时长 30–90 分钟。
- 每个 mission 的源码放在对应关卡的 `src/`，推导和复盘放在 `notes/`，数值输出放在 `results/`。
- 所有 compiler、assembler 和 linker 产物统一输出到项目根目录 `01_C_ASM_Engine_bin/`；Questline 与源码目录内不得创建 `build/`。
- 状态：`[ ]` 未开始，`[>]` 进行中，`[!]` 可运行但验证不足，`[x]` 已通过验收。
- 不以“能够编译”作为数值任务的完成条件。涉及 PDE 的 mission 必须检查适用条件、误差、边界和稳定性。
- 完成任务后同步更新 [PROGRESS.md](PROGRESS.md)；需要写任务复盘时复制 [MISSION_TEMPLATE.md](MISSION_TEMPLATE.md)。

## 地图

| Level | 主题 | Missions | 解锁条件 |
|---|---|---:|---|
| Lv.0 | Toolchain 与可复现运行 | 5 | 无 |
| Lv.1 | CFD 所需 C 基础 | 6 | Lv.0 |
| Lv.2 | Array、pointer 与 field memory | 7 | Lv.1 |
| Lv.3 | Discrete operators | 7 | Lv.2 |
| Lv.4 | Boundary conditions 与 ghost cells | 6 | Lv.3 |
| Lv.5 | Linear algebra 与 Poisson | 8 | Lv.3–4 |
| Lv.6 | Time integration 与 viscous diffusion | 6 | Lv.3、Lv.5 |
| Lv.7 | Incompressible projection method | 8 | Lv.4–6 |
| Lv.8 | Engineering 与 performance | 7 | 至少完成一个 Lv.7 solver |
| Boss | Verification cases | 5 | 按项目要求 |

基础任务总 XP：`7000`。Boss XP：`2500`。总 XP：`9500`。

## 统一验收门槛

1. 使用 warning 编译，无未解释的 warning。
2. 给出输入、预期结果和实际结果。
3. 数值 mission 至少报告一个 error/residual norm。
4. 涉及离散 PDE 时说明 grid、boundary condition、timestep 和 stopping criterion。
5. 不允许 silent NaN、Inf、out-of-bounds 或 memory leak。
