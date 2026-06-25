# 测试输出目录

Project 2 的所有运行产物统一写入本目录，不再散落在项目根目录。

每组算例使用独立子目录：

```text
runs/
  sod_baseline/
  lax_baseline/
  grid_study/
  cfl_study/
  viscosity_study/
```

单个算例目录建议包含：

```text
case_name/
  numerical.dat
  exact.dat
  errors.csv
  run.log
  plots/
    numerical_exact_combined.dat
    export_with_tecplot.mcr
    tecplot_rho_000001.png
    tecplot_u_000001.png
    tecplot_p_000001.png
```

约定：

- executable、日志、临时数据和图片均属于可再生成的运行产物；
- `runs/.gitignore` 默认忽略所有算例输出；
- 需要纳入报告的最终图片复制到对应报告的 `figures/`；
- 需要长期保存的基准参数应写入配置文件或报告，而不是依赖运行目录；
- 手动运行 solver 时也应显式将输出文件名设置到 `runs/<case_name>/`。
