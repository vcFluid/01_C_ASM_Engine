# 01. C 语法与程序结构

本文件只解释代码语法和工程组织，不先深入 Roe 数值格式。

源文件：

```text
02_CFD/project_3_Roe/1-D_Riemann_Roe.c
```

## 1. 头文件

```c
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
```

含义：

```text
stdio.h   -> printf, fprintf, fopen, fclose, fgets
stdlib.h  -> calloc, free, system
math.h    -> sqrt, fabs, isfinite
string.h  -> memset, memcpy, strcspn, strrchr
```

这份代码没有使用外部库。只要有 C 编译器和数学库 `-lm`，主 solver 就能编译。

## 2. 宏定义

```c
#define INPUT_LINE_LEN 128
#define OUTPUT_NAME_LEN 256
#define COMMAND_LINE_LEN 2048
#define PHYSICAL_FLOOR 1.0e-12
#define NGHOST 1
```

宏是预处理阶段的文本替换。这里它们不是变量，没有类型检查。

含义：

```text
INPUT_LINE_LEN      用户输入一行最多读多少字符
OUTPUT_NAME_LEN     输出文件名缓冲区长度
COMMAND_LINE_LEN    调用 exact solver 的命令行缓冲区长度
PHYSICAL_FLOOR      判断 rho 和 p 是否非物理的下限
NGHOST              每侧 ghost cell 数量
```

`PHYSICAL_FLOOR` 不是修补值。代码不会把负压强强行改成小正数，而是直接报错停止。这样做是为了让 Roe 的 positivity failure 进入结果讨论。

## 3. typedef struct

代码大量使用：

```c
typedef struct {
    double rho;
    double rhou;
    double rhoE;
} Conserved;
```

这等价于定义一个新类型 `Conserved`。以后可以直接写：

```c
Conserved q;
```

不用写：

```c
struct Conserved q;
```

这份代码的结构体分两类。

第一类是单点状态：

```text
Conserved       一个 cell 的守恒量 Q
Primitive       一个 cell 的原始量 W 和热力学辅助量
Flux            一个通量向量
PrimitiveState  Riemann 初始左/右状态
```

第二类是数组或 solver 配置：

```text
ConservedArray  一整层 Q 数组
PrimitiveArray  一整层 W 数组
FluxArray       一整层界面通量数组
Grid1D          网格信息
TimeControl     时间推进信息
Solver          整个 solver 的总容器
```

## 4. 指针数组

例如：

```c
typedef struct {
    double *rho;
    double *rhou;
    double *rhoE;
} ConservedArray;
```

这里的 `double *rho` 是指针，指向一段连续的 `double` 数组。真正的数组空间在运行时由 `calloc` 分配：

```c
q->rho = alloc_double_array(n);
```

所以 `q->rho[i]` 表示第 `i` 个数组元素。

这种写法的好处：

```text
rho, rhou, rhoE 分开存，循环访问直观
每个数组长度可以由网格数动态决定
current 和 next 可以共用同一种数组结构
```

这里采用的是 structure of arrays，简称 SoA：

```text
rho[]   rhou[]   rhoE[]
```

不是 array of structures：

```text
Q[0].rho, Q[0].rhou, Q[0].rhoE
```

## 5. `->` 和 `.`

如果变量本身是结构体，用点号：

```c
solver.grid.nx
```

如果变量是结构体指针，用箭头：

```c
solver->grid.nx
```

例如函数参数：

```c
static void grid_finalize(Grid1D *grid)
```

这里 `grid` 是指针，所以函数内部写：

```c
grid->dx
```

## 6. static 函数

几乎所有函数都写成：

```c
static int solver_allocate(Solver *solver)
```

`static` 在文件作用域函数上的含义是：

```text
这个函数只在当前 .c 文件内部可见
不会暴露给其他编译单元
```

这对单文件作业很合适，能避免函数名和其他文件冲突。

## 7. const 参数

例如：

```c
static Flux physical_flux_from_conserved(const Solver *solver, Conserved q)
```

`const Solver *solver` 表示这个函数不会修改 `solver` 指向的内容。

它的作用：

```text
告诉读代码的人：这里只读 solver
告诉编译器：如果函数里试图修改 solver，就报错
```

## 8. 内存分配与释放

核心函数：

```c
static double *alloc_double_array(size_t n)
{
    return (double *)calloc(n, sizeof(double));
}
```

`calloc(n, sizeof(double))` 做两件事：

```text
分配 n 个 double 的空间
把这段空间初始化为 0
```

释放函数：

```c
free(q->rho);
q->rho = NULL;
```

释放后把指针设为 `NULL`，是为了避免悬挂指针。

## 9. 为什么要有 current 和 next

显式时间推进不能一边读 `Q^n` 一边覆盖成 `Q^{n+1}`，否则后面的 cell 会读到已经更新过的邻居。

所以代码中有：

```text
current  -> 当前时间层 Q^n
next     -> 下一时间层 Q^{n+1}
```

每一步：

```text
用 current 计算所有界面通量
用通量差写入 next
检查 next 是否物理
把 next 复制回 current
```

对应代码：

```c
conserved_copy(solver, &solver->current, &solver->next);
```

## 10. 主流程 main()

`main()` 是程序入口。它的逻辑是：

```text
solver_create_default()
print_banner()
configure_riemann_case()
configure_domain_and_grid()
configure_numerics()
ask_output_filename()
solver_allocate()
initialize_riemann_problem()
while t < t_max:
    advance_one_roe_step()
write_tecplot()
run_exact_solver()
solver_destroy()
```

这说明代码把两类内容分开了：

```text
外层工程流程：读输入、分配内存、输出文件、调用精确解
数值推进核心：advance_one_roe_step()
```

真正需要理解 Roe 格式时，重点看：

```text
compute_roe_flux()
compute_all_roe_fluxes()
advance_one_roe_step()
```

## 11. 函数调用关系

简化调用图：

```text
main
  -> solver_create_default
  -> configure_riemann_case
  -> configure_domain_and_grid
  -> configure_numerics
  -> solver_allocate
  -> initialize_riemann_problem
  -> while loop
       -> advance_one_roe_step
            -> compute_dt_from_cfl
                 -> compute_max_wave_speed
                      -> compute_primitive_from_current
            -> apply_zero_gradient_boundary
            -> conserved_state_is_physical
            -> compute_all_roe_fluxes
                 -> compute_roe_flux
                      -> primitive_from_conserved
                      -> physical_flux_from_conserved
                      -> fabs for Roe eigenvalue magnitudes
            -> finite volume update into next
            -> apply_zero_gradient_boundary
            -> conserved_state_is_physical
            -> conserved_copy
  -> write_tecplot
  -> run_exact_solver
  -> solver_destroy
```

如果你只想抓住程序骨架，记住：

```text
main 管流程
Solver 管数据
advance_one_roe_step 管一次迭代
compute_roe_flux 管一个界面
```
