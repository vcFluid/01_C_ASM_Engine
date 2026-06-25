# Boss Battles

目标：用完整 benchmark case 检验数学一致性、物理合理性、数值稳定性和工程实现。总 XP：2500。

| ID | Project | Folder | XP | Unlock |
|---|---|---|---:|---|
| BOSS-01 | 2D lid-driven cavity | `src/BOSS-01_cavity/` | 500 | Lv.7 |
| BOSS-02 | 2D Taylor–Green vortex | `src/BOSS-02_taylor_green/` | 550 | Lv.6–7 |
| BOSS-03 | 2D Poiseuille flow | `src/BOSS-03_poiseuille/` | 450 | Lv.4–7 |
| BOSS-04 | Laminar backward-facing step | `src/BOSS-04_backward_step/` | 600 | BOSS-01 或 03 |
| BOSS-05 | Basilisk operator comparison | `src/BOSS-05_basilisk_compare/` | 400 | 任一 verified operator |

## 通用交付物

每个 Boss 项目必须包含：

- `README.md`：equations、domain、parameters、BC、discretization。
- `src/`：可复现源码。
- `results/`：精简后的数据和图，不提交无关大文件。
- `verification.md`：reference data、norm、grid/time refinement。
- `run.md`：compiler、command、platform 和 runtime。

## Boss 验收重点

- BOSS-01：centerline velocity、steady criterion、divergence 和 grid dependence。
- BOSS-02：空间/时间 observed order 与 kinetic-energy decay。
- BOSS-03：解析 velocity profile、flow rate、wall shear、pressure gradient。
- BOSS-04：reattachment length、mesh sensitivity；保持 laminar scope。
- BOSS-05：相同 PDE、domain、BC 和 resolution 下比较，不能只比较图片外观。

