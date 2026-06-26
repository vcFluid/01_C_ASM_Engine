# `1-D_Riemann_NM_MacC_teaching.c` 学习索引

## 文件定位

本目录是原程序 `../1-D_Riemann_NM_MacC.c` 的教学副本。

- 原文件继续作为实际开发版本；
- 教学副本保持相同的计算流程，并加入更详细的解释；
- 注释既解释 C 语法，也解释 Euler 方程、MacCormack 格式和程序设计；
- 教学副本完成后必须能够独立编译，以防“注释版”和真实代码脱节。

## 阅读顺序

1. **程序骨架**
   - 头文件、宏、`enum`、`typedef`
   - `struct`
   - 函数指针与模拟 OOP
2. **状态变量与内存**
   - 守恒量 `Q`
   - 原始量 `W`
   - 堆内存、`calloc`、`free`
3. **Euler 方程离散组件**
   - `Q -> W`
   - flux
   - CFL 时间步
   - boundary condition
   - artificial viscosity
4. **MacCormack 时间推进**
   - predictor
   - predicted-state flux
   - corrector
   - 时间层交换
5. **输入输出与程序耦合**
   - Tecplot ASCII
   - snapshot
   - `system()` 与 Project 0
   - Windows 路径和字符串转义
6. **运行控制**
   - 交互式输入
   - 七个预设 Riemann problems
   - `main`

## 分块进度

- [x] 第 1 块：文件结构、预处理、`enum`、`typedef`、`struct`、函数指针和基础辅助函数
- [x] 第 2 块：`solver_allocate()`、`solver_update_primitives()` 与内存/状态转换
- [x] 第 3 块：flux、CFL、boundary condition 与 artificial viscosity
- [x] 第 4 块：MacCormack predictor-corrector
- [x] 第 5 块：Tecplot、snapshot 与 Project 0 OS process coupling
- [x] 第 6 块：交互输入、七个预设算例和 `main`

教学注释已覆盖完整程序。

伴随资料：

- `C_LANGUAGE_NOTES.md`：教学代码中出现的 C 语法速查；
- `NUMERICAL_FLOW_NOTES.md`：Euler/MacCormack/人工粘性数值流程速查。

## 完整性验证

教学版使用与原版相同的 Sod 输入运行。最终数值 `.dat`：

- 行数均为 504；
- 逐行比较无差异；
- 教学版新增内容不改变离散算法或输出数据。

两份源码经过 `gcc -E -P` 预处理并移除注释后，SHA-256 均为：

```text
579DBFFB3521A7C0938D5A45160DDC53CE9C96F6C5066ED054AE5F450043ACFB
```

这说明教学版与原版的有效 C token stream 一致。

## 注释标签

教学副本使用以下标签：

- `[C syntax]`：C 语言语法；
- `[Memory]`：对象生命周期、指针和内存；
- `[Numerics]`：离散格式与稳定性；
- `[Physics]`：Euler 方程和物理量；
- `[Design]`：程序结构与模块职责；
- `[Risk]`：适用条件、潜在错误和常见误区。

## 编译

在 `Project_2` 目录执行：

```powershell
gcc .\self_teaching\1-D_Riemann_NM_MacC_teaching.c `
  -std=c11 -O2 -Wall -Wextra `
  -o .\self_teaching\riemann_teaching.exe -lm
```

程序仍通过相对路径调用：

```text
_Analysical_Solution_Solver\riemann_exact.exe
```

因此应从 `Project_2` 目录运行教学版可执行文件。

## 学习原则

不要只记住代码写法。每个模块至少回答三个问题：

1. 数学上正在计算什么？
2. 这些量为什么要以当前形式储存在内存中？
3. 如果删除或改变这一段，数值结果会发生什么？

## 建议学习方式

不要从第一行连续读到最后一行。建议按函数执行以下循环：

1. 先遮住函数实现，只看注释和函数签名，口述输入、输出和副作用；
2. 把离散公式手写一遍，再对照 array index；
3. 区分哪些语句负责数学计算，哪些语句只负责内存、I/O 或控制流程；
4. 使用 debugger 或临时输出观察一个网格点在 `Q -> Qbar -> Qnext` 中的变化；
5. 修改一个参数并先预测结果，再运行验证。

推荐练习：

1. 把 `solver_init_sod` 重命名为 `solver_init_riemann`，保持行为不变；
2. 把 `solver_compute_flux` 的三个分量逐项从公式重新推导；
3. 手动计算某一时间步的 `max(|u|+a)` 与 `dt`；
4. 对比 `rho/u/p` 三种 artificial-viscosity sensor；
5. 暂时关闭 artificial viscosity，定位第一次产生非有限值的网格点和时间步；
6. 将七个 `if/else` preset 改写为只读配置表，并比较可读性；
7. 为 `solver_write_tecplot` 增加 viscosity sensor 输出列；
8. 用 command-line arguments 替代 stdin，再比较两种 batch interface。
