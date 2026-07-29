# 05. 调试检查与常见误区

## 1. Roe 不是把 Project 2 的 F(Q_i) 换个公式

错误理解：

```text
Project 2 里有 F(Q_i)
Project 3 只要换成 Roe 的 F(Q_i) 就行
```

正确理解：

```text
Roe 输入的是左右状态 (Q_L,Q_R)
输出的是界面通量 Fhat_{i+1/2}
```

也就是：

```text
(Q_i, Q_{i+1}) -> Fhat_{i+1/2}
```

不是：

```text
Q_i -> F_i
```

## 2. FDM 格点通量和 FVM 界面通量

Project 2 更接近：

```text
Q_i at grid point x_i
F_i = F(Q_i)
```

Project 3 是：

```text
Q_i as cell average
Fhat_{i+1/2} at cell interface
```

如果你看到：

```c
interface_flux[i] - interface_flux[i - 1]
```

它不是普通中心差分，而是有限体积左右界面通量差。

## 3. 为什么 dx 改成 `(xmax-xmin)/nx`

FDM 节点网格常见：

```text
dx = (xmax-xmin)/(nx-1)
```

因为 `nx` 个点包含两个端点。

FVM cell-centered 网格：

```text
dx = (xmax-xmin)/nx
```

因为 `nx` 表示控制体数量。

第一个 cell center 是：

```text
xmin + 0.5*dx
```

最后一个 cell center 是：

```text
xmax - 0.5*dx
```

## 4. 为什么需要 ghost cell

如果没有 ghost cell，边界处没有左右状态构造 Riemann problem。

左边界界面需要：

```text
Q_ghost_left, Q_first
```

右边界界面需要：

```text
Q_last, Q_ghost_right
```

当前代码使用零梯度边界：

```text
Q_ghost_left = Q_first
Q_ghost_right = Q_last
```

这相当于 transmissive boundary。

## 5. 为什么初始化时要 average conserved variables

如果间断 `x0` 穿过某个控制体，FVM 的 unknown 是 cell average。

所以应该做：

```text
Q_i = theta*Q_L + (1-theta)*Q_R
```

不是：

```text
rho_i = theta*rho_L + ...
u_i   = theta*u_L + ...
p_i   = theta*p_L + ...
```

因为 Euler 方程守恒的是 \(Q\)，不是 \(W\)。

## 6. 为什么 case 4 可能失败

case 4 是强膨胀问题：

```text
left:  rho=1, u=-2, p=0.4
right: rho=1, u= 2, p=0.4
```

这种问题容易形成低密度/低压力区域。经典 Roe solver 不保证 positivity。

所以可能出现：

```text
p <= 0
```

这不一定是代码 bug，而是 Roe baseline 的方法限制。

Roe 原式不保证：

```text
rho > 0
p > 0
```

如果未来要稳定 case 4，需要考虑：

```text
HLLE/HLLC positivity-preserving flux
Roe-HLLE hybrid
更小 CFL
pressure/density floor 修补，但这会改变守恒性和物理解释
```

## 7. 如何检查 Roe 更新是否写对

检查 1：界面循环范围。

```text
i = first-1 ... last
```

如果少了 `first-1`，第一个物理 cell 没有左界面通量。

如果少了 `last`，最后一个物理 cell 没有右界面通量。

检查 2：cell 更新范围。

```text
i = first ... last
```

不能更新 ghost cell。

检查 3：右界面和左界面。

```text
right flux = interface_flux[i]
left flux  = interface_flux[i-1]
```

检查 4：守恒量三个分量都用同一个通量差结构。

```text
rho  uses mass flux
rhou uses momentum flux
rhoE uses energy flux
```

## 8. 如何检查输出是否是 cell-centered

看 Sod 输出的第一个 x。

如果 `xmin=0`, `xmax=1`, `nx=501`：

```text
dx = 1/501 = 0.001996...
first center = 0.000998...
```

如果第一个点是 0，说明仍是节点式输出。

当前代码应输出：

```text
0.000998...
```

## 9. exact 解为什么要插值

Roe 数值解在 cell center：

```text
x = xmin + (i+0.5)*dx
```

exact solver 可能输出端点采样：

```text
x = xmin + i*(xmax-xmin)/(nx-1)
```

两者坐标不完全一致，所以误差统计应该：

```text
把 exact 解插值到 numerical 的 x 网格上
再计算 L1/L2/Linf
```

这正是 postprocess 中 `targeted_matrix.py` 的做法。

## 10. 编译检查

推荐命令：

```powershell
gcc 1-D_Riemann_Roe.c -std=c11 -O2 -Wall -Wextra -o riemann_roe.exe -lm
```

如果出现链接错误，重点检查是否漏了：

```text
-lm
```

因为 `sqrt()` 和 `fabs()` 来自数学库。

## 11. 读代码时的最短路径

如果你时间很少，按这个顺序读：

```text
Grid1D
Solver
initialize_riemann_problem
advance_one_roe_step
compute_all_roe_fluxes
compute_roe_flux
write_tecplot
main
```

如果你能把这 8 个部分串起来，整份代码的骨架就掌握了。
