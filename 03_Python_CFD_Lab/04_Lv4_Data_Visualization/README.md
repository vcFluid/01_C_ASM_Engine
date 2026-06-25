# Lv.4 — Data Pipeline & Visualization

| ID | Mission | 核心能力 | XP |
|---|---|---|---:|
| PY4-M01 | robust text/CSV reader | header、缺失值、shape 校验 | 100 |
| PY4-M02 | Tecplot-style data parser | 读取现有 CFD 输出 | 125 |
| PY4-M03 | exact/numerical alignment | 坐标插值与误差计算 | 125 |
| PY4-M04 | publication plot template | units、legend、metadata | 100 |
| PY4-M05 | batch post-processing | 多 case 自动处理 | 125 |
| PY4-M06 | HDF5/xarray field storage | 带 metadata 的大场数据 | 125 |

风险：插值后的误差混合了 numerical error 与 interpolation error；比较前必须确认网格与采样位置。

