# Python CFD Questline

定位：Python 不替代 C numerical kernel，而是承担快速原型、verification、数据处理、可视化、自动化和测试。

## 游戏规则

- 每次完成一个 30–90 分钟的 mission。
- 状态：`[ ]` 未开始，`[>]` 进行中，`[!]` 可运行但验证不足，`[x]` 已验收。
- 源码放在各 Level 的 `src/`，测试放在 `tests/`，笔记放在 `notes/`，生成数据放在 `results/`。
- CFD 任务必须区分 implementation error、discretization error 和 iterative error。
- 禁止只看图判断正确；至少报告一个 error、residual 或 conservation metric。

## Quest Map

| Level | 主题 | Missions | XP | 解锁条件 |
|---|---|---:|---:|---|
| Lv.0 | Environment & reproducibility | 5 | 350 | 无 |
| Lv.1 | Python scientific fundamentals | 7 | 650 | Lv.0 |
| Lv.2 | NumPy field operations | 7 | 800 | Lv.1 |
| Lv.3 | Verification & testing | 7 | 900 | Lv.2 |
| Lv.4 | Data pipeline & visualization | 6 | 700 | Lv.2 |
| Lv.5 | PDE prototypes | 7 | 1100 | Lv.3–4 |
| Lv.6 | C/Python coupling & automation | 7 | 1000 | Lv.3–5 |
| Boss | CFD validation projects | 4 | 2000 | 按项目要求 |

总 XP：`7500`。

## Unified acceptance gate

1. 环境可复现：记录 Python version、依赖和运行命令。
2. 函数有明确 input shape、dtype、units 和 failure behavior。
3. numerical mission 至少有 analytic/manufactured/reference solution。
4. 自动化脚本失败时返回 non-zero exit code，不静默跳过错误。
5. 图像只是结果展示，原始数据与误差指标必须可追溯。

## 推荐环境

- Python 3.12+；每个项目使用独立 `.venv`。
- 核心：NumPy、SciPy、Matplotlib、pytest。
- 后续按需：pandas、xarray、h5py、meshio、PyVista、Numba。
- 不建议初期直接上 JAX/PyTorch：先把 array shape、broadcasting、memory allocation 和 numerical verification 学扎实。

进度记录见 [PROGRESS.md](PROGRESS.md)，任务复盘使用 [MISSION_TEMPLATE.md](MISSION_TEMPLATE.md)。

