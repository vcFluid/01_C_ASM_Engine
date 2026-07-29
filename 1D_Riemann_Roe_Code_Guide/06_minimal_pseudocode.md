# 06. 最小伪代码

本文件把 `1-D_Riemann_Roe.c` 抽象成最小伪代码。

## 1. 总程序

```text
create default solver

read case id
read domain and grid
read gamma, CFL, t_max
read output filename

allocate arrays with length ntotal = nx + 2*nghost

initialize Q^0 as cell averages
apply boundary

while t < t_max:
    advance_one_roe_step()

write numerical solution
run exact solver
free arrays
```

## 2. 初始化

```text
Q_L = primitive_to_conserved(left_state)
Q_R = primitive_to_conserved(right_state)

for each physical cell i:
    xl = left edge of cell i
    xr = right edge of cell i

    if xr <= x0:
        Q_i = Q_L
    else if xl >= x0:
        Q_i = Q_R
    else:
        theta = (x0 - xl)/dx
        Q_i = theta*Q_L + (1-theta)*Q_R

apply boundary
compute primitive variables
```

## 3. 一个 Roe-FVM 时间步

```text
function advance_one_roe_step:

    max_speed = max_i(|u_i| + a_i)
    dt = CFL * dx / max_speed
    if t + dt > t_max:
        dt = t_max - t

    apply boundary to current
    check current physical

    for each interface i = first-1 ... last:
        qL = current[i]
        qR = current[i+1]
        interface_flux[i] = roe_flux(qL, qR)

    lambda = dt/dx

    for each physical cell i = first ... last:
        next[i] =
            current[i]
            - lambda * (interface_flux[i] - interface_flux[i-1])

    apply boundary to next
    check next physical

    current = next
    t = t + dt
    step_count = step_count + 1
```

## 4. 一个界面的 Roe flux

```text
function roe_flux(qL, qR):

    WL = conserved_to_primitive(qL)
    WR = conserved_to_primitive(qR)

    if WL or WR nonphysical:
        fail

    FL = physical_flux(qL)
    FR = physical_flux(qR)

    sL = sqrt(WL.rho)
    sR = sqrt(WR.rho)
    denom = sL + sR

    u_tilde = (sL*WL.u + sR*WR.u)/denom
    H_tilde = (sL*WL.H + sR*WR.H)/denom
    a2_tilde = (gamma-1)*(H_tilde - 0.5*u_tilde^2)
    a_tilde = sqrt(a2_tilde)
    rho_tilde = sL*sR

    drho = WR.rho - WL.rho
    du   = WR.u   - WL.u
    dp   = WR.p   - WL.p

    abs_u         = abs(u_tilde)
    abs_u_plus_a  = abs(u_tilde + a_tilde)
    abs_u_minus_a = abs(u_tilde - a_tilde)

    alpha1 = abs_u * (drho - dp/a2_tilde)
    alpha2 = 0.5*abs_u_plus_a
             * (dp + rho_tilde*a_tilde*du)/a2_tilde
    alpha3 = 0.5*abs_u_minus_a
             * (dp - rho_tilde*a_tilde*du)/a2_tilde
    alpha4 = alpha1 + alpha2 + alpha3
    alpha5 = a_tilde*(alpha2 - alpha3)

    diss_mass = alpha4
    diss_momentum = u_tilde*alpha4 + alpha5
    diss_energy = H_tilde*alpha4 + u_tilde*alpha5
                  - a2_tilde*alpha1/(gamma-1)

    Fhat = 0.5*(FL+FR)
           - 0.5*[diss_mass, diss_momentum, diss_energy]

    return Fhat
```

## 5. 特征值绝对值

```text
abs_u         = abs(u_tilde)
abs_u_plus_a  = abs(u_tilde + a_tilde)
abs_u_minus_a = abs(u_tilde - a_tilde)
```

## 6. 最核心的三行

如果要把整份代码压缩成三行，就是：

```text
Fhat_{i+1/2} = Roe(Q_i, Q_{i+1})
Q_i^{n+1} = Q_i^n - dt/dx*(Fhat_{i+1/2} - Fhat_{i-1/2})
repeat until t = t_max
```

这就是 Project 3 的数值心脏。
