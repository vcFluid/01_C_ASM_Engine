# Case 5 Contact Double Expansion ????

????????????????????????????

## 1. ????

```text
runs/Solution_05_Contact_Double_Expansion/case5_contact_double_expansion_study
  baseline/
  matrix/
  beta_sweep_profiles/ ?????? beta_profiles/
```

## 2. ????

```text
case   = 5
sensor = rho
beta   = 0.5
CFL    = 0.5
Nx     = 501
mode   = 1
```

## 3. ????

| variable | L1 | L2 | Linf |
|---|---|---|---|
| rho | 3.277088e-03 | 1.400380e-02 | 1.634097e-01 |
| u | 2.346795e-03 | 5.549061e-03 | 2.465290e-02 |
| p | 1.269157e-03 | 3.013473e-03 | 1.724590e-02 |

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
| 0.0 | completed | 5.092842e-03 | 1.995542e-01 |
| 0.25 | completed | 2.864183e-03 | 1.739929e-01 |
| 0.5 | completed | 3.277088e-03 | 1.634097e-01 |
| 0.75 | completed | 3.686960e-03 | 1.586801e-01 |
| 1.0 | completed | 4.063865e-03 | 1.568341e-01 |
| 1.5 | completed | 4.702189e-03 | 1.615798e-01 |

## 7. ??? L1 ????

| variable | L1 | viscosity | sensor | beta | CFL | Nx |
|---|---|---|---|---|---|---|
| rho | 1.335311e-03 | on | u | 0.5 | 0.8 | 1001 |
| u | 1.034653e-03 | on | rho | 0.75 | 0.8 | 1001 |
| p | 5.548270e-04 | on | rho | 0.5 | 0.8 | 1001 |

## 8. ????????

- ????????????????????????
- ? beta ??????????????????????? overshoot ???
