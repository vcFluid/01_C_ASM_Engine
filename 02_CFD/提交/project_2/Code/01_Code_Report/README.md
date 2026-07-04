# Code Report

本报告解释 Project 2 的程序架构、关键代码逻辑与工程实现细节。

主文件：

```text
main.tex
```

章节：

```text
sections/
  01_architecture.tex
  02_solver_state.tex
  03_time_marching.tex
  04_process_coupling.tex
  05_postprocess.tex
  06_limitations.tex
```

编译：

```powershell
xelatex -output-directory=build main.tex
xelatex -output-directory=build main.tex
```

本报告回答“代码如何实现”，不重复展开数值方法推导和结果分析。
