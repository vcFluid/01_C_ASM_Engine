# README - Project 2 描述：
# 问题模型：1-D Riemann Prb

## 目录

- [Riemann问题控制方程_1-D Euler PDEs](#riemann问题控制方程_1-d-euler-pdes)
- [具体初值条件](#具体初值条件)
- [目标](#目标)
  - [用MacCormack格式算1-D黎曼问题的数值解](#用maccormack格式算1-d黎曼问题的数值解)
  - [将数值解与半解析解（精确解）（project_0）对比](#将数值解与半解析解精确解对比)
  - [具体细节（采用MacCormack格式）](#具体细节采用maccormack格式)
- [我对自己(Code)的期望](#我对自己code的期望)
- [当前代码进度](#当前代码进度)
- [结果集目录命名约定](#结果集目录命名约定)

# MacCormack格式+人工粘性系数问题分析
这个问题可以拆成两个子问题，第一是解决MacCormack格式的预测步（以及求解时需要在两种预测方式中交替），第二是解决如何引入人工粘性

当前 code 内置的 7 组 Riemann 初值条件如下：

| Case | 问题类型 | $\mathbf{W}_L=(\rho_L,u_L,p_L)$ | $\mathbf{W}_R=(\rho_R,u_R,p_R)$ |
|---:|---|---:|---:|
| 1 | Sod shock tube | $(1,\ 0,\ 1)$ | $(0.125,\ 0,\ 0.1)$ |
| 2 | Lax shock tube | $(0.445,\ 0.698,\ 3.528)$ | $(0.5,\ 0,\ 0.571)$ |
| 3 | Subsonic double expansion | $(1,\ -2,\ 4)$ | $(1,\ 2,\ 4)$ |
| 4 | Sjogreen supersonic expansion | $(1,\ -2,\ 0.4)$ | $(1,\ 2,\ 0.4)$ |
| 5 | Contact discontinuity with double expansion | $(1,\ -0.2,\ 0.5)$ | $(0.5,\ 0.5,\ 0.5)$ |
| 6 | Contact discontinuity with double shock | $(0.4,\ 0.5,\ 1.0)$ | $(1,\ -0.5,\ 0.9)$ |
| 7 | Pure contact discontinuity | $(10,\ 1.0,\ 2.0)$ | $(1.0,\ 1.0,\ 2.0)$ |

# 目标：
## 用MacCormack格式算1-D黎曼问题的数值解
## 将数值解与半解析解（精确解）（project_0）对比
## 具体细节（采用MacCormack格式）
 - 1-Sod问题数值解 + 对比精确解 + 讨论人工粘性的有无对解的影响 + 讨论人工粘性经验参数β对解的影响 + CFL数对解的影响（尤其是注意到人工粘性系数并非CFL的单调函数）
    - 开关函数使用的是密度，可以进一步讨论压力、速度，实际比较不同类型选取的差异
    - 如果不加人工粘性，只加密网格，是否能改善色散效应（正常情况下色散效应不会随网格密度增加而衰减，是格式的固有属性）

 - 2-LAX问题，同上

 - 3-亚声速后退问题，同上

 - 4-Sjogreen supersonic Expansion超声速后退问题，同上（注意到真空区）（尤其是能不能算，能算的话如何取值）

 - 5-接触间断 - 双膨胀激波问题

 - 6-双激波间断问题

 - 7-纯接触间断问题（注意到反而是有些高级格式在这个问题上比不过MacCormark格式，可以拓展对比一下）=> 不同格式优秀的点还是有差异的


## Code的期望

1. 将 7 种问题内置相应的初值条件到 Code 中。暂时不考虑用户自己输入边界条件；但由于我的 Code 是 OOP，未来可以非常便捷地优化，只需要新建一个输入函数，后续 `main` 中利用指针修改初值变量。
2. 提供一个选择：是否添加人工粘性。如果添加，进一步选择人工粘性系数用什么变量构造，例如 `rho`、`p` 或 `u`。
3. 支持用户自定义输入网格，同时设置一个默认值以便于结果对比。
4. 将 Project 0 作为外置程序，Project 2 能调用它实时求解精确解，以便比较。
5. 自动化地将数据渲染为 Tecplot 图像。
6. 让图像动起来，动态展示从 `t=0` 到 `t=t_max` 的全过程。
7. 程序需要能够递归地输出不同人工粘性经验参数，实际工程中可能需要多尝试几次 `beta` 以便找到最恰当的解。

## 当前代码进度

项目当前已经完成主要代码链路，后续重点转向运行7组初值条件（分别对应7个黎曼初值问题）的数据、筛选图像并撰写结果分析。

已完成：

1. `1-D_Riemann_NM_MacC.c` 已内置 7 种 Riemann 初值条件。
2. MacCormack solver 以守恒量 `Q = [rho, rho*u, rho*E]` 为主推进变量，并同步更新原始量 `W = [rho, u, p]`。
3. 程序已支持用户设置计算域、间断位置、网格数、`gamma`、CFL、终止时间。
4. 程序已支持 artificial viscosity on/off、经验系数 `beta`、sensor=`rho/u/p`。
5. Project 0 已作为外部 exact solver 接入；Project 2 通过 OS process 调用它，并传递相同左右状态、计算域、网格、`x0`、`gamma` 和输出时刻。
6. 后处理脚本已能读取 numerical/exact Tecplot ASCII 数据，完成对齐、误差计算和 Tecplot macro 生成。
7. Tecplot `.mcr` batch mode 已作为正式出图方案；不依赖 TecPLUS/PyTecplot connection。
8. snapshot + exact snapshot + Tecplot transient dataset 已支持时间动画。
9. `postprocess/beta_sweep.py` 用于固定其它参数时扫描 `beta`。
10. `postprocess/targeted_matrix.py` 用于围绕 Sod 自动组织目标实验：viscosity on/off、`beta`、sensor、CFL、Nx。
11. `beta_slider.py` 用于读取已有 beta sweep 数据，并用 Python slider 快速观察 `beta` 对曲线的影响。


Code关键细节：

```bash
 /*
    1-D Euler equations:
        原始量 W = [rho, u, p] (一维流动，所以速度变量只有一维)
        守恒量 U = [rho, rho*u, rho*E] 
        矢通量 F = [rho*u, rho*u*u + p, u*(rho*E + p)]

    MacCormack 格式使用两个步骤:

        预测步 Predictor: U_bar_i = U_i - dt/dx * (F_{i+1} - F_i)
        矫正步 Corrector: U_i^{n+1} = 0.5 * (U_i + U_bar_i
                              - dt/dx * (F_bar_i - F_bar_{i-1}))
*/
```
```bash
/*
虽然都是求解守恒性方程，
\partial_t [rho, rho*u, rho*E]+\partial_x [rho*u, rho*u*u + p, rho*E*u + p*u] = 0

但是，这里有两种在内存条上开辟内存空间的方法，或者说推进过程中谁是主变量
    一种是每一步选取守恒量Q的三个变量[rho rho*u rho*E]作为主变量，开辟内存空间  --  (1)
        核心思想：也就是说内存里存什么，格式就直接更新什么，MacCormack、有限体积、Roe、HLLC 这类数值格式通常天然更新的是守恒量 Q
        步骤：内存分配 -> Q -> 进入循环 -> 迭代 -> 算Q,W,mu ->更新＆重新循环
        但人工粘性系数需要原始量W作为自变量，所以每一步还需要反解出W，
        Q -> W
        W -> F
        W -> 人工粘性系数
        方程的“非线性”被压缩在了求解[人工粘性系数]和[矢通量]的过程中，每一步

    另一种是每一步选取原始量W的三个变量[rho u p]作为主变量，开辟内存空间，然后组合出守恒量，代入方程中   --  (2)
        核心思想：内存里存最直观、最容易用来构造其他项的物理量，也就是把控制方程看作是由 W 组合而成的复杂方程组
        步骤：内存分配 -> W -> 进入循环 -> 组合出Q(W) F(W) 人工粘性 -> 更新＆重新循环

        在利用特征值求解该Riemann问题的解析解(project 0)的时候，实际上我们选用的是 (2) 方案，更符合理论推导的逻辑

在这里，应该选用 (1) 方案，因为观察守恒型方程的格式，主变量天然地就是 Q.

如果内存空间足够，我们可以将两种方案融合，也就是内存长期储存两套变量Q＆W，但规定Q是主状态，W是Q的同步"子状态"
也就是严格尊重MacCormack格式推进Q，W不单独推进，由Q反解同步
1. 当前主状态 Q^n 已存在
2. Q^n -> W^n
3. 用 W^n 计算声速、CFL、人工粘性、通量 F(Q/W)
4. MacCormack 更新 Q_bar
5. Q_bar -> W_bar
6. 用 W_bar 计算 F_bar
7. MacCormack 校正得到 Q^{n+1}
8. Q^{n+1} -> W^{n+1}
9. 输出/下一步循环

注意到这样和方案1的核心区别在于，我的 W 开辟内存空间后，是可查的
*/
```
```bash
/*
至于时间方向上的储存策略，一种是完整储存整个时间演化的完整发展，或者说每一秒我都给流场拍一张照片，然后所有照片都储存起来最后分析，另一种是，我拍一张照片，然后看一眼这个照片判断下一张照片长什么样，看完立刻把这张照片洗白然后作为胶片继续进行
/*
```
```bash
/*
方案 A：全历史存储
每个时间层都保存下来。
   优点：
   可以回看任意时刻
   可以做动画
   可以做完整时空分析
   可以检查波传播过程
   可以做误差随时间演化分析

   缺点：
   内存爆炸
   文件很大
   计算时缓存压力大
   对大型 CFD 不现实
      if
         nx = 100000
         nt = 100000
         变量数 = 3
         double = 8 bytes
      then 
         100000 * 100000 * 3 * 8 bytes = 240 GB

   适用：
   小算例
   快速验证
   后处理动画
   调试
   保存每一瞬态的快照
   


方案 B：滚动时间层存储
只保存当前计算所需的少数几张“照片”，算完就覆盖旧照片。
   优点：
      只保存当前推进需要的几层状态。
      算一步、覆盖一步。
      适合真正的 CFD 时间推进。

对于方案B，也可以实现方案A的功能？我只需要把每一次或每几次更新打印过程打印一个状态出来，未来想研究中间过程，只需要找到对应的状态，当作初始条件就行

   内存用方案 B 滚动推进
   磁盘用快照保存方案 A 的观察能力
   计算时不保存所有历史
   但每隔 N 步把当前流场写成一个 snapshot 文件

   如果未来想研究，就
   1. 找到某个 snapshot
   2. 读入其中的 Q 或 W
   3. 把它作为新的初始条件
   4. 从这个时间点继续跑
*/
```
```bash
/*  总结
程序逻辑伪代码：

   struct Riemann_1D_MacC_solver solver        // 结构体，定义对象和方法接口

   // 定义几个小工具
   static double pressure_from_q           // 用守恒量反算压强 利用公式 E = \frac{p}{rho(gamma - 1 )} + u*u/2
   static double total_energy_density      // 算单位体积流体的总能量（E*rho）
   static double *alloc_double_array(size_t n)     //连续开辟空间，具体功能联系solver_allocate看

   // 定义具体的方法（struct中定义的是抽象的方法接口）
   int solver_allocate(solver *self)            //给所有方法分配内存，记得绑定为allocate
   void solver_update_primitives(solver *self)  //更新原始量
   void solver_compute_flux                    //用每一步的Q求F
   double solver_compute_dt(solver *self)  //根据条件数和步长算dt

   void solver_apply_boundary
   void solver_apply_artificial_viscosity(solver *self)  //算人工粘性
   void solver_init_sod(solver *self)     //初值条件定义
   void solver_step_maccormack(solver *self)  // 预测步算法
   void solver_write_tecplot(solver *self, const char *filename)  // 输出结果为tecplot格式

   void solver_destroy(solver *self)   // 释放堆上的内存
   void solver_bind_methods(solver *self)    // 将具体的方法与struct中的抽象方法接口绑定

   // 接下来处理一些杂事
   static solver solver_create_default(void)    // 创建一个具有默认配置的 solver 结构体并返回，将对象+方法实例化
   static void ask_output_filename(char *filename, size_t size)   // 用户输入决定最终输出文件名
   static void make_snapshot_filename(    // 保存快照，
    const char *final_filename,
    int step,
    char *snapshot_filename,
    size_t size
   ) {}
   static void print_banner(void)   // 打印程序标题
   static void configure_riemann_case(solver *self, int *case_id)    // 用户选择Riemann初始条件，程序同步修改solver 中的左右状态
   static void configure_domain_and_grid(solver *self)      // 配置空间计算域和网格（自适应地计算需要
   static void configure_numerics(solver *self)          // 配置全局的参数，包括比热比、CFL、t_max、人工粘性开关、人工粘性系数，快照输出间隔
   static void print_run_summary(solver *self, int case_id, const char *filename)   // 计算开始前，打印最终参数并保存为log_YYYY_MM_DD_hh

   // OK，写main函数
   int main(void){}
*/
```
## 结果集目录命名约定

正式结果集统一放在 `runs/Solution_XX_CaseName/` 下，例如：

- `runs/Solution_01_Sod/`
- `runs/Solution_02_LAX/`
- `runs/Solution_03_Subsonic_Double_Expansion/`

后续新增算例也沿用该格式，以便未来查找.
