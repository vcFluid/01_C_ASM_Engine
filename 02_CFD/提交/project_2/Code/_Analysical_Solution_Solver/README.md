# Project 0 精确解程序

本目录保存独立的一维 Euler Riemann 精确解程序。Project 2 不直接包含它的求解源码，
而是通过操作系统启动其 executable，以保持两个教学项目的相对独立性。

## 编译

在 `Project_2` 目录执行：

```powershell
gcc .\_Analysical_Solution_Solver\1-D_Riemann_AM.c `
  -std=c11 -O2 -Wall -Wextra `
  -o .\_Analysical_Solution_Solver\riemann_exact.exe `
  -lm
```

生成的 `.exe` 属于编译产物，不提交 Git。

## 独立交互运行

不带 command-line arguments 时，程序保持 Project 0 原有的交互模式：

```powershell
.\_Analysical_Solution_Solver\riemann_exact.exe
```

## Project 2 调用接口

Project 2 使用 batch mode：

```text
riemann_exact.exe --batch
  rho_L u_L p_L
  rho_R u_R p_R
  gamma
  xmin xmax x0
  nx
  time
  output.dat
```

示例：

```powershell
.\_Analysical_Solution_Solver\riemann_exact.exe `
  --batch `
  1.0 0.0 1.0 `
  0.125 0.0 0.1 `
  1.4 `
  0.0 1.0 0.5 `
  501 `
  0.2 `
  .\runs\sod_exact.dat
```

Batch mode 直接在 Project 2 的同一计算域和同一网格上采样精确解，因此 numerical
和 exact 数据可以逐点比较，不需要空间插值。

## C 与 OS 的调用关系

Project 2 使用标准库函数：

```c
int status = system(command);
```

执行过程为：

```text
Project 2 process
    ↓ 构造 command-line string
Windows cmd.exe
    ↓ 启动
riemann_exact.exe process
    ↓ 写出
exact.dat
    ↓ 返回 exit status
Project 2 process
```

关键技术点：

- executable 路径和输出路径需要用双引号包围；
- Windows `cmd.exe /c` 对首尾引号有特殊处理，因此整条 command 还需要一层外部引号；
- Project 2 在调用前检查 executable 是否存在；
- Project 0 成功返回 `0`，参数或输出错误返回非零值；
- Project 2 检查 `system()` 返回值，外部程序失败时不会静默继续；
- 这是 process-level coupling，不是普通 C function call；
- 进程启动有额外开销，适合课程作业和低频 benchmark，不适合每个时间步调用。
