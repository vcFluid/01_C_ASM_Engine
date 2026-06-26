# 数值流程速查

## 1. 连续方程

一维无粘 Euler equations：

```text
partial Q / partial t + partial F(Q) / partial x = 0
```

守恒量：

```text
Q = [rho, rho*u, rho*E]
```

通量：

```text
F = [
  rho*u,
  rho*u^2 + p,
  u*(rho*E + p)
]
```

理想气体 closure：

```text
p = (gamma-1) * [rho*E - rho*u^2/2]
a = sqrt(gamma*p/rho)
```

## 2. 为什么推进守恒量

激波是 discontinuous weak solution。Rankine-Hugoniot jump condition 来自 conservation
law 的积分形式，因此数值算法应围绕守恒量构造。

原始量 `rho/u/p` 更容易解释和画图，但不应在当前 MacCormack conservation form 中
作为独立主状态推进。

## 3. 一次时间步

```text
Q^n
 |
 | compute F(Q^n)
 v
Predictor:
Qbar_i = Q_i^n - dt/dx (F_{i+1}^n - F_i^n)
 |
 | apply boundary
 | compute F(Qbar)
 v
Corrector:
Qnext_i = 1/2 [Q_i^n + Qbar_i
               - dt/dx (Fbar_i - Fbar_{i-1})]
 |
 | artificial viscosity
 | apply boundary
 v
Q^(n+1)
```

## 4. CFL

Euler characteristic speeds：

```text
u-a, u, u+a
```

因此代码采用：

```text
dt = CFL * dx / max(|u|+a)
```

CFL 只约束显式传播速度。它不是稳定性的充分条件，尤其不能自动处理：

- shock oscillation；
- negative density/pressure；
- vacuum；
- 不合理 boundary；
- artificial viscosity 过弱或过强。

## 5. 人工粘性

局部 switch：

```text
epsilon_i =
beta * |phi_{i+1}-2phi_i+phi_{i-1}|
     / (|phi_{i+1}|+2|phi_i|+|phi_{i-1}|+delta)
```

修正：

```text
Qnext_i += epsilon_i (Q_{i+1}-2Q_i+Q_{i-1})
```

效果：

- beta 太小：间断附近 dispersive oscillation 仍强，甚至失稳；
- beta 增大：振荡减弱；
- beta 太大：shock/contact 被过度抹宽，误差可能重新增加。

不同变量对 beta 的最优值可能不同。因此不能只看一张密度图宣布“最佳 beta”。

## 6. 边界

当前：

```text
Q_0 = Q_1
Q_{N-1} = Q_{N-2}
```

这是离散 zero-gradient/transmissive approximation。只有在主要波系尚未到达边界时，
shock-tube 结果才较少受边界影响。

## 7. 误差解释

间断问题中的点值 `Linf` 对间断位置和网格采样高度敏感。建议同时报告：

```text
L1, L2, Linf
total variation
undershoot / overshoot
shock/contact thickness
```

对于包含 shock 的解，global convergence order 通常低于光滑区的形式阶数。

## 8. 当前程序有意保留的风险

- 没有 positivity-preserving update；
- `rho<=0` 或 `p<=0` 后可能产生 `NaN`；
- `a=0` fallback 不是物理修复；
- 200000 步是 emergency cap，不是收敛判据；
- Sjogreen expansion 可能涉及 vacuum，Project 0 和 Project 2 都需要额外审查；
- `system()` coupling 适合教学验证，不适合高频、大规模生产计算。

