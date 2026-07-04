# Case 6 Contact Double Shock ????

?????????????????????????????????

## 1. ????

```text
runs/Solution_06_Contact_Double_Shock/case6_contact_double_shock_study
  baseline/
  matrix/
  beta_sweep_profiles/ ?????? beta_profiles/
```

## 2. ????

```text
case   = 6
sensor = rho
beta   = 0.5
CFL    = 0.5
Nx     = 501
mode   = 1
```

## 3. ????

| variable | L1 | L2 | Linf |
|---|---|---|---|
| rho | 1.050757e-02 | 5.256369e-02 | 4.361527e-01 |
| u | 6.922352e-03 | 5.017774e-02 | 5.302812e-01 |
| p | 8.577551e-03 | 6.053528e-02 | 5.999013e-01 |

## 4. ????????

| status | count |
|---|---|
| completed | 192 |

## 5. ???????

| status | count |
|---|---|
| completed | 12 |

## 6. beta profile, rho

| beta | status | rho L1 | rho Linf |
|---|---|---|---|
| 0.0 | completed | 8.252312e-03 | 3.737348e-01 |
| 0.25 | completed | 8.653349e-03 | 4.347199e-01 |
| 0.5 | completed | 1.050757e-02 | 4.361527e-01 |
| 0.75 | completed | 1.129252e-02 | 4.303786e-01 |
| 1.0 | completed | 1.173938e-02 | 4.262740e-01 |
| 1.5 | completed | 1.245590e-02 | 4.203771e-01 |

## 7. ??? L1 ????

| variable | L1 | viscosity | sensor | beta | CFL | Nx |
|---|---|---|---|---|---|---|
| rho | 3.792424e-03 | on | p | 0.25 | 0.8 | 1001 |
| u | 1.562649e-03 | off | none | 0.0 | 0.8 | 1001 |
| p | 2.162916e-03 | off | none | 0.0 | 0.8 | 1001 |

## 8. ????????

- ????????????????????
- ????????????????????????
