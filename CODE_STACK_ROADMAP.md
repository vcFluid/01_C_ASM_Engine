# Gravity 的 Fluid + Code 技术栈路线

## 核心判断

不要同时“学很多语言”。应建立一条 CFD production 主线和一条 game/creative 副线：

- CFD 主线：`C → Python → Linux/Bash → C++ → OpenMP/MPI → GPU（按需求）`
- 数值软件基础设施：`Git + CMake + testing + profiling + documentation`
- 游戏副线：优先 `C# + Godot`；若目标是 engine-level 或深度复刻，再进入 `C++`。

## 为什么下一门主语言应是 C++

C++ 是 C 之后对 CFD 能力增益最大的语言：

- 大量 production CFD/HPC codebase 使用 C++，便于接触 OpenFOAM、deal.II、MFEM、AMReX 等生态。
- RAII、containers、templates 和 abstractions 能让 mesh、field、operator、solver 的 ownership 更清楚。
- 可保持接近 C 的性能，同时改善大型 solver 的工程组织。

但不要过早学习复杂 template metaprogramming。第一阶段只学：

1. reference、const correctness、RAII；
2. `std::vector`、`std::array`、`span` 概念；
3. class/struct、composition、move semantics 基础；
4. CMake、unit testing、sanitizers；
5. 用 C++ 重构一个已经由 C/Python 验证正确的小型 Poisson solver。

## Fortran 是否值得学

值得“能读、能改、能互操作”，暂不必作为主力语言。

- 优势：数组语义和科学计算生态成熟，仍有大量 legacy CFD/HPC code。
- 适用：阅读经典 solver、维护研究组代码、调用 BLAS/LAPACK 或现有 Fortran kernel。
- 建议目标：现代 Fortran 2008 基础、module、allocatable array、derived type、ISO_C_BINDING。

## 并行与 GPU

它们不是独立语言收藏，而是 solver 成熟后的能力层：

| 阶段 | 技术 | 进入条件 |
|---|---|---|
| 单节点 CPU | OpenMP | serial solver 已验证且 profiler 指向可并行热点 |
| 多节点 | MPI | 已理解 domain decomposition、halo/ghost exchange |
| GPU | CUDA 或 performance-portability framework | 问题规模与硬件确实需要，且 memory traffic 已分析 |

GPU 路线建议：

- NVIDIA 研究环境明确：CUDA。
- 希望跨平台：先观察 Kokkos、SYCL、OpenMP target 的项目需求，再选。
- 不建议把 GPU 作为当前第一优先级；离散一致性、verification 和 profiling 的收益更高。

## 桌宠与 Starbound-like 游戏路线

### 最短成品路线

`C# + Godot`

- C# 适合桌面工具、游戏逻辑和较大型项目结构。
- Godot 适合 2D、tilemap、UI、animation、mod-friendly 原型。
- 桌宠还需要学习：window transparency、always-on-top、input pass-through、sprite animation、state machine。

### 更底层路线

`C++ + SDL2/SDL3`

适合想理解 game loop、rendering、input、audio、asset pipeline 和 ECS 的情况，但完成作品更慢。

### 不建议的起步方式

- 一开始自研完整 engine。
- 因为 Starbound 使用底层技术，就直接复刻其全部架构。
- 同时学习 Unity、Unreal、Godot 和 SDL。

先做三个递进项目：

1. 透明窗口桌宠：idle/walk/click 三状态；
2. 2D tilemap sandbox：角色移动、碰撞、地图保存；
3. Starbound-like vertical slice：挖掘、放置、inventory、一个 biome，不做完整宇宙生成。

## 建议优先级

### Phase A：现在

- 完成 C Questline Lv.0–3。
- Python 完成 Lv.0–4，用于验证 C 数值结果。
- Linux/OS 完成 Lv.0–3，建立可靠 build/debug workflow。
- 同时使用 Git、CMake、pytest/CTest 和 sanitizer。

### Phase B：首个 incompressible solver 后

- 学 Modern C++ 基础并重构 Poisson/projection solver。
- 学 sparse linear algebra、BLAS/LAPACK、PETSc 基础。
- 做 lid-driven cavity、Poiseuille flow、Taylor–Green vortex verification。

### Phase C：性能扩展

- 先 profiling，再 OpenMP。
- 再做 MPI domain decomposition。
- 最后根据硬件与研究问题决定 CUDA/Kokkos/SYCL。

### Phase D：游戏副线

- 使用 C# + Godot 快速完成桌宠。
- 若对 engine internals 兴趣持续，再用 C++ + SDL 做一个小型 2D engine。

## 最终能力画像

目标不是“会几门语言”，而是能够完成以下闭环：

```text
PDE/physical model
  → mathematically consistent discretization
  → verified serial kernel
  → automated tests and post-processing
  → profiled parallel implementation
  → reproducible experiment and technical report
```

当你能独立完成这个闭环时，才真正具备 Fluid + Code 复合竞争力。
