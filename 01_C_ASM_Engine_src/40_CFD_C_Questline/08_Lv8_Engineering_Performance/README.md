# Lv.8 — Engineering and Performance

目标：把已验证的 serial solver 变成可测试、可维护、可 profiling 的 reference implementation。总 XP：800。

| ID | Mission | Target | XP | 时间 | Prerequisite |
|---|---|---|---:|---:|---|
| L8-M01 | Module boundaries | `notes/L8-M01_architecture.md` | 100 | 60m | Lv.7 |
| L8-M02 | Unit test layer | `src/L8-M02_tests.c` | 125 | 90m | M01 |
| L8-M03 | Memory sanitizer pass | `notes/L8-M03_memory_check.md` | 100 | 60m | M02 |
| L8-M04 | Optimization audit | `notes/L8-M04_optimization.md` | 100 | 60m | M02 |
| L8-M05 | AoS vs SoA | `src/L8-M05_layout_benchmark.c` | 125 | 90m | Lv.2 |
| L8-M06 | Stencil profiling | `src/L8-M06_stencil_benchmark.c` | 125 | 90m | Lv.3 |
| L8-M07 | Parallel readiness review | `notes/L8-M07_parallel_readiness.md` | 125 | 90m | M01–M06 |

## 验收

- M01：拆分 grid、field、BC、operator、linear solver、time integrator 和 case config。
- M02：operator/BC tests 不依赖完整 simulation。
- M03：AddressSanitizer、Valgrind 或平台等价工具无未解释问题。
- M04：比较 debug/release；确认 optimization 不改变通过 tolerance 的结果。
- M05：固定数据量和算法，重复计时并报告 variance。
- M06：报告 throughput 或 bandwidth，不只给单次 wall time。
- M07：确认 deterministic serial baseline、domain decomposition 边界和 reduction points。

规则：未完成 verification 前，不以 OpenMP/MPI/GPU 加速掩盖错误。

