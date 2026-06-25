# Riemann Post-processing

Generate `rho`, `u`, and `p` comparison plots:

```powershell
python .\postprocess\tecplot_compare.py
```

The defaults use:

```text
runs/sod_baseline/numerical.dat
runs/sod_baseline/exact.dat
```

The script:

1. recognizes `x/rho/u/p` and `X/Density/Velocity/Pressure`;
2. crops the exact solution to the numerical x-domain;
3. writes a normalized two-zone Tecplot dataset;
4. scales x from the numerical domain;
5. scales y from both visible datasets with 5% padding;
6. exports `rho.png`, `u.png`, and `p.png`.

The default backend is Matplotlib and does not require Tecplot or TecPLUS.
The normalized output remains Tecplot-compatible.

To generate the final figures with Tecplot without TecPLUS:

```powershell
python .\postprocess\tecplot_compare.py `
  --backend tecplot-macro `
  --numerical .\runs\sod_automation_test\numerical.dat `
  --exact .\runs\sod_automation_test\exact.dat `
  --output-dir .\runs\sod_automation_test\plots
```

This creates a Tecplot `.mcr` file and runs:

```text
tec360.exe -b export_with_tecplot.mcr
```

The exported `tecplot_*.png` images are rendered by Tecplot 360 and satisfy
the Tecplot output requirement. This route uses the normal Tecplot license,
not TecUtil Server or TecPLUS.

To use PyTecplot instead:

1. Open Tecplot 360.
2. Enable `Scripting > PyTecplot Connections`.
3. Run:

```powershell
python .\postprocess\tecplot_compare.py `
  --backend pytecplot `
  --connect `
  --keep-layout
```

Custom input:

```powershell
python .\postprocess\tecplot_compare.py `
  --numerical .\runs\sod_baseline\numerical.dat `
  --exact .\runs\sod_baseline\exact.dat `
  --output-dir .\runs\sod_baseline\plots
```

PyTecplot requires a valid TecPLUS service. If TecUtil Server reports that
TecPLUS has expired, continue using the default Matplotlib backend.
