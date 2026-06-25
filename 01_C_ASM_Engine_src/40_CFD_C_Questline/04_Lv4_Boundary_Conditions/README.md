# Lv.4 — Boundary Conditions

目标：让 boundary implementation 与离散 stencil 数学一致。总 XP：700。

| ID | Mission | Target | XP | 时间 | Prerequisite |
|---|---|---|---:|---:|---|
| L4-M01 | Dirichlet BC | `src/L4-M01_dirichlet.c` | 100 | 60m | Lv.3 |
| L4-M02 | Neumann BC | `src/L4-M02_neumann.c` | 125 | 75m | M01 |
| L4-M03 | Ghost-cell formulation | `src/L4-M03_ghost_cell.c` | 125 | 75m | M01–M02 |
| L4-M04 | Periodic BC | `src/L4-M04_periodic.c` | 100 | 60m | M03 |
| L4-M05 | Corner policy | `src/L4-M05_corners.c` | 100 | 60m | M03 |
| L4-M06 | Boundary-only test suite | `src/L4-M06_bc_tests.c` | 150 | 90m | M01–M05 |

## 验收

- M01：boundary value 与解析值一致。
- M02：写清一阶或二阶 closure，并用线性函数验证。
- M03：interior loop 不包含 BC-specific branch。
- M04：验证首尾邻接关系和 index wrap。
- M05：明确两个 wall condition 在 corner 的优先级或组合规则。
- M06：不启动 time loop 或完整 solver 即可测试所有 BC。

风险：Neumann-only Poisson problem 存在 null space，并需要 compatibility condition。

