# Lv.3 — Verification & Testing

| ID | Mission | 核心能力 | XP |
|---|---|---|---:|
| PY3-M01 | `pytest` scalar tests | absolute/relative tolerance | 100 |
| PY3-M02 | parameterized tests | 多组网格和参数 | 100 |
| PY3-M03 | L1/L2/Linf norms | 误差度量 | 100 |
| PY3-M04 | grid refinement utility | observed order | 150 |
| PY3-M05 | residual history checks | convergence/failure 判定 | 125 |
| PY3-M06 | manufactured solution | source term verification | 175 |
| PY3-M07 | regression test for C output | 防止 solver 回退 | 150 |

验收重点：tolerance 不能为了“让测试通过”任意放宽；refinement study 至少使用三组系统加密网格。

