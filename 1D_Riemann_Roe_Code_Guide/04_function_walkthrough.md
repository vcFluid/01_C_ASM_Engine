# 04. 函数级 walkthrough

本文件按代码顺序解释每组函数的作用。

## 1. 网格与内存管理

### `grid_finalize(Grid1D *grid)`

作用：

```text
根据 nx 和 NGHOST 生成 first, last, ntotal, dx
```

关键：

```text
dx = (xmax-xmin)/nx
```

这体现 cell-centered FVM。

### `cell_left_edge_x()` 和 `cell_center_x()`

作用：

```text
把数组下标 i 转成几何坐标
```

`cell_center_x()` 用于 Tecplot 输出。

### `alloc_double_array()`

作用：

```text
calloc 一段 double 数组
```

这使数组初值为 0，减少未初始化变量风险。

### `conserved_alloc()`, `flux_alloc()`, `primitive_alloc()`

作用：

```text
分别为 Q、Fhat、W 分配数组
```

这里传入的长度是 `ntotal`，所以 ghost cell 也有空间。

### `solver_allocate()`

作用：

```text
一次性分配 solver 运行需要的所有数组
```

如果任何一个分配失败，返回 0。

### `solver_destroy()`

作用：

```text
释放所有堆内存
```

这是 C 代码必须做的清理步骤。

## 2. 物理量转换

### `pressure_from_conserved()`

公式：

```text
p = (gamma-1)*(rhoE - 0.5*rhou^2/rho)
```

它是从守恒量反算压强的核心函数。

### `total_energy_density()`

公式：

```text
rhoE = p/(gamma-1) + 0.5*rho*u^2
```

用于把 Riemann 初始原始量转成守恒量。

### `conserved_from_primitive_state()`

作用：

```text
[rho,u,p] -> [rho,rho*u,rhoE]
```

初始化左右状态时使用。

### `conserved_at()`

作用：

```text
从数组中取出第 i 个 cell 的 Q
```

Roe flux 需要把左/右状态作为单个 `Conserved` 传入。

### `primitive_from_conserved()`

作用：

```text
Q -> W
```

额外计算：

```text
E = rhoE/rho
H = (rhoE+p)/rho
a = sqrt(gamma*p/rho)
```

其中 `H` 和 `a` 是 Roe average 的关键。

### `physical_flux_from_conserved()`

作用：

```text
由 Q 计算物理通量 F(Q)
```

Roe 通量需要左右物理通量：

```text
FL = F(Q_L)
FR = F(Q_R)
```

## 3. CFL 与边界

### `compute_primitive_from_current()`

作用：

```text
把 current 中所有物理 cell 的 Q 反算成 primitive 数组
```

只处理：

```text
i = first ... last
```

不处理 ghost cell。

### `compute_max_wave_speed()`

作用：

```text
计算 max(|u|+a)
```

这是显式格式 CFL 条件中的最大波速。

### `compute_dt_from_cfl()`

公式：

```text
dt = CFL * dx / max(|u|+a)
```

### `apply_zero_gradient_boundary()`

作用：

```text
ghost cell 复制相邻物理 cell
```

这让边界也可以构造 Roe interface flux。

### `conserved_state_is_physical()`

作用：

```text
检查 rho, rhou, rhoE 是否 finite
检查 rho > floor
检查 p > floor
```

如果失败，solver 停止。

这是为了记录 Roe 格式的不稳定/非物理状态，而不是悄悄修补。

## 4. Roe 核心

### 特征值绝对值

`compute_roe_flux()` 中直接使用 `fabs()` 计算 Roe 特征值绝对值：

```text
|u_tilde|
|u_tilde + a_tilde|
|u_tilde - a_tilde|
```

输出：

```text
修正后的 |lambda|
```

### `compute_roe_flux()`

这是最重要的函数。

输入：

```text
qL  界面左侧状态
qR  界面右侧状态
```

输出：

```text
fhat 界面数值通量
```

内部流程：

```text
1. qL,qR -> WL,WR
2. 计算 FL,FR
3. 计算 Roe average: u_tilde, H_tilde, a_tilde
4. 计算 drho, du, dp
5. 按课件公式计算 alpha1, alpha2, alpha3
6. 计算 alpha4 = alpha1 + alpha2 + alpha3
7. 计算 alpha5 = a_tilde*(alpha2-alpha3)
8. 显式构造 |A_tilde|DeltaQ 的三个分量
9. 组装 Fhat
10. 检查 Fhat 是否 finite
```

判断它是否正确，可以看最终是否符合：

```text
Fhat = 0.5*(FL+FR) - 0.5*(|A_tilde|DeltaQ)
```

### `compute_all_roe_fluxes()`

作用：

```text
对所有界面调用 compute_roe_flux()
```

循环范围：

```text
i = first-1 ... last
```

这包括左边界界面和右边界界面。

### `advance_one_roe_step()`

这是一个完整时间步。

结构：

```text
dt
boundary
physical check
all Roe fluxes
finite volume update
boundary
physical check
copy next -> current
t and step_count update
```

如果你只背一个函数，就背这个。

## 5. 初始化与输出

### `initialize_riemann_problem()`

作用：

```text
生成 Q^0
```

如果某个 cell 完全在 `x0` 左侧：

```text
Q_i = Q_L
```

如果完全在右侧：

```text
Q_i = Q_R
```

如果 `x0` 穿过该 cell：

```text
Q_i = theta*Q_L + (1-theta)*Q_R
```

这里平均的是守恒量，不是原始量。这是 FVM 中更正确的初始化。

### `write_tecplot()`

作用：

```text
输出 Tecplot ASCII 文件
```

输出列：

```text
x, rho, u, p, E, rho_conserved, rhou, rhoE
```

注意：

```text
x 是 cell center 坐标
ghost cell 不输出
```

## 6. 配置与文件名

### `solver_create_default()`

设置默认参数：

```text
Sod case
nx = 501
domain = [0,1]
x0 = 0.5
gamma = 1.4
CFL = 0.5
```

### `configure_riemann_case()`

选择 7 个经典 Riemann case。

与 Project 2 保持一致，方便对比。

### `configure_domain_and_grid()`

读入：

```text
xmin, xmax, x0, nx
```

最后调用：

```text
grid_finalize()
```

### `configure_numerics()`

读入：

```text
gamma
CFL
t_max
snapshot interval
```

## 7. exact solver

### `find_exact_solver()`

寻找精确解程序：

```text
本地 _Analysical_Solution_Solver/riemann_exact.exe
Project_2/_Analysical_Solution_Solver/riemann_exact.exe
```

### `run_exact_solver()`

用 `system(command)` 调用外部程序。

传入：

```text
left state
right state
gamma
xmin, xmax, x0
nx
output_time
exact_filename
```

注意：exact solver 的采样点可能是端点网格。后处理脚本会把 exact 解插值到 Roe numerical cell-center 网格。

## 8. main()

`main()` 是所有模块的组织者，不直接写数值格式公式。

它只负责：

```text
创建 solver
读入配置
分配内存
初始化
时间循环
输出
释放内存
```

真正的数值格式封装在：

```text
advance_one_roe_step()
```

这就是良好的 solver 结构：外层 workflow 和内层 numerical kernel 分离。
