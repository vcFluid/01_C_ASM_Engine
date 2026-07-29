# 03. Roe-FVM 迭代方法

本文件解释数值方法本身。

## 1. 控制方程

一维 Euler 方程写成守恒律：

```text
partial Q / partial t + partial F(Q) / partial x = 0
```

其中：

```text
Q = [rho, rho*u, rho*E]^T
F = [rho*u, rho*u^2 + p, u*(rhoE+p)]^T
```

理想气体关系：

```text
p = (gamma - 1) * (rhoE - 0.5*rho*u^2)
```

声速：

```text
a = sqrt(gamma*p/rho)
```

总焓：

```text
H = (rhoE+p)/rho
```

## 2. 有限体积离散

对第 \(i\) 个控制体积分：

```text
d/dt Q_i = -1/dx * (F_{i+1/2} - F_{i-1/2})
```

显式 Euler 时间推进：

```text
Q_i^{n+1}
= Q_i^n - dt/dx * (Fhat_{i+1/2} - Fhat_{i-1/2})
```

真正难点是：界面通量 \(Fhat_{i+1/2}\) 怎么算？

Godunov 思想：

```text
每个界面都有左右两个常值状态
把这个界面看成一个局部 Riemann 问题
用该 Riemann 问题在界面上的通量推进控制体平均量
```

Roe 方法是这个局部 Riemann 问题的近似解法。

## 3. Roe 通量结构

课件 Roe 通量公式：

```text
Fhat
= 0.5*(F_L + F_R)
 - 0.5*(|A_tilde| Delta Q)
```

理解方式：

```text
0.5*(F_L+F_R)             中心通量
|A_tilde| Delta Q         Roe 矩阵给出的迎风耗散
```

如果没有第二项，格式会像中心格式，激波附近容易振荡。

Roe 的关键是：

```text
用 Roe average 构造一个局部线性化矩阵 A_tilde
让 F_R - F_L 约等于 A_tilde*(Q_R - Q_L)
然后对 A_tilde 做特征分解
```

当前代码严格按课件展开式实现，没有显式构造完整矩阵 `A_tilde`，而是直接构造：

```text
|A_tilde| Delta Q =
[ alpha4,
  u_tilde*alpha4 + alpha5,
  H_tilde*alpha4 + u_tilde*alpha5
      - a_tilde^2*alpha1/(gamma-1) ]^T
```

## 4. Roe average

左右状态：

```text
W_L = [rho_L, u_L, p_L]
W_R = [rho_R, u_R, p_R]
```

先计算：

```text
s_L = sqrt(rho_L)
s_R = sqrt(rho_R)
denom = s_L + s_R
```

Roe 平均速度：

```text
u_tilde = (s_L*u_L + s_R*u_R) / denom
```

Roe 平均总焓：

```text
H_tilde = (s_L*H_L + s_R*H_R) / denom
```

Roe 平均声速：

```text
a_tilde^2 = (gamma-1)*(H_tilde - 0.5*u_tilde^2)
a_tilde = sqrt(a_tilde^2)
```

代码位置：

```text
compute_roe_flux()
```

## 5. 特征值

一维 Euler 的三条特征速度：

```text
lambda_1 = u_tilde - a_tilde
lambda_2 = u_tilde
lambda_3 = u_tilde + a_tilde
```

物理含义：

```text
lambda_1 左声波
lambda_2 接触波
lambda_3 右声波
```

Roe 通量使用的是绝对值：

```text
|lambda_k|
```

这就是 upwind 的来源。

## 6. 特征值绝对值

当前代码严格按课件 Roe 原式计算三个特征值绝对值：

```text
abs_u         = |u_tilde|
abs_u_plus_a  = |u_tilde + a_tilde|
abs_u_minus_a = |u_tilde - a_tilde|
```

也就是 C 代码中的：

```c
abs_u = fabs(u_tilde);
abs_u_plus_a = fabs(u_tilde + a_tilde);
abs_u_minus_a = fabs(u_tilde - a_tilde);
```

这些绝对值进入 `alpha1...alpha3`，构成 Roe 通量中的迎风耗散。

## 7. 课件 alpha1...alpha5

左右状态跳跃：

```text
drho = rho_R - rho_L
du   = u_R - u_L
dp   = p_R - p_L
```

代码中的 alpha 定义与课件一致：

```text
alpha1 = |u_tilde| * (drho - dp/a_tilde^2)
alpha2 = |u_tilde+a_tilde|/(2*a_tilde^2)
         * (dp + rho_tilde*a_tilde*du)
alpha3 = |u_tilde-a_tilde|/(2*a_tilde^2)
         * (dp - rho_tilde*a_tilde*du)
alpha4 = alpha1 + alpha2 + alpha3
alpha5 = a_tilde * (alpha2 - alpha3)
```

其中：

```text
alpha1 对应接触波相关耗散项
alpha2 对应右行声波相关耗散项
alpha3 对应左行声波相关耗散项
alpha4 和 alpha5 是课件为了压缩 |A_tilde|DeltaQ 三个分量引入的组合量
```

## 8. `|A_tilde| Delta Q` 展开

代码直接构造：

```text
diss_mass = alpha4
diss_momentum = u_tilde*alpha4 + alpha5
diss_energy = H_tilde*alpha4 + u_tilde*alpha5
              - a_tilde^2*alpha1/(gamma-1)
```

然后：

```text
Fhat_mass     = 0.5*(FL_mass+FR_mass)         - 0.5*diss_mass
Fhat_momentum = 0.5*(FL_momentum+FR_momentum) - 0.5*diss_momentum
Fhat_energy   = 0.5*(FL_energy+FR_energy)     - 0.5*diss_energy
```

最后：

```text
Fhat = 0.5*(FL+FR) - 0.5*dissipation
```

## 9. 一个完整时间步

代码中的 `advance_one_roe_step()` 做：

```text
1. dt = CFL * dx / max(|u|+a)
2. 如果 t+dt 超过 t_max，缩短最后一步 dt
3. 对 current 补 ghost cell
4. 检查 current 是否物理
5. 对每个界面计算 Roe flux
6. 对每个物理 cell 执行 FVM 更新
7. 对 next 补 ghost cell
8. 检查 next 是否物理
9. next 复制回 current
10. t += dt, step_count += 1
```

最核心代码：

```c
solver->next.rho[i] =
    solver->current.rho[i] -
    lambda * (solver->interface_flux.mass[i] -
              solver->interface_flux.mass[i - 1]);
```

三条守恒方程分别更新：

```text
rho
rho*u
rho*E
```

## 10. 为什么这是 FVM

判断是不是 FVM，不看是不是写了 Roe，而看三个条件：

```text
未知量是否表示控制体平均守恒量
更新是否来自左右界面通量差
界面通量是否由左右状态构造
```

当前代码满足：

```text
Q_i 是 cell-centered 控制体状态
Fhat_i 是界面通量
Q_i^{n+1}=Q_i^n-dt/dx*(Fhat_i-Fhat_{i-1})
```

因此它是 Godunov-type FVM，Roe 是 approximate Riemann solver。

## 11. Roe 格式的风险

Roe 格式的优点：

```text
结构清楚
接触间断分辨率较好
波传播方向由特征分解决定
比精确 Riemann solver 便宜
```

风险：

```text
不保证 positivity，强膨胀或近真空可能负压/负密度
一阶格式仍然会扩散间断
```

所以结果报告中 case 4 failure 不应该被简单当成 bug，而应讨论为 Roe baseline 的方法局限。
