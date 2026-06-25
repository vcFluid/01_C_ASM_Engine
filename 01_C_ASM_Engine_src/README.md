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
├─ 99_Trash_Can/               暂时无法归类、待复核内容
├─ build/                      编译生成物；不作为源码阅读入口
└─ CFD_C_TASKLINE_TODO.md      面向 CFD 的 C 学习任务线
```

## 使用约定

- 一个 `.c` 文件尽量只验证一个概念。
- 错误示例保留，并通过文件名或配套 `.md` 说明错误原因。
- 源码和笔记放在主题目录；`.exe`、`.o`、`.obj`、`.pdb`、`.ilk` 放在 `build/`。
- 新增练习优先使用 `NN_topic.c` 命名；现有文件名暂不改动。
- 数值练习至少记录：输入、预期结果、误差指标和边界条件。
- 不把“程序能运行”等同于“数值方法正确”；需要单独检查 consistency、stability 和 conservation。

## 当前说明

- `build/_legacy_mixed_Debug/` 是整理前由多个根目录源码共同产生的历史输出，暂不强行归属。
- `20_Numerical_Logic/youxianyuan.c` 涉及邻接表、消元与 fill-in，归入数值/稀疏结构主题。
- `30_Basilisk_Lab/` 暂时保留为空目录，后续用于独立、可复现的 Basilisk 小实验。

