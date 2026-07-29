# Roe FVM Batch Matrix

## 与 Project 2 的对应关系

| Project 2 | Project 3 Roe |
|---|---|
| `parameter_matrix.py` | `targeted_matrix.py` |
| `run_report_matrix.py` | `run_report_matrix.py` |
| artificial viscosity axis | not used |
| sensor axis | not used |
| CFL | CFL |
| Nx | Nx |
| numerical/exact error table | numerical/exact error table |

## 当前矩阵轴

```text
case = 1..7
CFL
Nx
```

默认报告数据：

```text
CFL = 0.5
Nx  = 501
```

每个 Riemann case 一个 run。

## 快速 smoke test

```powershell
python .\postprocess\targeted_matrix.py `
  --case 1 `
  --cfls 0.5 `
  --nxs 101 `
  --output-dir .\runs\_roe_matrix_smoke `
  --backend none
```

预期输出：

```text
matrix_results.csv
matrix_errors.dat
SUMMARY.md
```
