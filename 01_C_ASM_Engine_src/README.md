# C / CFD 学习代码索引

这个目录用于保存 C 语言学习过程中的最小示例、语法实验和面向 CFD 的数值练习。

## 目录结构

```text
01_C_ASM_Engine_src/
├─ 00_Environment_Check/       编译器、终端和最小程序验证
├─ 01_Fundamentals/            C 基础与 pointer 基础示例
├─ 02_Syntax_Experiments/      macro、pointer、struct 等语法实验
├─ 10_CFD_Mini_Exercises/      带有 CFD/物理背景的小练习
├─ 20_Numerical_Logic/         数值算法、稀疏结构和 solver 底层逻辑
├─ 30_Basilisk_Lab/            Basilisk 相关实验预留区
├─ 40_CFD_C_Questline/         游戏化 CFD-oriented C 学习任务线
├─ 99_Trash_Can/               暂时无法归类、待复核内容
└─ CFD_C_TASKLINE_TODO.md      面向 CFD 的 C 学习任务线
```

## 使用约定

- 一个 `.c` 文件尽量只验证一个概念。
- 错误示例保留，并通过文件名或配套 `.md` 说明错误原因。
- 源码和笔记放在 `01_C_ASM_Engine_src/`。
- 所有编译产物统一放到项目根目录的 `01_C_ASM_Engine_bin/`；源码树内禁止新增 `build/`。
- 编译产物包括但不限于 `.exe`、`.o`、`.obj`、`.pdb`、`.ilk`、`.out` 和临时链接文件。
- 新增练习优先使用 `NN_topic.c` 命名；现有文件名暂不改动。
- 数值练习至少记录：输入、预期结果、误差指标和边界条件。
- 不把“程序能运行”等同于“数值方法正确”；需要单独检查 consistency、stability 和 conservation。

## 当前说明

- 当前源码树中的旧 `build/` 仅属于历史遗留内容；后续编译不得继续写入。
- `20_Numerical_Logic/youxianyuan.c` 涉及邻接表、消元与 fill-in，归入数值/稀疏结构主题。
- `30_Basilisk_Lab/` 暂时保留为空目录，后续用于独立、可复现的 Basilisk 小实验。
- `40_CFD_C_Questline/` 是当前执行入口；从其中的 `PROGRESS.md` 开始，每个 Level 的 `README.md` 给出具体任务与验收标准。
