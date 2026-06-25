# Lv.0 — Toolchain

目标：建立可复现的 compile–run–debug–verify 循环。总 XP：300。

| ID | Mission | Target | XP | 时间 | Prerequisite |
|---|---|---|---:|---:|---|
| L0-M01 | Toolchain scan | `notes/L0-M01_toolchain.md` | 50 | 30m | 无 |
| L0-M02 | Warning gate | `src/L0-M02_warning_gate.c` | 50 | 30m | M01 |
| L0-M03 | Source/build separation | `notes/L0-M03_build_layout.md` | 50 | 30m | M01 |
| L0-M04 | Debug scalar/array/pointer | `src/L0-M04_debugger.c` | 75 | 45m | M02 |
| L0-M05 | Verification harness | `src/L0-M05_verify.c` | 75 | 60m | M02 |

## 验收

- M01：记录 compiler name、version、平台和完整 command。
- M02：故意制造 3 类 warning，再修复到 warning-clean。
- M03：源码目录不产生 `.exe/.o/.obj/.pdb`。
- M04：用 breakpoint 观察 scalar、array element、pointer address 和 dereference。
- M05：实现 absolute/relative error，并测试 expected value 为零的情形。

已有参考：[`../../00_Environment_Check/test.c`](../../00_Environment_Check/test.c)。

