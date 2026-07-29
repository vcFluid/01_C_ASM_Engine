# 1-D_Riemann_Roe.c 完整理解指南

本文件夹用于解释：

```text
02_CFD/project_3_Roe/1-D_Riemann_Roe.c
```

目标不是简单复述注释，而是帮助你真正理解三件事：

1. 这份 C 代码如何组织数据、函数、内存和主流程。
2. Roe 格式为什么是 Godunov-type finite volume method 中的 interface numerical flux。
3. 一个时间步到底如何从 \(Q^n\) 迭代到 \(Q^{n+1}\)。

## 推荐阅读顺序

1. `01_C_syntax_and_program_structure.md`

   先解释 C 语言层面的语法逻辑，包括 `typedef struct`、指针数组、`static`、`const`、`calloc`、`memcpy`、函数调用关系和 `main()` 主流程。

2. `02_data_model_and_indexing.md`

   解释 solver 内部的数据模型：守恒量、原始量、通量、cell-centered 网格、ghost cell、物理 cell 下标、界面通量下标。

3. `03_roe_fvm_iteration.md`

   解释 Roe 格式本身：Euler 方程、有限体积积分、Roe average、特征值、波强和 FVM 守恒更新。

4. `04_function_walkthrough.md`

   按代码函数顺序解释每个函数的角色，以及这些函数如何串成完整 solver。

5. `05_debug_checks_and_common_misunderstandings.md`

   解释常见误区和检查方法：FDM vs FVM、格点通量 vs 界面通量、为什么 case 4 会负压、为什么 exact 解要插值比较。

6. `06_minimal_pseudocode.md`

   给出一份从代码抽象出来的最小伪代码。读完前面几份后，用它检查自己是否真正掌握了迭代逻辑。

7. `07_ppt_formula_mapping.md`

   对照课件中 \(\hat{\bm F}=\frac12(F_L+F_R)-\frac12|\tilde A|\Delta Q\) 以及 \(\alpha_1...\alpha_5\) 的写法，说明当前代码如何按课件公式显式构造 `alpha1...alpha5`。

## 一句话总览

Project 3 的代码可以抽象成：

```text
读入 Riemann 初值和网格
    -> 初始化 cell average 守恒量 Q_i^0
    -> 每步计算 CFL 时间步
    -> 用 ghost cell 补边界
    -> 在每个界面用 Roe 近似 Riemann solver 得到 Fhat_{i+1/2}
    -> 用有限体积通量差更新 Q_i
    -> 输出 numerical solution 并调用 exact solver 对比
```

与 Project 2 的关键差异：

```text
Project 2: FDM/MacCormack, 状态在格点上，使用格点物理通量 F(Q_i) 做差分
Project 3: FVM/Roe, 状态在控制体上，使用界面数值通量 Fhat_{i+1/2} 做守恒更新
```
