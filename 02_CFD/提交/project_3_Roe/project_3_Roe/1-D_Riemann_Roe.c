#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define INPUT_LINE_LEN 128      // 交互输入缓冲区长度
#define OUTPUT_NAME_LEN 256     // 输出文件名缓冲区长度
#define COMMAND_LINE_LEN 2048   // 调用 exact solver 时的命令行缓冲区长度
#define PHYSICAL_FLOOR 1.0e-12  // 用于检查 rho 和 p 是否已经变成非物理值
#define NGHOST 1                // 一维一阶 Godunov/FVM 每侧只需要一层 ghost cell

/*
    1-D Euler equations:
        原始量 W = [rho, u, p]
        守恒量 Q = [rho, rho*u, rho*E]
        物理通量 F(Q) = [rho*u, rho*u*u + p, u*(rho*E + p)]

    Project 3 的数值核心不是 MacCormack predictor-corrector，而是
    Godunov-type finite volume update:

        Q_i^{n+1}
        =
        Q_i^n - dt/dx * (Fhat_{i+1/2} - Fhat_{i-1/2})

    Roe 方法在这里的角色:
        输入左右控制体状态 (Q_i, Q_{i+1})
        通过 Roe average 和特征分解构造界面数值通量 Fhat_{i+1/2}

    注意:
        current        对应 Q^n
        next           对应 Q^{n+1}
        interface_flux 对应 Fhat_{i+1/2}
        primitive      是由 current 反算得到的 W，不单独推进
*/

// 单个控制体的守恒量 Q = [rho, rho*u, rho*E]
typedef struct {
    double rho;   // q1 = rho，密度
    double rhou;  // q2 = rho*u，动量密度
    double rhoE;  // q3 = rho*E，总能量密度
} Conserved;

// 单个控制体的原始量和热力学辅助量
typedef struct {
    double rho;  // 密度
    double u;    // 一维速度
    double p;    // 压强
    double E;    // 单位质量总能量 E = rhoE / rho
    double H;    // 总焓 H = (rhoE + p) / rho，Roe 平均需要它
    double a;    // 声速 a = sqrt(gamma*p/rho)，CFL 和 Roe 特征值需要它
} Primitive;

// 单个界面或单个控制体上的 Euler 通量
typedef struct {
    double mass;      // 质量通量
    double momentum;  // 动量通量
    double energy;    // 能量通量
} Flux;

// 一整层守恒量数组，current/next 都使用这个结构
typedef struct {
    double *rho;   // 所有 cell 的 rho
    double *rhou;  // 所有 cell 的 rho*u
    double *rhoE;  // 所有 cell 的 rho*E
} ConservedArray;

// Roe 格式保存的是控制体界面的数值通量，不是 Project 2 那种格点上的物理通量 F(Q_i)
typedef struct {
    double *mass;      // Fhat 的质量分量
    double *momentum;  // Fhat 的动量分量
    double *energy;    // Fhat 的能量分量
} FluxArray;

// 输出、CFL 和诊断使用的原始量缓存
typedef struct {
    double *rho;  // 密度
    double *u;    // 速度
    double *p;    // 压强
    double *E;    // 单位质量总能量
    double *a;    // 声速
} PrimitiveArray;

// Riemann 问题某一侧的初始原始状态
typedef struct {
    double rho;  // 初始密度
    double u;    // 初始速度
    double p;    // 初始压强
} PrimitiveState;

// 一维 cell-centered 有限体积网格信息
typedef struct {
    int nx;       // 物理控制体数量，不包含 ghost cell
    int nghost;   // 每侧 ghost cell 数量
    int ntotal;   // 数组总长度 = nx + 2*nghost
    int first;    // 第一个物理 cell 在数组中的下标
    int last;     // 最后一个物理 cell 在数组中的下标
    double xmin;  // 计算区域左端点
    double xmax;  // 计算区域右端点
    double x0;    // 初始间断位置
    double dx;    // 控制体宽度 dx = (xmax-xmin)/nx
} Grid1D;

// Riemann 问题左右两侧初值
typedef struct {
    PrimitiveState left;   // 间断左侧初值
    PrimitiveState right;  // 间断右侧初值
} RiemannInitialCondition;

// 理想气体参数
typedef struct {
    double gamma; // 比热比，本作业默认取 1.4
} GasModel;

// 时间推进参数
typedef struct {
    double cfl;      // 目标 CFL，显式格式需要小于 1
    double t;        // 当前物理时间
    double t_max;    // 计算终止时间
    double dt;       // 当前时间步长，每一步由 CFL 重新计算
    int step_count;  // 已经完成的时间步数
    int step_limit;  // 最大步数，防止异常情况下无限循环
    int output_interval; // 快照输出间隔，0 表示只输出最终结果
} TimeControl;

// 总求解器，把网格、物理参数、时间状态、Roe 通量和缓存变量放在一起
typedef struct {
    Grid1D grid;                     // cell-centered FVM 网格
    GasModel gas;                    // 理想气体参数
    RiemannInitialCondition initial; // Riemann 初值
    TimeControl time;                // 时间推进控制

    ConservedArray current;  // 当前时间层 Q^n
    ConservedArray next;     // 下一时间层 Q^{n+1}
    FluxArray interface_flux; // 界面通量 Fhat_{i+1/2}
    PrimitiveArray primitive; // 由 current 反算得到的原始量缓存
} Solver;

static void grid_finalize(Grid1D *grid)
{
    // 根据用户输入的物理 cell 数 nx，统一生成 ghost cell 下标和 dx。
    // Project 2 的 nx 更接近节点数；这里 nx 明确表示控制体数量。
    grid->nghost = NGHOST;
    grid->first = grid->nghost;
    grid->last = grid->first + grid->nx - 1;
    grid->ntotal = grid->nx + 2 * grid->nghost;
    grid->dx = (grid->xmax - grid->xmin) / (double)grid->nx;
}

static double cell_left_edge_x(const Solver *solver, int i)
{
    // 数组下标 i 先减去 first，得到第几个物理控制体，再反推左界面坐标。
    return solver->grid.xmin +
           (double)(i - solver->grid.first) * solver->grid.dx;
}

static double cell_center_x(const Solver *solver, int i)
{
    // Tecplot 输出用 cell center，避免把有限体积单元平均量误读成节点值。
    return cell_left_edge_x(solver, i) + 0.5 * solver->grid.dx;
}

static double *alloc_double_array(size_t n)
{
    // 给长度为 n 的 double 数组开空间，并全部初始化为 0。
    return (double *)calloc(n, sizeof(double));
}

static int conserved_alloc(ConservedArray *q, size_t n)
{
    // 给一整组守恒量 Q=[rho,rhou,rhoE] 开空间。
    q->rho = alloc_double_array(n);
    q->rhou = alloc_double_array(n);
    q->rhoE = alloc_double_array(n);
    return q->rho != NULL && q->rhou != NULL && q->rhoE != NULL;
}

static void conserved_free(ConservedArray *q)
{
    // 释放一整组守恒量，避免三个数组漏掉某一个。
    free(q->rho);
    free(q->rhou);
    free(q->rhoE);
    q->rho = NULL;
    q->rhou = NULL;
    q->rhoE = NULL;
}

static void conserved_copy(const Solver *solver, ConservedArray *dst, const ConservedArray *src)
{
    // Q 层之间整组复制；长度必须包含 ghost cell。
    size_t bytes = sizeof(double) * (size_t)solver->grid.ntotal;
    memcpy(dst->rho, src->rho, bytes);
    memcpy(dst->rhou, src->rhou, bytes);
    memcpy(dst->rhoE, src->rhoE, bytes);
}

static int flux_alloc(FluxArray *flux, size_t n)
{
    // 给界面数值通量 Fhat=[mass,momentum,energy] 开空间。
    flux->mass = alloc_double_array(n);
    flux->momentum = alloc_double_array(n);
    flux->energy = alloc_double_array(n);
    return flux->mass != NULL && flux->momentum != NULL && flux->energy != NULL;
}

static void flux_free(FluxArray *flux)
{
    // 释放 Roe 界面通量数组。
    free(flux->mass);
    free(flux->momentum);
    free(flux->energy);
    flux->mass = NULL;
    flux->momentum = NULL;
    flux->energy = NULL;
}

static int primitive_alloc(PrimitiveArray *w, size_t n)
{
    // 给原始量 W=[rho,u,p,E,a] 开空间。
    w->rho = alloc_double_array(n);
    w->u = alloc_double_array(n);
    w->p = alloc_double_array(n);
    w->E = alloc_double_array(n);
    w->a = alloc_double_array(n);
    return w->rho != NULL && w->u != NULL && w->p != NULL &&
           w->E != NULL && w->a != NULL;
}

static void primitive_free(PrimitiveArray *w)
{
    // 释放原始量数组。
    free(w->rho);
    free(w->u);
    free(w->p);
    free(w->E);
    free(w->a);
    w->rho = NULL;
    w->u = NULL;
    w->p = NULL;
    w->E = NULL;
    w->a = NULL;
}

static int solver_allocate(Solver *solver)
{
    // 给整个 solver 需要的数组一次性开好空间。
    // 注意 n 是 ntotal，不是 nx；ghost cell 也要有存储空间。
    size_t n = (size_t)solver->grid.ntotal;
    int ok = conserved_alloc(&solver->current, n) &&
             conserved_alloc(&solver->next, n) &&
             flux_alloc(&solver->interface_flux, n) &&
             primitive_alloc(&solver->primitive, n);

    if (!ok) {
        return 0;
    }
    return 1;
}

static void solver_destroy(Solver *solver)
{
    // 程序结束前释放所有堆上的内存。
    conserved_free(&solver->current);
    conserved_free(&solver->next);
    flux_free(&solver->interface_flux);
    primitive_free(&solver->primitive);
}

static double pressure_from_conserved(const Solver *solver, double rho, double rhou, double rhoE)
{
    // 由守恒量反算压强: p=(gamma-1)*(rhoE-0.5*rho*u^2)。
    double kinetic = 0.5 * rhou * rhou / rho;
    return (solver->gas.gamma - 1.0) * (rhoE - kinetic);
}

static double total_energy_density(const Solver *solver, double rho, double u, double p)
{
    // 由原始量构造总能量密度 rhoE，初始化 Riemann 问题时使用。
    return p / (solver->gas.gamma - 1.0) + 0.5 * rho * u * u;
}

static Conserved conserved_from_primitive_state(const Solver *solver, PrimitiveState state)
{
    // 把某一侧的初始原始量 [rho,u,p] 转成守恒量 [rho,rhou,rhoE]。
    Conserved q;
    q.rho = state.rho;
    q.rhou = state.rho * state.u;
    q.rhoE = total_energy_density(solver, state.rho, state.u, state.p);
    return q;
}

static Conserved conserved_at(const ConservedArray *q, int i)
{
    // 从数组形式中取出第 i 个 cell 的守恒状态，方便 Roe flux 使用。
    Conserved value;
    value.rho = q->rho[i];
    value.rhou = q->rhou[i];
    value.rhoE = q->rhoE[i];
    return value;
}

static Primitive primitive_from_conserved(const Solver *solver, Conserved q)
{
    // 单个守恒状态转原始状态；Roe 平均还需要总焓 H。
    Primitive w;
    w.rho = q.rho;
    w.u = q.rhou / q.rho;
    w.p = pressure_from_conserved(solver, q.rho, q.rhou, q.rhoE);
    w.E = q.rhoE / q.rho;
    w.H = (q.rhoE + w.p) / q.rho;
    if (w.rho > 0.0 && w.p > 0.0) {
        w.a = sqrt(solver->gas.gamma * w.p / w.rho);
    } else {
        w.a = 0.0;
    }
    return w;
}

static Flux physical_flux_from_conserved(const Solver *solver, Conserved q)
{
    // Euler 物理通量 F(Q)，Roe 通量公式中的 FL 和 FR 需要它。
    Primitive w = primitive_from_conserved(solver, q);
    Flux f;
    f.mass = q.rhou;
    f.momentum = q.rhou * w.u + w.p;
    f.energy = (q.rhoE + w.p) * w.u;
    return f;
}

static void compute_primitive_from_current(Solver *solver)
{
    // 由当前守恒量 Q^n 反算原始量 W；只处理物理 cell，不处理 ghost cell。
    for (int i = solver->grid.first; i <= solver->grid.last; i++) {
        Conserved q = conserved_at(&solver->current, i);
        Primitive w = primitive_from_conserved(solver, q);
        solver->primitive.rho[i] = w.rho;
        solver->primitive.u[i] = w.u;
        solver->primitive.p[i] = w.p;
        solver->primitive.E[i] = w.E;
        solver->primitive.a[i] = w.a;
    }
}

static double compute_max_wave_speed(Solver *solver)
{
    // 计算 max(|u|+a)，也就是一维 Euler 的最大谱半径。
    double max_speed = 0.0;
    compute_primitive_from_current(solver);

    for (int i = solver->grid.first; i <= solver->grid.last; i++) {
        double speed = fabs(solver->primitive.u[i]) + solver->primitive.a[i];
        if (speed > max_speed) {
            max_speed = speed;
        }
    }
    return max_speed;
}

static double compute_dt_from_cfl(Solver *solver)
{
    // 根据 CFL = max(|u|+a)*dt/dx 反算 dt。
    double max_speed = compute_max_wave_speed(solver);
    if (max_speed <= 0.0) {
        return 1.0e-8;
    }
    return solver->time.cfl * solver->grid.dx / max_speed;
}

static void apply_zero_gradient_boundary(const Solver *solver, ConservedArray *q)
{
    // 零梯度边界: ghost cell 直接复制相邻物理 cell。
    // 这里和 Project 2 的边界条件物理含义一致，但实现上更符合 FVM。
    for (int g = 1; g <= solver->grid.nghost; g++) {
        int left_ghost = solver->grid.first - g;
        int right_ghost = solver->grid.last + g;
        int left_src = solver->grid.first;
        int right_src = solver->grid.last;

        q->rho[left_ghost] = q->rho[left_src];
        q->rhou[left_ghost] = q->rhou[left_src];
        q->rhoE[left_ghost] = q->rhoE[left_src];

        q->rho[right_ghost] = q->rho[right_src];
        q->rhou[right_ghost] = q->rhou[right_src];
        q->rhoE[right_ghost] = q->rhoE[right_src];
    }
}

static int conserved_state_is_physical(const Solver *solver, const ConservedArray *q)
{
    // 检查物理 cell 中是否出现负密度、负压力或 NaN/Inf。
    // Roe baseline 不做强制修补，因为 failure 本身是结果讨论的一部分。
    for (int i = solver->grid.first; i <= solver->grid.last; i++) {
        double rho = q->rho[i];
        double rhou = q->rhou[i];
        double rhoE = q->rhoE[i];
        double p = 0.0;

        if (!isfinite(rho) || !isfinite(rhou) || !isfinite(rhoE)) {
            fprintf(stderr, "[Error] Nonfinite conserved state at i=%d.\n", i);
            return 0;
        }
        if (rho <= PHYSICAL_FLOOR) {
            fprintf(stderr, "[Error] Nonpositive density at i=%d, rho=%.17g.\n", i, rho);
            return 0;
        }
        p = pressure_from_conserved(solver, rho, rhou, rhoE);
        if (!isfinite(p) || p <= PHYSICAL_FLOOR) {
            fprintf(stderr, "[Error] Nonpositive pressure at i=%d, p=%.17g.\n", i, p);
            return 0;
        }
    }
    return 1;
}

static int compute_roe_flux(const Solver *solver, Conserved qL, Conserved qR, Flux *fhat)
{
    /*
        计算单个界面的 Roe 数值通量 Fhat_{i+1/2}。

        输入:
            qL = 左侧 cell 的守恒状态
            qR = 右侧 cell 的守恒状态

        主要步骤严格按课件展开式:
            1. Q -> W，得到左右原始量和总焓；
            2. 计算左右物理通量 FL, FR；
            3. 用 sqrt(rho) 加权得到 Roe average；
            4. 计算课件中的 alpha1, alpha2, alpha3；
            5. alpha4 = alpha1 + alpha2 + alpha3；
               alpha5 = c_tilde * (alpha2 - alpha3)；
            6. 显式构造 |A_tilde|*DeltaQ 的三个分量；
            7. Fhat = 0.5*(FL+FR) - 0.5*(|A_tilde|*DeltaQ)。
    */
    Primitive WL = primitive_from_conserved(solver, qL);
    Primitive WR = primitive_from_conserved(solver, qR);
    Flux FL = physical_flux_from_conserved(solver, qL);
    Flux FR = physical_flux_from_conserved(solver, qR);
    double sqrt_rho_L = 0.0;
    double sqrt_rho_R = 0.0;
    double denom = 0.0;
    double u_tilde = 0.0;
    double H_tilde = 0.0;
    double a2_tilde = 0.0;
    double a_tilde = 0.0;
    double rho_tilde = 0.0;
    double drho = 0.0;
    double du = 0.0;
    double dp = 0.0;
    double abs_u = 0.0;
    double abs_u_plus_a = 0.0;
    double abs_u_minus_a = 0.0;
    double alpha1 = 0.0;
    double alpha2 = 0.0;
    double alpha3 = 0.0;
    double alpha4 = 0.0;
    double alpha5 = 0.0;
    double diss_mass = 0.0;
    double diss_momentum = 0.0;
    double diss_energy = 0.0;

    if (WL.rho <= PHYSICAL_FLOOR || WR.rho <= PHYSICAL_FLOOR ||
        WL.p <= PHYSICAL_FLOOR || WR.p <= PHYSICAL_FLOOR) {
        // Roe 平均需要左右状态本身物理；否则直接报告失败。
        return 0;
    }

    // Roe average: 密度平方根加权平均速度和总焓。
    sqrt_rho_L = sqrt(WL.rho);
    sqrt_rho_R = sqrt(WR.rho);
    denom = sqrt_rho_L + sqrt_rho_R;
    if (denom <= 0.0) {
        return 0;
    }

    u_tilde = (sqrt_rho_L * WL.u + sqrt_rho_R * WR.u) / denom;
    H_tilde = (sqrt_rho_L * WL.H + sqrt_rho_R * WR.H) / denom;
    a2_tilde = (solver->gas.gamma - 1.0) *
               (H_tilde - 0.5 * u_tilde * u_tilde);
    if (a2_tilde <= PHYSICAL_FLOOR || !isfinite(a2_tilde)) {
        return 0;
    }
    a_tilde = sqrt(a2_tilde);
    rho_tilde = sqrt_rho_L * sqrt_rho_R;

    // 课件中的 Delta(.) = (.)_R - (.)_L。
    drho = WR.rho - WL.rho;
    du = WR.u - WL.u;
    dp = WR.p - WL.p;

    // 课件公式中的三个特征值绝对值: |u|, |u+c|, |u-c|。
    abs_u = fabs(u_tilde);
    abs_u_plus_a = fabs(u_tilde + a_tilde);
    abs_u_minus_a = fabs(u_tilde - a_tilde);

    /*
        严格按课件 alpha1...alpha5:

        alpha1 = |u| * (Delta rho - Delta p / c^2)
        alpha2 = |u+c|/(2c^2) * (Delta p + rho*c*Delta u)
        alpha3 = |u-c|/(2c^2) * (Delta p - rho*c*Delta u)
        alpha4 = alpha1 + alpha2 + alpha3
        alpha5 = c * (alpha2 - alpha3)
    */
    alpha1 = abs_u * (drho - dp / a2_tilde);
    alpha2 = 0.5 * abs_u_plus_a *
             (dp + rho_tilde * a_tilde * du) / a2_tilde;
    alpha3 = 0.5 * abs_u_minus_a *
             (dp - rho_tilde * a_tilde * du) / a2_tilde;
    alpha4 = alpha1 + alpha2 + alpha3;
    alpha5 = a_tilde * (alpha2 - alpha3);

    /*
        课件中的 |A_tilde|*DeltaQ 展开式:

        [ alpha4,
          u_tilde*alpha4 + alpha5,
          H_tilde*alpha4 + u_tilde*alpha5
              - c_tilde^2*alpha1/(gamma-1) ].
    */
    diss_mass = alpha4;
    diss_momentum = u_tilde * alpha4 + alpha5;
    diss_energy = H_tilde * alpha4 + u_tilde * alpha5 -
                  a2_tilde * alpha1 / (solver->gas.gamma - 1.0);

    // Fhat = 0.5*(FL+FR) - 0.5*(|A_tilde|*DeltaQ)。
    fhat->mass =
        0.5 * (FL.mass + FR.mass) - 0.5 * diss_mass;

    fhat->momentum =
        0.5 * (FL.momentum + FR.momentum) - 0.5 * diss_momentum;

    fhat->energy =
        0.5 * (FL.energy + FR.energy) - 0.5 * diss_energy;

    return isfinite(fhat->mass) && isfinite(fhat->momentum) && isfinite(fhat->energy);
}

static int compute_all_roe_fluxes(Solver *solver)
{
    // 逐界面计算 Fhat。interface_flux[i] 表示 i 与 i+1 之间的界面通量。
    // 循环从 left ghost/first 之间的界面开始，到 last/right ghost 之间的界面结束。
    for (int i = solver->grid.first - 1; i <= solver->grid.last; i++) {
        Conserved qL = conserved_at(&solver->current, i);
        Conserved qR = conserved_at(&solver->current, i + 1);
        Flux fhat;

        if (!compute_roe_flux(solver, qL, qR, &fhat)) {
            fprintf(stderr, "[Error] Roe flux failed at interface i+1/2=%d.\n", i);
            return 0;
        }

        solver->interface_flux.mass[i] = fhat.mass;
        solver->interface_flux.momentum[i] = fhat.momentum;
        solver->interface_flux.energy[i] = fhat.energy;
    }
    return 1;
}

static int advance_one_roe_step(Solver *solver)
{
    // 完成一个 Roe-FVM 时间步: CFL -> boundary -> Roe flux -> conservative update。
    double lambda = 0.0;

    // 先由 CFL 条件算本步 dt。
    solver->time.dt = compute_dt_from_cfl(solver);
    if (solver->time.t + solver->time.dt > solver->time.t_max) {
        // 最后一步不要超过用户给定的 t_max。
        solver->time.dt = solver->time.t_max - solver->time.t;
    }
    if (solver->time.dt <= 0.0) {
        return 1;
    }

    // 当前层先补 ghost cell，再检查物理性，再构造界面 Riemann 问题。
    apply_zero_gradient_boundary(solver, &solver->current);
    if (!conserved_state_is_physical(solver, &solver->current)) {
        return 0;
    }
    if (!compute_all_roe_fluxes(solver)) {
        return 0;
    }

    conserved_copy(solver, &solver->next, &solver->current);
    lambda = solver->time.dt / solver->grid.dx;

    /*
        有限体积守恒更新:
            next_i = current_i - dt/dx * (Fhat_{i+1/2} - Fhat_{i-1/2})

        在数组记号中:
            interface_flux[i]     是右界面 Fhat_{i+1/2}
            interface_flux[i - 1] 是左界面 Fhat_{i-1/2}
    */
    for (int i = solver->grid.first; i <= solver->grid.last; i++) {
        solver->next.rho[i] =
            solver->current.rho[i] -
            lambda * (solver->interface_flux.mass[i] -
                      solver->interface_flux.mass[i - 1]);
        solver->next.rhou[i] =
            solver->current.rhou[i] -
            lambda * (solver->interface_flux.momentum[i] -
                      solver->interface_flux.momentum[i - 1]);
        solver->next.rhoE[i] =
            solver->current.rhoE[i] -
            lambda * (solver->interface_flux.energy[i] -
                      solver->interface_flux.energy[i - 1]);
    }

    apply_zero_gradient_boundary(solver, &solver->next);
    if (!conserved_state_is_physical(solver, &solver->next)) {
        fprintf(stderr, "[Error] Nonphysical state after Roe update.\n");
        return 0;
    }

    // 新时间层成为下一步的当前层。
    conserved_copy(solver, &solver->current, &solver->next);
    solver->time.t += solver->time.dt;
    solver->time.step_count++;
    return 1;
}

static void initialize_riemann_problem(Solver *solver)
{
    // 按左右初值给 current=Q^0 赋值。
    // 如果 x0 落在某个控制体内部，用守恒量的体积分数平均初始化该 cell。
    Conserved q_left = conserved_from_primitive_state(solver, solver->initial.left);
    Conserved q_right = conserved_from_primitive_state(solver, solver->initial.right);

    for (int i = solver->grid.first; i <= solver->grid.last; i++) {
        double xl = cell_left_edge_x(solver, i);
        double xr = xl + solver->grid.dx;
        Conserved q = q_right;

        if (xr <= solver->grid.x0) {
            // 整个 cell 都在间断左侧。
            q = q_left;
        } else if (xl < solver->grid.x0) {
            // 间断穿过当前 cell；用 conservative cell average，而不是平均原始量。
            double left_fraction = (solver->grid.x0 - xl) / solver->grid.dx;
            q.rho = left_fraction * q_left.rho +
                    (1.0 - left_fraction) * q_right.rho;
            q.rhou = left_fraction * q_left.rhou +
                     (1.0 - left_fraction) * q_right.rhou;
            q.rhoE = left_fraction * q_left.rhoE +
                     (1.0 - left_fraction) * q_right.rhoE;
        }

        solver->current.rho[i] = q.rho;
        solver->current.rhou[i] = q.rhou;
        solver->current.rhoE[i] = q.rhoE;
    }

    apply_zero_gradient_boundary(solver, &solver->current);
    compute_primitive_from_current(solver);
}

static void write_tecplot(Solver *solver, const char *filename)
{
    // 输出 Tecplot 可读的 dat 文件，只输出物理 cell，不输出 ghost cell。
    FILE *fp = fopen(filename, "w");

    if (fp == NULL) {
        fprintf(stderr, "[Error] Cannot open output file: %s\n", filename);
        return;
    }

    compute_primitive_from_current(solver);

    fprintf(fp, "TITLE = \"1D Euler Roe Result\"\n");
    fprintf(fp,
            "VARIABLES = \"x\", \"rho\", \"u\", \"p\", \"E\", "
            "\"rho_conserved\", \"rhou\", \"rhoE\"\n");
    fprintf(fp, "ZONE T=\"t=%.8f\", I=%d, F=POINT\n",
            solver->time.t,
            solver->grid.nx);

    for (int i = solver->grid.first; i <= solver->grid.last; i++) {
        // x 是 cell center 坐标；数值值代表该控制体内的平均/代表状态。
        double x = cell_center_x(solver, i);
        fprintf(fp,
                "%.10f %.10f %.10f %.10f %.10f %.10f %.10f %.10f\n",
                x,
                solver->primitive.rho[i],
                solver->primitive.u[i],
                solver->primitive.p[i],
                solver->primitive.E[i],
                solver->current.rho[i],
                solver->current.rhou[i],
                solver->current.rhoE[i]);
    }

    fclose(fp);
}

static Solver solver_create_default(void)
{
    // 给 solver 一个默认设置，用户直接回车时就用这些值。
    Solver solver;
    memset(&solver, 0, sizeof(solver));

    // 默认用 Sod shock tube 的常用区域 [0,1] 和间断位置 x=0.5。
    solver.grid.nx = 501;
    solver.grid.xmin = 0.0;
    solver.grid.xmax = 1.0;
    solver.grid.x0 = 0.5;
    grid_finalize(&solver.grid);

    solver.gas.gamma = 1.4;

    // 默认 CFL=0.5，比极限 CFL<1 更保守。
    solver.time.cfl = 0.5;
    solver.time.t = 0.0;
    solver.time.t_max = 0.2;
    solver.time.dt = 0.0;
    solver.time.step_count = 0;
    solver.time.step_limit = 200000;
    solver.time.output_interval = 0;

    // 默认初值就是 Sod 问题。
    solver.initial.left = (PrimitiveState){1.0, 0.0, 1.0};
    solver.initial.right = (PrimitiveState){0.125, 0.0, 0.1};

    return solver;
}

static int read_line(char *buffer, size_t size)
{
    // 读取一整行用户输入，并去掉 Windows/Linux 换行符。
    if (fgets(buffer, (int)size, stdin) == NULL) {
        return 0;
    }
    buffer[strcspn(buffer, "\r\n")] = '\0';
    return 1;
}

static int ask_int_range(const char *prompt, int default_value, int min_value, int max_value)
{
    // 读取整数参数；直接回车表示采用默认值，并限制上下界。
    char line[INPUT_LINE_LEN];
    int value = default_value;

    while (1) {
        printf("%s [%d]: ", prompt, default_value);
        if (!read_line(line, sizeof(line)) || line[0] == '\0') {
            return default_value;
        }

        if (sscanf(line, "%d", &value) == 1 &&
            value >= min_value && value <= max_value) {
            return value;
        }

        printf("Invalid input.\n");
    }
}

static double ask_double_range(
    const char *prompt,
    double default_value,
    double min_value,
    double max_value
) {
    // 读取浮点参数；直接回车表示采用默认值，并限制上下界。
    char line[INPUT_LINE_LEN];
    double value = default_value;

    while (1) {
        printf("%s [%.6g]: ", prompt, default_value);
        if (!read_line(line, sizeof(line)) || line[0] == '\0') {
            return default_value;
        }

        if (sscanf(line, "%lf", &value) == 1 &&
            value >= min_value && value <= max_value) {
            return value;
        }

        printf("Invalid input.\n");
    }
}

static double ask_double_min(const char *prompt, double default_value, double min_value)
{
    // 读取只限制最小值的浮点参数，比如 xmin 可以给很小的负数。
    char line[INPUT_LINE_LEN];
    double value = default_value;

    while (1) {
        printf("%s [%.6g]: ", prompt, default_value);
        if (!read_line(line, sizeof(line)) || line[0] == '\0') {
            return default_value;
        }

        if (sscanf(line, "%lf", &value) == 1 && value >= min_value) {
            return value;
        }

        printf("Invalid input.\n");
    }
}

static void configure_riemann_case(Solver *solver, int *case_id)
{
    // 与 Project 2 保持同一组七个 Riemann test cases，方便横向对比。
    printf("case: 1-Sod 2-Lax 3-SubExp 4-Sjogreen 5-ContactExp 6-ContactShock 7-Contact\n");
    *case_id = ask_int_range("case", 1, 1, 7);

    if (*case_id == 1) {
        solver->initial.left = (PrimitiveState){1.0, 0.0, 1.0};
        solver->initial.right = (PrimitiveState){0.125, 0.0, 0.1};
        solver->time.t_max = 0.2;
    } else if (*case_id == 2) {
        solver->initial.left = (PrimitiveState){0.445, 0.698, 3.528};
        solver->initial.right = (PrimitiveState){0.5, 0.0, 0.571};
        solver->time.t_max = 0.16;
    } else if (*case_id == 3) {
        solver->initial.left = (PrimitiveState){1.0, -2.0, 4.0};
        solver->initial.right = (PrimitiveState){1.0, 2.0, 4.0};
        solver->time.t_max = 0.15;
    } else if (*case_id == 4) {
        solver->initial.left = (PrimitiveState){1.0, -2.0, 0.4};
        solver->initial.right = (PrimitiveState){1.0, 2.0, 0.4};
        solver->time.t_max = 0.15;
    } else if (*case_id == 5) {
        solver->initial.left = (PrimitiveState){1.0, -0.2, 0.5};
        solver->initial.right = (PrimitiveState){0.5, 0.5, 0.5};
        solver->time.t_max = 0.2;
    } else if (*case_id == 6) {
        solver->initial.left = (PrimitiveState){0.4, 0.5, 1.0};
        solver->initial.right = (PrimitiveState){1.0, -0.5, 0.9};
        solver->time.t_max = 0.2;
    } else {
        solver->initial.left = (PrimitiveState){10.0, 1.0, 2.0};
        solver->initial.right = (PrimitiveState){1.0, 1.0, 2.0};
        solver->time.t_max = 0.2;
    }
}

static void configure_domain_and_grid(Solver *solver)
{
    // 配置计算域和 cell 数；最后调用 grid_finalize() 生成 FVM 网格派生量。
    double xmax_default = 0.0;
    double x0_default = 0.0;

    solver->grid.xmin = ask_double_min("xmin", solver->grid.xmin, -1.0e30);

    xmax_default = solver->grid.xmax;
    if (xmax_default <= solver->grid.xmin) {
        xmax_default = solver->grid.xmin + 1.0;
    }
    solver->grid.xmax = ask_double_min("xmax", xmax_default, solver->grid.xmin + 1.0e-12);

    x0_default = solver->grid.x0;
    if (x0_default < solver->grid.xmin || x0_default > solver->grid.xmax) {
        x0_default = 0.5 * (solver->grid.xmin + solver->grid.xmax);
    }
    solver->grid.x0 = ask_double_min("x0", x0_default, solver->grid.xmin);
    if (solver->grid.x0 > solver->grid.xmax) {
        solver->grid.x0 = 0.5 * (solver->grid.xmin + solver->grid.xmax);
    }

    solver->grid.nx = ask_int_range("nx", solver->grid.nx, 5, 200000);
    grid_finalize(&solver->grid);
}

static void configure_numerics(Solver *solver)
{
    // 配置气体参数、CFL 和终止时间。
    solver->gas.gamma = ask_double_min("gamma", solver->gas.gamma, 1.000001);
    solver->time.cfl = ask_double_range("CFL", solver->time.cfl, 1.0e-12, 0.999999);
    solver->time.t_max = ask_double_min("t_max", solver->time.t_max, 0.0);
    solver->time.output_interval =
        ask_int_range("snapshot interval", solver->time.output_interval, 0, 1000000);
}

static void ask_output_filename(char *filename, size_t size, int case_id)
{
    // 默认输出到 runs 文件夹，并把 case 编号和 Roe 方法名写进文件名。
    char line[OUTPUT_NAME_LEN];
    char default_filename[OUTPUT_NAME_LEN];

    snprintf(default_filename, sizeof(default_filename),
             "runs\\case_%02d_roe_numerical.dat", case_id);
    printf("output file [%s]: ", default_filename);
    if (!read_line(line, sizeof(line)) || line[0] == '\0') {
        snprintf(filename, size, "%s", default_filename);
        return;
    }

    snprintf(filename, size, "%s", line);
}

static void make_exact_filename(const char *numerical_filename, char *exact_filename, size_t size)
{
    // 精确解文件名直接由数值解文件名生成，方便一一对应比较。
    const char *dot = strrchr(numerical_filename, '.');
    const char *slash = strrchr(numerical_filename, '\\');
    const char *forward_slash = strrchr(numerical_filename, '/');
    const char *separator = slash;

    if (forward_slash != NULL && (separator == NULL || forward_slash > separator)) {
        separator = forward_slash;
    }

    if (dot != NULL && (separator == NULL || dot > separator)) {
        int prefix_len = (int)(dot - numerical_filename);
        snprintf(exact_filename, size, "%.*s_exact%s", prefix_len, numerical_filename, dot);
    } else {
        snprintf(exact_filename, size, "%s_exact.dat", numerical_filename);
    }
}

static void make_snapshot_filename(
    const char *final_filename,
    int step,
    char *snapshot_filename,
    size_t size
) {
    // 快照文件名格式: case_01_roe_numerical_step_000100.dat。
    const char *dot = strrchr(final_filename, '.');
    const char *slash = strrchr(final_filename, '\\');
    const char *forward_slash = strrchr(final_filename, '/');
    const char *separator = slash;

    if (forward_slash != NULL && (separator == NULL || forward_slash > separator)) {
        separator = forward_slash;
    }

    if (dot != NULL && (separator == NULL || dot > separator)) {
        int prefix_len = (int)(dot - final_filename);
        snprintf(snapshot_filename, size, "%.*s_step_%06d%s",
                 prefix_len, final_filename, step, dot);
    } else {
        snprintf(snapshot_filename, size, "%s_step_%06d.dat", final_filename, step);
    }
}

static const char *find_exact_solver(void)
{
    // 优先找 Project 3 本地 exact solver；找不到就复用 Project 2 中的 exact solver。
    static const char *candidates[] = {
        "_Analysical_Solution_Solver\\riemann_exact.exe",
        "..\\Project_2\\_Analysical_Solution_Solver\\riemann_exact.exe",
        NULL
    };

    for (int i = 0; candidates[i] != NULL; i++) {
        FILE *fp = fopen(candidates[i], "rb");
        if (fp != NULL) {
            fclose(fp);
            return candidates[i];
        }
    }
    return NULL;
}

static int run_exact_solver(const Solver *solver, double output_time, const char *exact_filename)
{
    // 这里不是重新写精确解，而是调用 Project 0/2 已经编译好的 Riemann exact solver。
    char command[COMMAND_LINE_LEN];
    const char *exact_exe = find_exact_solver();

    if (exact_exe == NULL) {
        fprintf(stderr,
                "[Exact Solver Error] Cannot find riemann_exact.exe.\n"
                "Expected either local _Analysical_Solution_Solver or "
                "..\\Project_2\\_Analysical_Solution_Solver.\n");
        return 0;
    }

    snprintf(
        command,
        sizeof(command),
        "\"\"%s\" --batch "
        "%.17g %.17g %.17g "
        "%.17g %.17g %.17g "
        "%.17g %.17g %.17g %.17g %d %.17g \"%s\"\"",
        exact_exe,
        solver->initial.left.rho, solver->initial.left.u, solver->initial.left.p,
        solver->initial.right.rho, solver->initial.right.u, solver->initial.right.p,
        solver->gas.gamma,
        solver->grid.xmin, solver->grid.xmax, solver->grid.x0,
        solver->grid.nx, output_time,
        exact_filename
    );

    // system(command) 会启动外部 exe，并等待它运行结束。
    fflush(stdout);
    if (system(command) != 0) {
        fprintf(stderr, "[Exact Solver Error] External process failed.\n");
        return 0;
    }
    return 1;
}

static void print_banner(void)
{
    // 打印程序标题。
    printf("========================================\n");
    printf("1-D Euler Roe Solver\n");
    printf("========================================\n\n");
}

static void print_run_summary(const Solver *solver, int case_id, const char *filename)
{
    // 正式计算前把所有关键参数打印出来，便于核对本次 run 的配置。
    printf("[Final Check Before Launch]\n");
    printf("case_id      = %d\n", case_id);
    printf("domain       = [%.6f, %.6f], x0 = %.6f\n",
           solver->grid.xmin, solver->grid.xmax, solver->grid.x0);
    printf("grid         = ncell %d, ghost %d each side, dx %.10f\n",
           solver->grid.nx, solver->grid.nghost, solver->grid.dx);
    printf("time         = t_max %.6f, CFL %.6f\n",
           solver->time.t_max, solver->time.cfl);
    printf("gas          = gamma %.6f\n", solver->gas.gamma);
    printf("left state   = rho %.6f, u %.6f, p %.6f\n",
           solver->initial.left.rho,
           solver->initial.left.u,
           solver->initial.left.p);
    printf("right state  = rho %.6f, u %.6f, p %.6f\n",
           solver->initial.right.rho,
           solver->initial.right.u,
           solver->initial.right.p);
    printf("method       = Roe FDS, first-order finite volume\n");
    printf("snapshots    = every %d steps\n", solver->time.output_interval);
    printf("output       = %s\n", filename);
    printf("--------------------------------------------------\n");
}

int main(void)
{
    /*
        主流程保持 Project 2 的外层组织:
            1. 创建默认 solver；
            2. 读取 case/domain/numerics/output；
            3. 分配数组并初始化 Riemann 问题；
            4. 时间循环中反复调用 advance_one_roe_step()；
            5. 输出数值解，并调用 exact solver 生成精确解。

        与 Project 2 的根本区别:
            advance_one_roe_step() 内部不是 filter -> predictor -> corrector，
            而是 boundary -> Roe interface flux -> FVM conservative update。
    */
    Solver solver = solver_create_default();
    char output_filename[OUTPUT_NAME_LEN];
    char exact_filename[OUTPUT_NAME_LEN + 32];
    char snapshot_filename[OUTPUT_NAME_LEN + 64];
    int case_id = 1;

    print_banner();

    configure_riemann_case(&solver, &case_id);
    configure_domain_and_grid(&solver);
    configure_numerics(&solver);
    ask_output_filename(output_filename, sizeof(output_filename), case_id);
    make_exact_filename(output_filename, exact_filename, sizeof(exact_filename));

    print_run_summary(&solver, case_id, output_filename);

    if (!solver_allocate(&solver)) {
        fprintf(stderr, "[Fatal Error] Memory allocation failed.\n");
        solver_destroy(&solver);
        return 1;
    }

    initialize_riemann_problem(&solver);

    if (solver.time.output_interval > 0) {
        // 如果用户要求快照，先输出 t=0 的初始场和对应精确解。
        make_snapshot_filename(output_filename,
                               solver.time.step_count,
                               snapshot_filename,
                               sizeof(snapshot_filename));
        write_tecplot(&solver, snapshot_filename);

        make_exact_filename(snapshot_filename, exact_filename, sizeof(exact_filename));
        if (!run_exact_solver(&solver, solver.time.t, exact_filename)) {
            solver_destroy(&solver);
            return 1;
        }
    }

    while (solver.time.t < solver.time.t_max &&
           solver.time.step_count < solver.time.step_limit) {
        // 时间推进核心；失败通常意味着 Roe 产生了负密度/负压等非物理状态。
        if (!advance_one_roe_step(&solver)) {
            solver_destroy(&solver);
            return 1;
        }

        if (solver.time.output_interval > 0 &&
            solver.time.step_count % solver.time.output_interval == 0) {
            // 按固定步数输出快照，并生成同一时刻的 exact solution。
            make_snapshot_filename(output_filename,
                                   solver.time.step_count,
                                   snapshot_filename,
                                   sizeof(snapshot_filename));
            write_tecplot(&solver, snapshot_filename);

            make_exact_filename(snapshot_filename, exact_filename, sizeof(exact_filename));
            if (!run_exact_solver(&solver, solver.time.t, exact_filename)) {
                solver_destroy(&solver);
                return 1;
            }
        }
    }

    if (solver.time.step_count >= solver.time.step_limit && solver.time.t < solver.time.t_max) {
        // 防止由于 dt 异常变小导致程序无限跑。
        fprintf(stderr, "[Error] Step limit reached before t_max.\n");
        solver_destroy(&solver);
        return 1;
    }

    // 输出最终数值解和对应精确解，供后处理脚本比较。
    write_tecplot(&solver, output_filename);

    make_exact_filename(output_filename, exact_filename, sizeof(exact_filename));
    if (!run_exact_solver(&solver, solver.time.t, exact_filename)) {
        solver_destroy(&solver);
        return 1;
    }

    solver_destroy(&solver);
    return 0;
}
