# Lv.1 — C Fundamentals for CFD

目标：掌握会直接影响 solver 正确性的 type、function、macro 和 struct。总 XP：450。

| ID | Mission | Target | XP | 时间 | Prerequisite |
|---|---|---|---:|---:|---|
| L1-M01 | Reynolds calculator | `src/L1-M01_reynolds.c` | 50 | 30m | Lv.0 |
| L1-M02 | Fluid properties functions | `src/L1-M02_properties.c` | 75 | 45m | M01 |
| L1-M03 | Integer division trap | `src/L1-M03_division.c` | 50 | 30m | Lv.0 |
| L1-M04 | Floating tolerance | `src/L1-M04_tolerance.c` | 75 | 45m | M03 |
| L1-M05 | Safe macro lab | `src/L1-M05_macros.c` | 100 | 60m | Lv.0 |
| L1-M06 | Cell and material structs | `src/L1-M06_structs.c` | 100 | 60m | M02 |

## 验收

- M01：说明每个输入的 SI unit；拒绝 `μ <= 0` 或无效 characteristic length。
- M02：function 不依赖隐藏 global state。
- M03：展示 `1/N` 与 `1.0/N` 的差异。
- M04：同时处理 absolute 和 relative tolerance。
- M05：解释 macro precedence 和 repeated evaluation。
- M06：打印成员值和 `sizeof`，解释 padding 不能被假定为零。

已有参考：

- [`../../10_CFD_Mini_Exercises/03_Re_turbulence.c`](../../10_CFD_Mini_Exercises/03_Re_turbulence.c)
- [`../../10_CFD_Mini_Exercises/05_code_optimization.c`](../../10_CFD_Mini_Exercises/05_code_optimization.c)
- [`../../02_Syntax_Experiments/Macro_Definition_4_11_square-error.c`](../../02_Syntax_Experiments/Macro_Definition_4_11_square-error.c)
- [`../../02_Syntax_Experiments/structure_61_1.c`](../../02_Syntax_Experiments/structure_61_1.c)

