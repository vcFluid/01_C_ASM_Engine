# 02. 数据模型与下标逻辑

Project 3 最容易混乱的地方不是 Roe 公式，而是下标语义。

核心结论：

```text
Project 2: 更接近 FDM 格点存储，Q_i 位于 x_i
Project 3: cell-centered FVM，Q_i 表示第 i 个控制体的 cell average
```

## 1. 三类变量

一维 Euler 方程中，代码使用三类变量。

### 守恒量 Q

```text
Q = [rho, rho*u, rho*E]
```

代码结构：

```c
typedef struct {
    double rho;
    double rhou;
    double rhoE;
} Conserved;
```

守恒量是 solver 真正推进的主变量。有限体积法必须推进守恒量，因为激波跳跃关系依赖守恒形式。

### 原始量 W

```text
W = [rho, u, p]
```

代码结构：

```c
typedef struct {
    double rho;
    double u;
    double p;
    double E;
    double H;
    double a;
} Primitive;
```

原始量用于：

```text
CFL 波速 |u|+a
Roe average
压力检查
Tecplot 输出
物理解释
```

它不独立推进，而是由 \(Q\) 反算。

### 通量 F

Euler 物理通量：

```text
F(Q) = [rho*u, rho*u^2 + p, u*(rhoE + p)]
```

代码结构：

```c
typedef struct {
    double mass;
    double momentum;
    double energy;
} Flux;
```

在 Project 3 中要区分：

```text
F(Q_i)              物理通量，由某个状态直接算出
Fhat_{i+1/2}        界面数值通量，由左右两个状态通过 Roe solver 算出
```

## 2. cell-centered 网格

代码中：

```c
typedef struct {
    int nx;
    int nghost;
    int ntotal;
    int first;
    int last;
    double xmin;
    double xmax;
    double x0;
    double dx;
} Grid1D;
```

含义：

```text
nx      物理控制体数量
nghost  每侧 ghost cell 数量
ntotal  数组总长度 = nx + 2*nghost
first   第一个物理 cell 的数组下标
last    最后一个物理 cell 的数组下标
dx      控制体宽度 = (xmax-xmin)/nx
```

注意：这里 `nx` 不是 Project 2 那种节点数量含义。

## 3. 用 nx=5 的例子理解下标

假设：

```text
nx = 5
nghost = 1
ntotal = 7
first = 1
last = 5
```

数组下标：

```text
i:        0     1     2     3     4     5     6
role:   ghost  cell  cell  cell  cell  cell  ghost
        left    0     1     2     3     4    right
```

物理 cell 是：

```text
i = first ... last
i = 1 ... 5
```

左 ghost cell：

```text
i = first - 1 = 0
```

右 ghost cell：

```text
i = last + 1 = 6
```

## 4. cell center 坐标

代码：

```c
static double cell_left_edge_x(const Solver *solver, int i)
{
    return solver->grid.xmin +
           (double)(i - solver->grid.first) * solver->grid.dx;
}

static double cell_center_x(const Solver *solver, int i)
{
    return cell_left_edge_x(solver, i) + 0.5 * solver->grid.dx;
}
```

第一个物理 cell 的中心：

```text
x = xmin + 0.5*dx
```

不是：

```text
x = xmin
```

这就是 FVM cell-centered 输出和 FDM grid-point 输出的差异。

## 5. ghost cell 边界

代码：

```c
q->rho[left_ghost] = q->rho[left_src];
q->rho[right_ghost] = q->rho[right_src];
```

物理意义：

```text
左 ghost cell 复制第一个物理 cell
右 ghost cell 复制最后一个物理 cell
```

这就是 transmissive 或 zero-gradient boundary。

在 FVM 中，ghost cell 的作用是让边界处也能构造界面 Riemann problem：

```text
左边界界面: ghost_L | first physical cell
右边界界面: last physical cell | ghost_R
```

## 6. interface_flux 的下标

代码约定：

```text
interface_flux[i] 表示 i 和 i+1 之间的界面通量
```

即：

```text
interface_flux[i] = Fhat_{i+1/2}
```

循环：

```c
for (int i = solver->grid.first - 1; i <= solver->grid.last; i++)
```

以 `nx=5` 为例：

```text
interface_flux[0] -> ghost 0 和 cell 1 之间的界面
interface_flux[1] -> cell 1 和 cell 2 之间的界面
interface_flux[2] -> cell 2 和 cell 3 之间的界面
interface_flux[3] -> cell 3 和 cell 4 之间的界面
interface_flux[4] -> cell 4 和 cell 5 之间的界面
interface_flux[5] -> cell 5 和 ghost 6 之间的界面
```

## 7. 更新公式中的下标

有限体积更新：

```text
Q_i^{n+1}
= Q_i^n - dt/dx * (Fhat_{i+1/2} - Fhat_{i-1/2})
```

代码写成：

```c
next[i] = current[i]
          - lambda * (interface_flux[i] - interface_flux[i - 1]);
```

因为：

```text
interface_flux[i]     是 cell i 的右界面
interface_flux[i - 1] 是 cell i 的左界面
```

这套下标是代码中最重要的逻辑。

## 8. Project 2 和 Project 3 的存储差异

### Project 2

典型语义：

```text
x_i = xmin + i*dx
dx = (xmax-xmin)/(nx-1)
Q_i 位于格点 x_i
F_i = F(Q_i)
用 F_i 做差分
```

它更像：

```text
grid-point finite difference storage
```

### Project 3

当前代码语义：

```text
dx = (xmax-xmin)/nx
x_i = xmin + (i+1/2)*dx
Q_i 是控制体平均守恒量
Fhat_{i+1/2} 是界面数值通量
用 Fhat 差做守恒更新
```

它是：

```text
cell-centered finite volume storage
```

## 9. 一句话记忆

```text
Project 2 的 F_i 住在格点上。
Project 3 的 Fhat_{i+1/2} 住在控制体界面上。
```

如果你把这句话理解清楚，Roe 代码下标基本就不会乱。

