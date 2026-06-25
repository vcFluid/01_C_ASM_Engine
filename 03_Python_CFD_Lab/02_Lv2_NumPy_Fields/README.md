# Lv.2 — NumPy Field Operations

| ID | Mission | 核心能力 | XP |
|---|---|---|---:|
| PY2-M01 | 1D/2D field creation | shape、dtype、layout | 100 |
| PY2-M02 | indexing and ghost cells | interior/boundary 分离 | 100 |
| PY2-M03 | broadcasting experiments | 识别合法与错误 broadcast | 100 |
| PY2-M04 | vectorized gradient/divergence | 离散算子表达 | 125 |
| PY2-M05 | memory order C/F | 与 C/Fortran 数据布局对应 | 125 |
| PY2-M06 | allocation benchmark | temporary array 成本 | 100 |
| PY2-M07 | loop vs vectorization vs Numba | 正确测量性能 | 150 |

验收重点：每个 operator 明确 input/output shape 和 boundary convention；性能测试先验证结果一致，再比较时间。

