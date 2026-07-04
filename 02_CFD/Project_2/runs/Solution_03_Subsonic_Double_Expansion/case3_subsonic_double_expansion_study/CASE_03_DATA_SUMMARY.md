# Case 3 Subsonic Double Expansion ????

?? CFL=0.5 ?? predictor ?????????????????? CFL=0.2 ???????

## 1. ????

```text
runs/Solution_03_Subsonic_Double_Expansion/case3_subsonic_double_expansion_study
  baseline/
  matrix/
  beta_sweep_profiles/ ?????? beta_profiles/
```

## 2. ????

```text
case   = 3
sensor = rho
beta   = 0.5
CFL    = 0.2
Nx     = 501
mode   = 1
```

## 3. ????

| variable | L1 | L2 | Linf |
|---|---|---|---|
| rho | 1.656326e-03 | 2.629560e-03 | 8.544744e-03 |
| u | 6.557940e-03 | 1.100992e-02 | 4.197867e-02 |
| p | 6.834567e-03 | 1.122252e-02 | 3.325253e-02 |

## 4. ????????

| status | count |
|---|---|
| completed | 72 |
| failed | 120 |

## 5. ???????

| status | count |
|---|---|
| failed | 12 |

## 6. beta profile, rho

| beta | status | rho L1 | rho Linf |
|---|---|---|---|
| 0.5 | completed | 1.656326e-03 | 8.544744e-03 |
| 0.75 | completed | 2.173418e-03 | 1.091788e-02 |
| 1.0 | completed | 2.641771e-03 | 1.296889e-02 |
| 1.5 | completed | 3.436642e-03 | 1.589433e-02 |

## 7. ??? L1 ????

| variable | L1 | viscosity | sensor | beta | CFL | Nx |
|---|---|---|---|---|---|---|
| rho | 8.425699e-04 | on | rho | 0.5 | 0.2 | 1001 |
| u | 3.087887e-03 | on | u | 0.5 | 0.2 | 1001 |
| p | 3.200836e-03 | on | u | 0.5 | 0.2 | 1001 |

## 8. ????????

- ??????????????CFL=0.5 ????????CFL=0.2 ???????
- beta ?????????????????????????
