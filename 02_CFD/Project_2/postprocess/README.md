# Project 2 后处理程序

本目录仍然是 Project 2 自动化流程的核心，并未因更换 Tecplot 自动化方案而废弃。(原方案需要订阅 Tecplot)

当前后处理流程为：

```text
数值解与精确解数据
        ↓
Python 读取、统一变量名和计算域
        ↓
生成包含 Numerical 与 Exact 两个 zone 的 Tecplot 数据
        ↓
Python 自动生成 Tecplot macro
        ↓
tec360.exe -b 执行 macro
        ↓
由 Tecplot 360 导出 rho、u、p 对比图
```

其中 Python 负责数据整理和流程控制，最终正式图片由 Tecplot 360 渲染，满足作业使用
Tecplot 出图的要求。

## 自动化方案说明

原计划使用 PyTecplot 连接 Tecplot GUI 或调用 Tecplot engine，但本机 TecPLUS 服务已经
过期，导致以下功能不可用：

```text
TecUtil Server
PyTecplot Connections
PyTecplot batch engine
```

目前采用 Tecplot 原生 `.mcr` macro batch 方案：

```text
tec360.exe -b export_with_tecplot.mcr
```

该方案使用基础 Tecplot 360 许可证，不依赖 TecPLUS，已经完成实际验证。因此：

- `postprocess` 文件夹仍应保留；
- `tecplot_compare.py` 仍是后处理主程序；
- PyTecplot backend 仅作为未来恢复 TecPLUS 后的可选方案；
- 当前正式出图应使用 `tecplot-macro` backend；
- Matplotlib backend 仅用于快速预览和检查，不作为最终作业图片的默认来源。

## 主程序

```text
tecplot_compare.py
```

其主要功能包括：

1. 读取数值解和精确解的 Tecplot ASCII `.dat` 文件；
2. 自动识别两套变量名称：
   - `x, rho, u, p`
   - `X, Density, Velocity, Pressure`
3. 以数值解计算域为准裁剪精确解；
4. 自动计算各变量的横纵坐标范围；
5. 生成统一的双-zone Tecplot 数据文件；
6. 自动生成 Tecplot `.mcr` 文件；
7. 调用 Tecplot batch mode 导出图片；
8. 提供 Matplotlib 快速预览和 PyTecplot 可选 backend。

## Tecplot 正式出图

以当前 Sod 自动化测试为例：

```powershell
python .\postprocess\tecplot_compare.py `
  --backend tecplot-macro `
  --numerical .\runs\sod_baseline\numerical.dat `
  --exact .\runs\sod_baseline\exact.dat `
  --output-dir .\runs\sod_baseline\plots
```

程序将自动生成：

```text
numerical_exact_combined.dat
export_with_tecplot.mcr
tecplot_rho_000001.png
tecplot_u_000001.png
tecplot_p_000001.png
```

其中 `tecplot_*.png` 由 Tecplot 360 实际渲染。

## Matplotlib 快速预览

不指定 backend 时默认使用 Matplotlib：

```powershell
python .\postprocess\tecplot_compare.py `
  --numerical .\runs\sod_baseline\numerical.dat `
  --exact .\runs\sod_baseline\exact.dat `
  --output-dir .\runs\sod_baseline\plots
```

该模式不需要 Tecplot 许可证，适合快速检查：

- 数据能否正确读取；
- 精确解和数值解是否位于同一坐标系；
- 自动坐标范围是否合理；
- 波结构和数值振荡是否符合预期。

正式提交的作业图片仍建议使用 `tecplot-macro` backend。

## PyTecplot 可选方案

只有 TecPLUS 服务有效时，才可使用：

```powershell
python .\postprocess\tecplot_compare.py `
  --backend pytecplot `
  --connect `
  --keep-layout
```

如果 Tecplot 显示：

```text
TecUtil Server could not be enabled.
Your TecPLUS service has expired.
```

说明 PyTecplot connection 不可用。此时无需修改求解器或续订服务，直接使用
`tecplot-macro` backend 即可。

## 后续扩展

后续数值验证功能仍应放在本目录中，包括：

- 点值误差与绝对误差曲线；
- `L1`、`L2` 和 `Linf` 误差；
- 接触间断和激波附近的局部放大图；
- 网格收敛性分析；
- CFL、人工粘性系数和 sensor 参数扫描；
- 数值解与精确解的时间动画；
- Tecplot macro 和报告图片的批量生成。
