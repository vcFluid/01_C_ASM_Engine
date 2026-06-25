# Lv.2 — Field Memory

目标：安全表达 structured-grid field，明确 memory layout、size、boundary 和 ownership。总 XP：700。

| ID | Mission | Target | XP | 时间 | Prerequisite |
|---|---|---|---:|---:|---|
| L2-M01 | Contiguous field probe | `src/L2-M01_layout.c` | 75 | 45m | Lv.1 |
| L2-M02 | Index vs pointer traversal | `src/L2-M02_traversal.c` | 75 | 45m | M01 |
| L2-M03 | Clear field API | `src/L2-M03_clear_field.c` | 75 | 45m | M02 |
| L2-M04 | 2D flattening | `src/L2-M04_index2d.c` | 100 | 60m | M01 |
| L2-M05 | Stencil inspector | `src/L2-M05_stencil.c` | 100 | 60m | M04 |
| L2-M06 | Boundary guard | `src/L2-M06_boundary_guard.c` | 125 | 60m | M05 |
| L2-M07 | Dynamic field ownership | `src/L2-M07_dynamic_field.c` | 150 | 90m | M03–M06 |

## 验收

- M01：用地址差验证相邻 `double` 的逻辑步长，不硬编码 8 bytes。
- M02：两种遍历结果完全一致。
- M03：API 接收 pointer 和 explicit size，测试 `n=0`。
- M04：证明 `id=i*nx+j` 的索引范围。
- M05：打印中心点及其 ±x/±y 邻居。
- M06：通过显式 guard 阻止越界，不依赖 crash 检测错误。
- M07：每次 allocation 有清晰 owner 和对应 free；通过 memory checker。

已有参考：`../../02_Syntax_Experiments/pointer_53_*.c` 与 `pointer_54_*.c`。

