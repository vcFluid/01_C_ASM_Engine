# Linux / OS Questline

定位：目标不是背命令，而是理解 CFD 程序在操作系统上的编译、运行、内存、I/O、并行与故障诊断。

## Quest Map

| Level | 主题 | Missions | XP | 解锁条件 |
|---|---|---:|---:|---|
| Lv.0 | Shell navigation & safety | 6 | 450 | 无 |
| Lv.1 | Files, permissions & text tools | 7 | 650 | Lv.0 |
| Lv.2 | Process, memory & I/O | 7 | 850 | Lv.1 |
| Lv.3 | Build, link & debug | 7 | 1000 | Lv.1–2 |
| Lv.4 | Automation & reproducibility | 6 | 800 | Lv.3 |
| Lv.5 | HPC foundations | 7 | 1100 | Lv.3–4 |
| Boss | Operate a CFD experiment | 3 | 1500 | Lv.5 |

总 XP：`6350`。

## 游戏规则

- 优先在 WSL2 或原生 Linux 中完成；Windows PowerShell 仅用于对照。
- 危险命令先在 disposable sandbox 中演练，不对真实数据目录执行。
- 每个 mission 记录命令、预期、实际输出和 failure mode。
- “会执行命令”不等于理解 OS；必须能解释 process、virtual memory、file descriptor、dynamic linking 等关键概念。

## Unified acceptance gate

1. 命令可重复，路径和依赖明确。
2. 脚本使用严格错误处理并传播 exit code。
3. 能区分 CPU time、wall time、memory usage 和 I/O wait。
4. 能解释 compile、assemble、link、load 的边界。
5. 并行任务必须检查正确性与 scaling，不能只报告“更快”。

进度记录见 [PROGRESS.md](PROGRESS.md)。

