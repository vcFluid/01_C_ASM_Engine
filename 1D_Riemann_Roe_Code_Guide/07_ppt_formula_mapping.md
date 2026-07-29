# 07. 课件 Roe 公式与当前代码的一一对应

当前代码已经按图片中的课件公式显式实现 Roe 界面通量。

也就是说，`compute_roe_flux()` 不再使用

```text
sum_k |lambda_k| beta_k r_k
```

这种等价的特征向量求和写法，而是直接使用课件中的：

```text
Fhat = 1/2*(F_L + F_R) - 1/2*(|A_tilde| Delta Q)
```

以及：

```text
|A_tilde| Delta Q =
[ alpha4,
  u_tilde*alpha4 + alpha5,
  H_tilde*alpha4 + u_tilde*alpha5
      - c_tilde^2*alpha1/(gamma-1) ]^T
```

## 1. FVM 更新公式

课件：

```text
Q_j^{n+1}
= Q_j^n - dt/dx * (Fhat_{j+1/2} - Fhat_{j-1/2})
```

代码：

```c
solver->next.rho[i] =
    solver->current.rho[i] -
    lambda * (solver->interface_flux.mass[i] -
              solver->interface_flux.mass[i - 1]);
```

其中：

```text
lambda = dt/dx
interface_flux[i]     = Fhat_{i+1/2}
interface_flux[i - 1] = Fhat_{i-1/2}
```

`rhou` 和 `rhoE` 的更新完全同型。

## 2. 界面左右状态

课件：

```text
Q_L,j+1/2
Q_R,j+1/2
```

当前代码是一阶 piecewise constant reconstruction：

```c
Conserved qL = conserved_at(&solver->current, i);
Conserved qR = conserved_at(&solver->current, i + 1);
```

所以：

```text
Q_L,j+1/2 = Q_j
Q_R,j+1/2 = Q_{j+1}
```

如果以后做二阶 MUSCL，才需要额外重构界面左右状态。

## 3. Roe 平均

代码：

```c
u_tilde = (sqrt_rho_L * WL.u + sqrt_rho_R * WR.u) / denom;
H_tilde = (sqrt_rho_L * WL.H + sqrt_rho_R * WR.H) / denom;
a2_tilde = (gamma - 1.0) * (H_tilde - 0.5 * u_tilde * u_tilde);
a_tilde = sqrt(a2_tilde);
rho_tilde = sqrt_rho_L * sqrt_rho_R;
```

课件符号对应：

```text
u_tilde    -> u~
H_tilde    -> H~
a_tilde    -> c~
rho_tilde  -> rho~
```

## 4. 课件 alpha1...alpha5

代码现在直接使用课件中的 `alpha1...alpha5`：

```c
alpha1 = abs_u * (drho - dp / a2_tilde);
alpha2 = 0.5 * abs_u_plus_a *
         (dp + rho_tilde * a_tilde * du) / a2_tilde;
alpha3 = 0.5 * abs_u_minus_a *
         (dp - rho_tilde * a_tilde * du) / a2_tilde;
alpha4 = alpha1 + alpha2 + alpha3;
alpha5 = a_tilde * (alpha2 - alpha3);
```

对应课件：

```text
alpha1 = |u~| (Delta rho - Delta p / c~^2)

alpha2 = |u~+c~|/(2 c~^2)
          * (Delta p + rho~ c~ Delta u)

alpha3 = |u~-c~|/(2 c~^2)
          * (Delta p - rho~ c~ Delta u)

alpha4 = alpha1 + alpha2 + alpha3

alpha5 = c~(alpha2 - alpha3)
```

注意：

```text
drho = rho_R - rho_L
du   = u_R - u_L
dp   = p_R - p_L
```

这和图片中 `Delta(.) = (.)_R - (.)_L` 一致。

## 5. `|A_tilde| Delta Q` 三个分量

代码：

```c
diss_mass = alpha4;
diss_momentum = u_tilde * alpha4 + alpha5;
diss_energy = H_tilde * alpha4 + u_tilde * alpha5 -
              a2_tilde * alpha1 / (gamma - 1.0);
```

对应课件：

```text
|A_tilde| Delta Q =
[ alpha4,
  u~ alpha4 + alpha5,
  H~ alpha4 + u~ alpha5 - c~^2 alpha1/(gamma-1) ]^T
```

## 6. 数值通量组装

代码：

```c
fhat->mass =
    0.5 * (FL.mass + FR.mass) - 0.5 * diss_mass;

fhat->momentum =
    0.5 * (FL.momentum + FR.momentum) - 0.5 * diss_momentum;

fhat->energy =
    0.5 * (FL.energy + FR.energy) - 0.5 * diss_energy;
```

对应课件：

```text
Fhat = 1/2*(F_L + F_R) - 1/2*(|A_tilde| Delta Q)
```

## 8. 最终判断

当前代码符合课件逻辑：

```text
yes: Q_j^{n+1} = Q_j^n - dt/dx*(Fhat_{j+1/2}-Fhat_{j-1/2})
yes: Fhat = 1/2*(F_L+F_R) - 1/2*|A_tilde|DeltaQ
yes: alpha1...alpha5 显式展开
yes: Delta(.) = right - left
yes: 特征值绝对值直接使用课件原式
```
