# Lv.6 — C/Python Coupling & Automation

| ID | Mission | 核心能力 | XP |
|---|---|---|---:|
| PY6-M01 | Python launch C executable | subprocess、exit code、logs | 100 |
| PY6-M02 | parameter sweep | case generation | 125 |
| PY6-M03 | parse solver diagnostics | residual/conservation gate | 125 |
| PY6-M04 | `ctypes` call to C function | ABI 与 pointer/shape | 150 |
| PY6-M05 | shared library field operation | zero-copy 风险 | 175 |
| PY6-M06 | automated refinement study | run–collect–fit–plot | 150 |
| PY6-M07 | CI-style local verification command | 一键回归测试 | 175 |

风险：跨语言接口最危险的是 ownership、lifetime、dtype、contiguity 和 ABI；任何一项不明确都可能产生 silent corruption。

