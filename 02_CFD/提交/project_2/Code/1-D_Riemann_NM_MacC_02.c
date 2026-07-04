#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define INPUT_LINE_LEN 128
#define OUTPUT_NAME_LEN 128
#define COMMAND_LINE_LEN 1024
#define EXACT_SOLVER_EXE "_Analysical_Solution_Solver\\riemann_exact.exe" // 调用 Project 0 中已经写好的 Riemann 精确解程序
#define DEFAULT_SENSOR_EPSILON 1.0e-6 // PPT 中 theta_i 分母里的小量 epsilon，防止分母为 0
#define PHYSICAL_FLOOR 1.0e-12 // 用于检查 rho 和 p 是否已经变成非物理值

/*
    1-D Euler equations:
        原始量 W = [rho, u, p]
        守恒量 Q = [rho, rho*u, rho*E]
        通量   F = [rho*u, rho*u*u + p, u*(rho*E + p)]

    1. filter:
        Qbar_i^n = Q_i^n + 1/2 * nu_i * (Q_{i+1}^n - 2Q_i^n + Q_{i-1}^n)
    2. predictor:
        用 Qbar^n 做一次差分，得到 Qtilde^{n+1}
    3. corrector:
        用 Qtilde^{n+1} 的通量再做反方向差分，得到 Q^{n+1}

    注意:
        current   对应 Q^n
        filtered  对应 Qbar^n
        predicted 对应 Qtilde^{n+1}
        corrected 对应 Q^{n+1}
*/

typedef enum { // 用于判断人工粘性 sensor 取哪个物理量
    VISCOSITY_SENSOR_RHO = 1,   // 用密度 rho 判断激波或间断位置
    VISCOSITY_SENSOR_U = 2,     // 用速度 u 判断间断位置
    VISCOSITY_SENSOR_P = 3      // 用压强 p 判断间断位置
} ViscositySensorType;

typedef enum { // MacCormack 格式内部真正使用的差分方向
    MACCORMACK_FTBS_FTFS = 1,  // 预测步 FTBS，矫正步 FTFS
    MACCORMACK_FTFS_FTBS = 2   // 预测步 FTFS，矫正步 FTBS，是另一种写法
} MacCormackStencil;

typedef enum { // 在程序中选择的时间推进模式
    MACCORMACK_MODE_FIXED_FTBS_FTFS = 1,    // 每一步都使用 FTBS -> FTFS
    MACCORMACK_MODE_FIXED_FTFS_FTBS = 2,    // 每一步都使用 FTFS -> FTBS
    MACCORMACK_MODE_ALTERNATE_FTBS_FIRST = 3, // 两种写法交替使用，第一步从 FTBS -> FTFS 开始
    MACCORMACK_MODE_ALTERNATE_FTFS_FIRST = 4  // 两种写法交替使用，第一步从 FTFS -> FTBS 开始
} MacCormackMode;

// 守恒量数组，实际推进的主变量 Q = [rho, rho*u, rho*E]
typedef struct {
    double *rho;   // q1 = rho，密度，守恒量q_1
    double *rhou;  // q2 = rho*u，动量密度，守恒量q_2
    double *rhoE;  // q3 = rho*E，总能量密度，守恒量q_3
} Conserved;

// 原始量数组，由守恒量反算得到，用于输出、CFL 和人工粘性 sensor
typedef struct {
    double *rho;   // 密度，用于输出和计算声速
    double *u;     // 速度，一维问题只有一个速度分量
    double *p;     // 压强
    double *E;     // 单位质量总能量 E = rhoE / rho
    double *a;     // 局部声速 a = sqrt(gamma*p/rho)，用于 CFL 条件
} Primitive;

// Euler 方程通量数组 F(Q)
typedef struct {
    double *mass;      // 质量方程通量: rho*u
    double *momentum;  // 动量方程通量: rho*u*u + p
    double *energy;    // 能量方程通量: u*(rhoE + p)
} Flux;

// Riemann 问题某一侧的原始状态
typedef struct {
    double rho; // 某一侧初值的密度
    double u;   // 某一侧初值的速度
    double p;   // 某一侧初值的压强
} PrimitiveState;

// 一维均匀网格信息
typedef struct {
    int nx;       // 网格点数
    double xmin;  // 计算区域左端点
    double xmax;  // 计算区域右端点
    double x0;    // 初始间断位置
    double dx;    // 空间步长
} Grid1D;

// Riemann 问题左右两侧初值
typedef struct {
    PrimitiveState left;   // 间断左侧的初始状态
    PrimitiveState right;  // 间断右侧的初始状态
} RiemannInitialCondition;

// 理想气体参数
typedef struct {
    double gamma; // 理想气体比热比，本作业默认取 1.4
} GasModel;

// 时间推进参数
typedef struct {
    double cfl;      // 目标 CFL，稳定性要求小于 1
    double t;        // 当前物理时间
    double t_max;    // 计算终止时间
    double dt;       // 当前时间步长，每一步由 CFL 重新计算
    int step_count;  // 已经完成的迭代步数
    int step_limit;  // 最大迭代步数，防止异常情况下无限循环
    int output_interval; // 快照输出间隔，0 表示只输出最终结果
} TimeControl;

// 人工粘性滤波参数
typedef struct {
    int enabled;     // 人工粘性开关
    double beta;     // beta，量级取 O(1)，依赖于经验，需要尝试输入不同的值测试
    double sensor_epsilon; // theta_i 分母里的 epsilon
    ViscositySensorType sensor_type; // theta_i 用 rho、u 还是 p 来计算
} ArtificialViscosity;

// MacCormack 差分方向控制
typedef struct {
    MacCormackMode mode; // 固定一种写法，或者两种写法交替
} MacCormackControl;

// MacCormack 一步推进中用到的四个守恒量时间层
typedef struct {
    Conserved current;    // 当前时间层 Q^n，是程序真正推进的主状态
    Conserved filtered;   // 过滤后得到的 Qbar^n
    Conserved predicted;  // 预测步结果 Qtilde^{n+1}
    Conserved corrected;  // 矫正步结果 Q^{n+1}，算完后复制回 current，实现迭代
} TimeLayers;

// MacCormack 预测步和矫正步需要用到的两组通量
typedef struct {
    Flux filtered;   // F(Qbar^n)，预测步需要用
    Flux predicted;  // F(Qtilde^{n+1})，矫正步需要用
} FluxLayers;

// 总求解器，把网格、物理参数、时间层和辅助量放在一起
typedef struct {
    Grid1D grid;                     // 网格
    GasModel gas;                    // 气体
    RiemannInitialCondition initial; // 初值
    TimeControl time;                // 时间
    ArtificialViscosity av;          // 人工粘性
    MacCormackControl maccormack;    // MacCormack 格式

    TimeLayers state;     // 时间层
    FluxLayers flux;      // 通量层
    Primitive primitive;  // 原始量缓存
    double *nu;           // 人工粘性系数
} Solver;

/*
===========接下来开辟空间--------------------------
*/


static double *alloc_double_array(size_t n) { // 给长度为 n 的 double 数组开空间，并全部初始化为 0
    return (double *)calloc(n, sizeof(double));
}

static int conserved_alloc(Conserved *q, size_t n) { // 给一整组守恒量 Q=[rho,rhou,rhoE] 开空间
    q->rho = alloc_double_array(n);
    q->rhou = alloc_double_array(n);
    q->rhoE = alloc_double_array(n);
    return q->rho != NULL && q->rhou != NULL && q->rhoE != NULL;
}

static void conserved_free(Conserved *q) { // 释放一整组守恒量，避免三个数组漏掉某一个
    free(q->rho);
    free(q->rhou);
    free(q->rhoE);
    q->rho = NULL;
    q->rhou = NULL;
    q->rhoE = NULL;
}

static void conserved_copy(const Solver *solver, Conserved *dst, const Conserved *src) { // Q 层之间整组复制
    size_t bytes = sizeof(double) * (size_t)solver->grid.nx;
    memcpy(dst->rho, src->rho, bytes);
    memcpy(dst->rhou, src->rhou, bytes);
    memcpy(dst->rhoE, src->rhoE, bytes);
}

static int flux_alloc(Flux *flux, size_t n) { // 给一整组通量 F=[mass,momentum,energy] 开空间
    flux->mass = alloc_double_array(n);
    flux->momentum = alloc_double_array(n);
    flux->energy = alloc_double_array(n);
    return flux->mass != NULL && flux->momentum != NULL && flux->energy != NULL;
}

static void flux_free(Flux *flux) { // 释放通量数组
    free(flux->mass);
    free(flux->momentum);
    free(flux->energy);
    flux->mass = NULL;
    flux->momentum = NULL;
    flux->energy = NULL;
}

static int primitive_alloc(Primitive *primitive, size_t n) { // 给原始量 W=[rho,u,p,E,a] 开空间
    primitive->rho = alloc_double_array(n);
    primitive->u = alloc_double_array(n);
    primitive->p = alloc_double_array(n);
    primitive->E = alloc_double_array(n);
    primitive->a = alloc_double_array(n);
    return primitive->rho != NULL &&
           primitive->u != NULL &&
           primitive->p != NULL &&
           primitive->E != NULL &&
           primitive->a != NULL;
}

static void primitive_free(Primitive *primitive) { // 释放原始量数组
    free(primitive->rho);
    free(primitive->u);
    free(primitive->p);
    free(primitive->E);
    free(primitive->a);
    primitive->rho = NULL;
    primitive->u = NULL;
    primitive->p = NULL;
    primitive->E = NULL;
    primitive->a = NULL;
}

static double pressure_from_conserved(
    const Solver *solver,
    double rho,
    double rhou,
    double rhoE
) {

    /*
        由守恒量反算压强:
        rhoE = p/(gamma-1) + 1/2*rho*u^2
        其中 rho*u = rhou，所以动能密度写成 1/2*rhou^2/rho
    */

    double kinetic_energy_density = 0.5 * rhou * rhou / rho;
    return (solver->gas.gamma - 1.0) * (rhoE - kinetic_energy_density);
}

static double total_energy_density(
    const Solver *solver,
    double rho,
    double u,
    double p
) {
    // 由原始量构造总能量密度 rhoE，初始化 Riemann 问题时使用
    return p / (solver->gas.gamma - 1.0) + 0.5 * rho * u * u;
}

static const double *viscosity_sensor_values(const Solver *solver) { // 返回人工粘性 theta_i 要看的那一列数据
    switch (solver->av.sensor_type) {
        case VISCOSITY_SENSOR_RHO:
            return solver->primitive.rho;
        case VISCOSITY_SENSOR_U:
            return solver->primitive.u;
        case VISCOSITY_SENSOR_P:
            return solver->primitive.p;
        default:
            return solver->primitive.rho;
    }
}

static const char *viscosity_sensor_name(ViscositySensorType type) { // 把 sensor 选择转成输出时能看懂的名字
    switch (type) {
        case VISCOSITY_SENSOR_RHO:
            return "rho";
        case VISCOSITY_SENSOR_U:
            return "u";
        case VISCOSITY_SENSOR_P:
            return "p";
        default:
            return "rho";
    }
}

static const char *maccormack_mode_name(MacCormackMode mode) { // 把 MacCormack 模式转成屏幕提示文字
    switch (mode) {
        case MACCORMACK_MODE_FIXED_FTBS_FTFS:
            return "fixed predictor FTBS / corrector FTFS";
        case MACCORMACK_MODE_FIXED_FTFS_FTBS:
            return "fixed predictor FTFS / corrector FTBS";
        case MACCORMACK_MODE_ALTERNATE_FTBS_FIRST:
            return "alternate, FTBS/FTFS first";
        case MACCORMACK_MODE_ALTERNATE_FTFS_FIRST:
            return "alternate, FTFS/FTBS first";
        default:
            return "fixed predictor FTBS / corrector FTFS";
    }
}

static MacCormackStencil stencil_for_current_step(const Solver *solver) {

    /*
        根据用户选择，决定这一时间步用哪一种 MacCormack 写法。
        交替模式下用 step_count 的奇偶性切换方向。
    */

    switch (solver->maccormack.mode) {
        case MACCORMACK_MODE_FIXED_FTBS_FTFS:
            return MACCORMACK_FTBS_FTFS;
        case MACCORMACK_MODE_FIXED_FTFS_FTBS:
            return MACCORMACK_FTFS_FTBS;
        case MACCORMACK_MODE_ALTERNATE_FTBS_FIRST:
            return (solver->time.step_count % 2 == 0)
                ? MACCORMACK_FTBS_FTFS
                : MACCORMACK_FTFS_FTBS;
        case MACCORMACK_MODE_ALTERNATE_FTFS_FIRST:
            return (solver->time.step_count % 2 == 0)
                ? MACCORMACK_FTFS_FTBS
                : MACCORMACK_FTBS_FTFS;
        default:
            return MACCORMACK_FTBS_FTFS;
    }
}

static int solver_allocate(Solver *solver) { // 给整个 solver 需要的数组一次性开好空间
    size_t n = (size_t)solver->grid.nx;

    // current/filtered/predicted/corrected 每一层都包含三个守恒量
    if (!conserved_alloc(&solver->state.current, n) ||
        !conserved_alloc(&solver->state.filtered, n) ||
        !conserved_alloc(&solver->state.predicted, n) ||
        !conserved_alloc(&solver->state.corrected, n) ||
        !flux_alloc(&solver->flux.filtered, n) ||
        !flux_alloc(&solver->flux.predicted, n) ||
        !primitive_alloc(&solver->primitive, n)) {
        return 0;
    }

    solver->nu = alloc_double_array(n); // nu_i 单独保存，主要为了观察人工粘性的分布
    return solver->nu != NULL;
}

static void solver_destroy(Solver *solver) { // 程序结束前释放所有堆上的内存
    conserved_free(&solver->state.current);
    conserved_free(&solver->state.filtered);
    conserved_free(&solver->state.predicted);
    conserved_free(&solver->state.corrected);
    flux_free(&solver->flux.filtered);
    flux_free(&solver->flux.predicted);
    primitive_free(&solver->primitive);
    free(solver->nu);
    memset(solver, 0, sizeof(*solver));
}

static void compute_primitive_from_current(Solver *solver) { // 由当前守恒量 Q^n 反算原始量 W
    const Conserved *q = &solver->state.current;
    Primitive *w = &solver->primitive;

    for (int i = 0; i < solver->grid.nx; i++) {
        w->rho[i] = q->rho[i];              // rho 本身就是第一守恒量
        w->u[i] = q->rhou[i] / q->rho[i];   // u = rho*u / rho
        w->p[i] = pressure_from_conserved(
            solver,
            q->rho[i],
            q->rhou[i],
            q->rhoE[i]
        );
        w->E[i] = q->rhoE[i] / q->rho[i];   // E = rhoE / rho

        if (w->rho[i] > 0.0 && w->p[i] > 0.0) {
            w->a[i] = sqrt(solver->gas.gamma * w->p[i] / w->rho[i]);
        } else {
            w->a[i] = 0.0;
        }
    }
}

static void compute_flux_from_conserved(
    const Solver *solver,
    const Conserved *q,
    Flux *flux
) { // 由守恒量 Q 计算 Euler 方程通量 F(Q)
    for (int i = 0; i < solver->grid.nx; i++) {
        double rho = q->rho[i];
        double u = q->rhou[i] / rho;
        double p = pressure_from_conserved(solver, q->rho[i], q->rhou[i], q->rhoE[i]);

        flux->mass[i] = q->rhou[i];              // F1 = rho*u
        flux->momentum[i] = q->rhou[i] * u + p;  // F2 = rho*u^2 + p
        flux->energy[i] = (q->rhoE[i] + p) * u;  // F3 = u*(rhoE+p)
    }
}

static double compute_max_wave_speed(Solver *solver) { // 计算 max(|u|+a)，也就是一维 Euler 的谱半径
    double max_speed = 0.0;

    compute_primitive_from_current(solver);
    for (int i = 0; i < solver->grid.nx; i++) {
        double speed = fabs(solver->primitive.u[i]) + solver->primitive.a[i];
        if (speed > max_speed) {
            max_speed = speed;
        }
    }

    return max_speed;
}

static double compute_dt_from_cfl(Solver *solver) { // 根据 CFL = max(|u|+a)*dt/dx 反算 dt
    double max_speed = compute_max_wave_speed(solver);
    if (max_speed <= 0.0) {
        return 1.0e-8;
    }
    return solver->time.cfl * solver->grid.dx / max_speed;
}

static void apply_zero_gradient_boundary(const Solver *solver, Conserved *q) { // 零梯度边界: 边界点直接复制相邻内点
    int last = solver->grid.nx - 1;

    q->rho[0] = q->rho[1];
    q->rhou[0] = q->rhou[1];
    q->rhoE[0] = q->rhoE[1];

    q->rho[last] = q->rho[last - 1];
    q->rhou[last] = q->rhou[last - 1];
    q->rhoE[last] = q->rhoE[last - 1];
}

static int conserved_state_is_physical(const Solver *solver, const Conserved *q) {
    for (int i = 0; i < solver->grid.nx; i++) {
        double rho = q->rho[i];
        double rhou = q->rhou[i];
        double rhoE = q->rhoE[i];
        double p = 0.0;

        if (!isfinite(rho) || !isfinite(rhou) || !isfinite(rhoE)) {
            return 0;
        }
        if (rho <= PHYSICAL_FLOOR) {
            return 0;
        }

        p = pressure_from_conserved(solver, rho, rhou, rhoE);
        if (!isfinite(p) || p <= PHYSICAL_FLOOR) {
            return 0;
        }
    }

    return 1;
}

/*
======================== 4. 人工粘性滤波与 MacCormack 推进 ========================
*/


static void apply_artificial_viscosity_filter(Solver *solver) { // 按 PPT 公式先对 Q^n 做滤波，得到 Qbar^n
    double max_speed = 0.0;
    double actual_cfl = 0.0;
    const double *sensor = NULL;
    Conserved *q = &solver->state.current;
    Conserved *filtered = &solver->state.filtered;


    /*
        默认情况下 filtered 先等于 current。
        如果用户关闭人工粘性，那么后面直接用这个 filtered 进入 MacCormack。
    */

    conserved_copy(solver, filtered, q);
    for (int i = 0; i < solver->grid.nx; i++) {
        solver->nu[i] = 0.0; // 每一步重新计算 nu_i，不能沿用上一时间步的值
    }

    if (!solver->av.enabled || solver->av.beta <= 0.0) {
        // 人工粘性关闭时，Qbar^n = Q^n，只需要补一下边界
        apply_zero_gradient_boundary(solver, filtered);
        return;
    }

    max_speed = compute_max_wave_speed(solver);
    if (max_speed <= 0.0) {
        // 极端情况下没有传播速度，就不加滤波，避免除以 0
        apply_zero_gradient_boundary(solver, filtered);
        return;
    }

    // PPT 中 CFL = rho(A)*dt/dx，这里 rho(A) = max(|u|+a)
    actual_cfl = max_speed * solver->time.dt / solver->grid.dx;

    // sensor 可以是 rho/u/p，这是老师要求保留的三种人工粘性算法
    sensor = viscosity_sensor_values(solver);

    for (int i = 1; i < solver->grid.nx - 1; i++) {
        // theta_i 的分子: sensor 的二阶差分，间断附近会变大
        double second_sensor = sensor[i + 1] - 2.0 * sensor[i] + sensor[i - 1];

        // theta_i 的分母: 左右一阶差分的绝对值之和，再加 epsilon 防止分母为 0
        double first_sensor_sum =
            fabs(sensor[i + 1] - sensor[i]) +
            fabs(sensor[i] - sensor[i - 1]) +
            solver->av.sensor_epsilon;

        // theta_i 只决定哪里需要加粘性；beta 决定加多少
        double theta = fabs(second_sensor) / first_sensor_sum;
        double nu = actual_cfl * (1.0 - actual_cfl) * solver->av.beta * theta;

        solver->nu[i] = nu; // 存下来，输出文件里可以看人工粘性到底加在哪里


        /*
            PPT 的 filter 公式:
            Qbar_i = Q_i + 1/2*nu_i*(Q_{i+1}-2Q_i+Q_{i-1})
            三个守恒量 rho、rhou、rhoE 都用同一个 nu_i。
        */

        filtered->rho[i] = q->rho[i] + 0.5 * nu *
            (q->rho[i + 1] - 2.0 * q->rho[i] + q->rho[i - 1]);
        filtered->rhou[i] = q->rhou[i] + 0.5 * nu *
            (q->rhou[i + 1] - 2.0 * q->rhou[i] + q->rhou[i - 1]);
        filtered->rhoE[i] = q->rhoE[i] + 0.5 * nu *
            (q->rhoE[i + 1] - 2.0 * q->rhoE[i] + q->rhoE[i - 1]);
    }

    apply_zero_gradient_boundary(solver, filtered);
}

static void compute_maccormack_predictor(Solver *solver, MacCormackStencil stencil) { // 预估步: Qbar^n -> Qtilde^{n+1}
    double lambda = solver->time.dt / solver->grid.dx;
    const Conserved *filtered = &solver->state.filtered;
    Conserved *predicted = &solver->state.predicted;
    const Flux *flux = &solver->flux.filtered;

    if (stencil == MACCORMACK_FTBS_FTFS) {

        /*
            写法 1: 预估步用 FTBS，也就是 F_i - F_{i-1}
            这个分支对应 PPT 里 predictor 的方向。
        */

        for (int i = 1; i < solver->grid.nx - 1; i++) {
            predicted->rho[i] = filtered->rho[i]
                - lambda * (flux->mass[i] - flux->mass[i - 1]);
            predicted->rhou[i] = filtered->rhou[i]
                - lambda * (flux->momentum[i] - flux->momentum[i - 1]);
            predicted->rhoE[i] = filtered->rhoE[i]
                - lambda * (flux->energy[i] - flux->energy[i - 1]);
        }
    } else {
        // 写法 2: 预估步用 FTFS，也就是 F_{i+1} - F_i
        for (int i = 1; i < solver->grid.nx - 1; i++) {
            predicted->rho[i] = filtered->rho[i]
                - lambda * (flux->mass[i + 1] - flux->mass[i]);
            predicted->rhou[i] = filtered->rhou[i]
                - lambda * (flux->momentum[i + 1] - flux->momentum[i]);
            predicted->rhoE[i] = filtered->rhoE[i]
                - lambda * (flux->energy[i + 1] - flux->energy[i]);
        }
    }

    apply_zero_gradient_boundary(solver, predicted);
}

static void compute_maccormack_corrector(Solver *solver, MacCormackStencil stencil) { // 矫正步: Qbar^n 和 Qtilde^{n+1} 平均后修正
    double lambda = solver->time.dt / solver->grid.dx;
    const Conserved *filtered = &solver->state.filtered;
    const Conserved *predicted = &solver->state.predicted;
    Conserved *corrected = &solver->state.corrected;
    const Flux *flux = &solver->flux.predicted;

    if (stencil == MACCORMACK_FTBS_FTFS) {

        /*
            如果预估步用了 FTBS，那么矫正步反过来用 FTFS。
            这里的 0.5*lambda 对应 PPT 中 dt/(2*dx)。
        */

        for (int i = 1; i < solver->grid.nx - 1; i++) {
            corrected->rho[i] =
                0.5 * (filtered->rho[i] + predicted->rho[i])
                - 0.5 * lambda * (flux->mass[i + 1] - flux->mass[i]);
            corrected->rhou[i] =
                0.5 * (filtered->rhou[i] + predicted->rhou[i])
                - 0.5 * lambda * (flux->momentum[i + 1] - flux->momentum[i]);
            corrected->rhoE[i] =
                0.5 * (filtered->rhoE[i] + predicted->rhoE[i])
                - 0.5 * lambda * (flux->energy[i + 1] - flux->energy[i]);
        }
    } else {

        /*
            如果预估步用了 FTFS，那么矫正步反过来用 FTBS。
            两个方向反着配，是 MacCormack 二阶精度的关键。
        */

        for (int i = 1; i < solver->grid.nx - 1; i++) {
            corrected->rho[i] =
                0.5 * (filtered->rho[i] + predicted->rho[i])
                - 0.5 * lambda * (flux->mass[i] - flux->mass[i - 1]);
            corrected->rhou[i] =
                0.5 * (filtered->rhou[i] + predicted->rhou[i])
                - 0.5 * lambda * (flux->momentum[i] - flux->momentum[i - 1]);
            corrected->rhoE[i] =
                0.5 * (filtered->rhoE[i] + predicted->rhoE[i])
                - 0.5 * lambda * (flux->energy[i] - flux->energy[i - 1]);
        }
    }

    apply_zero_gradient_boundary(solver, corrected);
}

static void initialize_riemann_problem(Solver *solver) { // 按左右初值给 current=Q^0 赋值
    Conserved *q = &solver->state.current;

    for (int i = 0; i < solver->grid.nx; i++) {
        double x = solver->grid.xmin + i * solver->grid.dx;
        PrimitiveState state = solver->initial.right;

        if (x < solver->grid.x0) {
            state = solver->initial.left; // x0 左边取左状态，x0 右边取右状态
        }

        q->rho[i] = state.rho; // 原始量转守恒量
        q->rhou[i] = state.rho * state.u;
        q->rhoE[i] = total_energy_density(solver, state.rho, state.u, state.p);
    }

    apply_zero_gradient_boundary(solver, q);
    compute_primitive_from_current(solver);
}

static int advance_one_maccormack_step(Solver *solver) { // 完成一个完整时间步: filter -> predictor -> corrector
    MacCormackStencil stencil = MACCORMACK_FTBS_FTFS;

    // 先由 CFL 条件算本步 dt
    solver->time.dt = compute_dt_from_cfl(solver);
    if (solver->time.t + solver->time.dt > solver->time.t_max) {
        // 最后一步不要超过用户给定的 t_max
        solver->time.dt = solver->time.t_max - solver->time.t;
    }

    // 固定模式下每步一样；交替模式下根据 step_count 自动切换
    stencil = stencil_for_current_step(solver);

    // 这几行就是本程序最核心的数值流程，对应 PPT 的公式顺序
    apply_artificial_viscosity_filter(solver);
    if (!conserved_state_is_physical(solver, &solver->state.filtered)) {
        fprintf(stderr, "[Error] Nonphysical state after artificial-viscosity filter.\n");
        return 0;
    }

    compute_flux_from_conserved(solver, &solver->state.filtered, &solver->flux.filtered);
    compute_maccormack_predictor(solver, stencil);
    if (!conserved_state_is_physical(solver, &solver->state.predicted)) {
        fprintf(stderr, "[Error] Nonphysical state after MacCormack predictor.\n");
        return 0;
    }

    compute_flux_from_conserved(solver, &solver->state.predicted, &solver->flux.predicted);
    compute_maccormack_corrector(solver, stencil);
    if (!conserved_state_is_physical(solver, &solver->state.corrected)) {
        fprintf(stderr, "[Error] Nonphysical state after MacCormack corrector.\n");
        return 0;
    }

    // 矫正后的 Q^{n+1} 成为下一步的 Q^n
    conserved_copy(solver, &solver->state.current, &solver->state.corrected);
    solver->time.t += solver->time.dt;
    solver->time.step_count++;
    return 1;
}

static void write_tecplot(Solver *solver, const char *filename) { // 输出 Tecplot 可读的 dat 文件
    FILE *fp = fopen(filename, "w");
    const Conserved *q = &solver->state.current;
    const Primitive *w = &solver->primitive;

    if (fp == NULL) {
        printf("[Error] Cannot open output file: %s\n", filename);
        return;
    }

    compute_primitive_from_current(solver);

    // 同时输出原始量和守恒量，方便后处理时检查是否有不合理的 rho 或 p
    fprintf(fp, "TITLE = \"1D Euler Filtered MacCormack Result\"\n");
    fprintf(fp,
            "VARIABLES = \"x\", \"rho\", \"u\", \"p\", \"E\", "
            "\"rho_conserved\", \"rhou\", \"rhoE\", \"nu\"\n");
    fprintf(fp, "ZONE T=\"t=%.8f\", I=%d, F=POINT\n",
            solver->time.t,
            solver->grid.nx);

    for (int i = 0; i < solver->grid.nx; i++) {
        double x = solver->grid.xmin + i * solver->grid.dx;
        fprintf(fp,
                "%.10f %.10f %.10f %.10f %.10f %.10f %.10f %.10f %.10f\n",
                x,
                w->rho[i],
                w->u[i],
                w->p[i],
                w->E[i],
                q->rho[i],
                q->rhou[i],
                q->rhoE[i],
                solver->nu[i]);
    }

    fclose(fp);
}

/*
======================== 5. 默认参数和交互输入 ========================
*/

/// 给 solver 一个默认设置，用户不输入时就用这些值
static Solver solver_create_default(void) { 
    Solver solver;
    memset(&solver, 0, sizeof(solver)); // 先清零，避免指针里有随机地址

    // 默认用 Sod shock tube 的常用区域 [0,1] 和间断位置 x=0.5
    solver.grid.nx = 501;
    solver.grid.xmin = 0.0;
    solver.grid.xmax = 1.0;
    solver.grid.x0 = 0.5;
    solver.grid.dx = (solver.grid.xmax - solver.grid.xmin) /
                     (double)(solver.grid.nx - 1);

    solver.gas.gamma = 1.4;

    // 默认 CFL=0.5，比 CFL<1 更保守一些
    solver.time.cfl = 0.5;
    solver.time.t = 0.0;
    solver.time.t_max = 0.2;
    solver.time.dt = 0.0;
    solver.time.step_count = 0;
    solver.time.step_limit = 200000;
    solver.time.output_interval = 0;

    // 默认初值就是 Sod 问题
    solver.initial.left.rho = 1.0;
    solver.initial.left.u = 0.0;
    solver.initial.left.p = 1.0;
    solver.initial.right.rho = 0.125;
    solver.initial.right.u = 0.0;
    solver.initial.right.p = 0.1;

    // 默认打开人工粘性，用 rho 做 sensor
    solver.av.enabled = 1;
    solver.av.beta = 0.5;
    solver.av.sensor_epsilon = DEFAULT_SENSOR_EPSILON;
    solver.av.sensor_type = VISCOSITY_SENSOR_RHO;

    // 默认使用 PPT 中的顺序: predictor FTBS, corrector FTFS
    solver.maccormack.mode = MACCORMACK_MODE_FIXED_FTBS_FTFS;

    return solver;
}

static int read_line(char *buffer, size_t size) { // 读取一整行用户输入，并去掉换行符
    if (fgets(buffer, (int)size, stdin) == NULL) {
        return 0;
    }
    buffer[strcspn(buffer, "\r\n")] = '\0';
    return 1;
}

static int ask_int_range(
    const char *prompt,
    int default_value,
    int min_value,
    int max_value
) {
    char line[INPUT_LINE_LEN];
    int value = default_value;

    while (1) {
        // 直接回车表示采用默认值
        printf("%s [%d]: ", prompt, default_value);
        if (!read_line(line, sizeof(line)) || line[0] == '\0') {
            return default_value;
        }

        if (sscanf(line, "%d", &value) == 1 &&
            value >= min_value &&
            value <= max_value) {
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
    char line[INPUT_LINE_LEN];
    double value = default_value;

    while (1) {
        // 这里限制上下界，主要是防止 CFL、网格范围等输入成明显错误的值
        printf("%s [%.6g]: ", prompt, default_value);
        if (!read_line(line, sizeof(line)) || line[0] == '\0') {
            return default_value;
        }

        if (sscanf(line, "%lf", &value) == 1 &&
            value >= min_value &&
            value <= max_value) {
            return value;
        }

        printf("Invalid input.\n");
    }
}

static double ask_double_min(
    const char *prompt,
    double default_value,
    double min_value
) {
    char line[INPUT_LINE_LEN];
    double value = default_value;

    while (1) {
        // 只限制最小值，不限制最大值，比如 xmin 可以给很小的负数
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

static int ask_yes_no(const char *prompt, int default_value) { // 把 y/n 输入转成 1/0
    char line[INPUT_LINE_LEN];

    while (1) {
        printf("%s [%c]: ", prompt, default_value ? 'y' : 'n');
        if (!read_line(line, sizeof(line)) || line[0] == '\0') {
            return default_value;
        }

        if (line[0] == 'y' || line[0] == 'Y') {
            return 1;
        }
        if (line[0] == 'n' || line[0] == 'N') {
            return 0;
        }

        printf("Invalid input.\n");
    }
}

static void ask_output_filename(
    char *filename,
    size_t size,
    int case_id
) {
    char line[OUTPUT_NAME_LEN];
    char default_filename[OUTPUT_NAME_LEN];

    // 默认输出到 runs 文件夹，并把 case 编号写进文件名
    snprintf(default_filename, sizeof(default_filename),
             "runs\\case_%02d_numerical_02.dat", case_id);
    printf("output file [%s]: ", default_filename);
    if (!read_line(line, sizeof(line)) || line[0] == '\0') {
        snprintf(filename, size, "%s", default_filename);
        return;
    }

    snprintf(filename, size, "%s", line);
}

static void make_exact_filename(
    const char *numerical_filename,
    char *exact_filename,
    size_t size
) {
    const char *dot = strrchr(numerical_filename, '.');
    const char *slash = strrchr(numerical_filename, '\\');
    const char *forward_slash = strrchr(numerical_filename, '/');
    const char *separator = slash;

    // 精确解文件名直接由数值解文件名生成，方便一一对应比较
    if (forward_slash != NULL && (separator == NULL || forward_slash > separator)) {
        separator = forward_slash;
    }

    if (dot != NULL && (separator == NULL || dot > separator)) {
        int prefix_len = (int)(dot - numerical_filename);
        snprintf(exact_filename, size, "%.*s_exact%s",
                 prefix_len, numerical_filename, dot);
    } else {
        snprintf(exact_filename, size, "%s_exact.dat", numerical_filename);
    }
}

static int run_exact_solver(
    const Solver *solver,
    double output_time,
    const char *exact_filename
) {
    char command[COMMAND_LINE_LEN];
    FILE *exact_executable = fopen(EXACT_SOLVER_EXE, "rb");

    // 这里不是重新写精确解，而是调用 Project 0 已经编译好的程序
    if (exact_executable == NULL) {
        fprintf(stderr,
                "[Exact Solver Error] Cannot find %s\n"
                "Compile it first with:\n"
                "gcc _Analysical_Solution_Solver\\1-D_Riemann_AM.c "
                "-std=c11 -O2 -Wall -Wextra "
                "-o _Analysical_Solution_Solver\\riemann_exact.exe -lm\n",
                EXACT_SOLVER_EXE);
        return 0;
    }
    fclose(exact_executable);

    // 把左右状态、gamma、区域、网格数、输出时间都作为命令行参数传给精确解程序
    snprintf(
        command,
        sizeof(command),
        "\"\"%s\" --batch "
        "%.17g %.17g %.17g "
        "%.17g %.17g %.17g "
        "%.17g %.17g %.17g %.17g %d %.17g \"%s\"\"",
        EXACT_SOLVER_EXE,
        solver->initial.left.rho, solver->initial.left.u, solver->initial.left.p,
        solver->initial.right.rho, solver->initial.right.u, solver->initial.right.p,
        solver->gas.gamma,
        solver->grid.xmin, solver->grid.xmax, solver->grid.x0,
        solver->grid.nx, output_time,
        exact_filename
    );

    fflush(stdout);
    // system(command) 会启动外部 exe，并等待它运行结束
    if (system(command) != 0) {
        fprintf(stderr, "[Exact Solver Error] External process failed.\n");
        return 0;
    }
    return 1;
}

static void make_snapshot_filename(
    const char *final_filename,
    int step,
    char *snapshot_filename,
    size_t size
) {
    const char *dot = strrchr(final_filename, '.');
    const char *slash = strrchr(final_filename, '\\');
    const char *forward_slash = strrchr(final_filename, '/');
    const char *separator = slash;
    int prefix_len = 0;

    // 快照文件名格式: 原文件名_step_000100.dat
    if (forward_slash != NULL && (separator == NULL || forward_slash > separator)) {
        separator = forward_slash;
    }

    if (dot != NULL && (separator == NULL || dot > separator)) {
        prefix_len = (int)(dot - final_filename);
        snprintf(snapshot_filename, size, "%.*s_step_%06d%s",
                 prefix_len, final_filename, step, dot);
    } else {
        snprintf(snapshot_filename, size, "%s_step_%06d.dat",
                 final_filename, step);
    }
}

static void print_banner(void) { // 打印程序标题，并把 Windows 控制台切到 UTF-8
    system("chcp 65001 > nul");
    printf("========================================\n");
    printf("1-D Euler Filtered MacCormack Solver 02\n");
    printf("========================================\n\n");
}

static void configure_riemann_case(Solver *solver, int *case_id) { // 选择 7 组典型 Riemann 初值
    printf("case: 1-Sod 2-Lax 3-SubExp 4-Sjogreen 5-ContactExp 6-ContactShock 7-Contact\n");
    *case_id = ask_int_range("case", 1, 1, 7);

    // 每个 case 都给出 left/right 两侧的 rho,u,p，同时给一个常用终止时间
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

static void configure_domain_and_grid(Solver *solver) { // 输入计算区域和网格点数
    double xmax_default = 0.0;
    double x0_default = 0.0;

    solver->grid.xmin = ask_double_min("xmin", solver->grid.xmin, -1.0e30);

    xmax_default = solver->grid.xmax;
    if (xmax_default <= solver->grid.xmin) {
        xmax_default = solver->grid.xmin + 1.0;
    }
    solver->grid.xmax = ask_double_min("xmax",
                                       xmax_default,
                                       solver->grid.xmin + 1.0e-12);

    x0_default = solver->grid.x0;
    if (x0_default < solver->grid.xmin || x0_default > solver->grid.xmax) {
        x0_default = 0.5 * (solver->grid.xmin + solver->grid.xmax);
    }
    solver->grid.x0 = ask_double_min("x0",
                                     x0_default,
                                     solver->grid.xmin);

    // 如果 x0 超出计算区域，就退回到区域中心，避免初值全在同一侧
    if (solver->grid.x0 > solver->grid.xmax) {
        solver->grid.x0 = 0.5 * (solver->grid.xmin + solver->grid.xmax);
    }

    solver->grid.nx = ask_int_range("nx", solver->grid.nx, 5, 200000);

    // 均匀网格，所以 dx 由区域长度和网格点数自动给出
    solver->grid.dx = (solver->grid.xmax - solver->grid.xmin) /
                      (double)(solver->grid.nx - 1);
}

static void configure_numerics(Solver *solver) { // 输入 CFL、人工粘性和 MacCormack 写法
    int mode_value = 1;

    solver->gas.gamma = ask_double_min("gamma", solver->gas.gamma, 1.000001);
    solver->time.cfl = ask_double_range("CFL",
                                        solver->time.cfl,
                                        1.0e-12,
                                        0.999999);
    solver->time.t_max = ask_double_min("t_max",
                                        solver->time.t_max,
                                        0.0);

    // 这里是 02 版新增的功能: 可以固定一种写法，也可以两种写法交替
    printf("MacCormack: 1-FTBS/FTFS 2-FTFS/FTBS 3-alt-FTBS-first 4-alt-FTFS-first\n");
    mode_value = ask_int_range("mode",
                               (int)solver->maccormack.mode,
                               1,
                               4);
    solver->maccormack.mode = (MacCormackMode)mode_value;

    solver->av.enabled = ask_yes_no("artificial viscosity",
                                    solver->av.enabled);

    if (solver->av.enabled) {
        // beta 越大，间断附近被抹平得越明显；太小可能压不住振荡
        solver->av.beta = ask_double_min("beta",
                                         solver->av.beta,
                                         0.0);
        // 三种 sensor 都保留，便于比较 rho/u/p 对滤波位置判断的影响
        printf("sensor: 1-rho 2-u 3-p\n");
        solver->av.sensor_type = (ViscositySensorType)
            ask_int_range("sensor",
                          (int)solver->av.sensor_type,
                          (int)VISCOSITY_SENSOR_RHO,
                          (int)VISCOSITY_SENSOR_P);
    } else {
        // 关闭人工粘性时，beta 也置零，输出摘要更直观
        solver->av.beta = 0.0;
    }

    solver->time.output_interval =
        ask_int_range("snapshot interval",
                      solver->time.output_interval,
                      0,
                      1000000);
}

static void print_run_summary(
    const Solver *solver,
    int case_id,
    const char *filename
) { // 计算开始前把关键参数打印一遍，方便检查输入是否正确
    printf("[Step 4] Final Check Before Launch\n");
    printf("case_id      = %d\n", case_id);
    printf("domain       = [%.6f, %.6f], x0 = %.6f\n",
           solver->grid.xmin, solver->grid.xmax, solver->grid.x0);
    printf("grid         = nx %d, dx %.10f\n", solver->grid.nx, solver->grid.dx);
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
    printf("MacCormack   = %s\n", maccormack_mode_name(solver->maccormack.mode));
    printf("filter       = %s, beta %.6f, sensor %s, eps %.1e\n",
           solver->av.enabled ? "on" : "off",
           solver->av.beta,
           viscosity_sensor_name(solver->av.sensor_type),
           solver->av.sensor_epsilon);
    printf("snapshots    = every %d steps\n", solver->time.output_interval);
    printf("output       = %s\n", filename);
    printf("--------------------------------------------------\n");
}

/*
======================== 6. 主程序 ========================
*/


int main(void) {
    Solver solver = solver_create_default();
    char output_filename[OUTPUT_NAME_LEN];
    char exact_filename[OUTPUT_NAME_LEN + 16];
    char snapshot_filename[OUTPUT_NAME_LEN + 32];
    int case_id = 1;

    print_banner();

    // 先让用户设置问题，再开内存；这样 nx 确定之后再分配数组
    configure_riemann_case(&solver, &case_id);
    configure_domain_and_grid(&solver);
    configure_numerics(&solver);
    ask_output_filename(output_filename, sizeof(output_filename), case_id);
    make_exact_filename(output_filename, exact_filename, sizeof(exact_filename));

    print_run_summary(&solver, case_id, output_filename);

    if (!solver_allocate(&solver)) {
        // 如果网格数太大或者内存不足，就直接停止
        fprintf(stderr, "[Fatal Error] Memory allocation failed.\n");
        solver_destroy(&solver);
        return 1;
    }

    // 构造初始条件 Q^0
    initialize_riemann_problem(&solver);

    if (solver.time.output_interval > 0) {
        // 如果用户要求输出快照，先把 t=0 的初始场也写出来
        make_snapshot_filename(output_filename,
                               solver.time.step_count,
                               snapshot_filename,
                               sizeof(snapshot_filename));
        write_tecplot(&solver, snapshot_filename);

        make_exact_filename(snapshot_filename,
                            exact_filename,
                            sizeof(exact_filename));
        if (!run_exact_solver(&solver, solver.time.t, exact_filename)) {
            solver_destroy(&solver);
            return 1;
        }
    }

    while (solver.time.t < solver.time.t_max &&
           solver.time.step_count < solver.time.step_limit) {
        // 每次循环只推进一个时间步，内部会自动更新 dt
        if (!advance_one_maccormack_step(&solver)) {
            solver_destroy(&solver);
            return 1;
        }

        // 到达指定间隔时输出数值解，同时调用精确解程序输出对应时刻的精确解
        if (solver.time.output_interval > 0 &&
            solver.time.step_count % solver.time.output_interval == 0) {
            make_snapshot_filename(output_filename,
                                   solver.time.step_count,
                                   snapshot_filename,
                                   sizeof(snapshot_filename));
            write_tecplot(&solver, snapshot_filename);

            make_exact_filename(snapshot_filename,
                                exact_filename,
                                sizeof(exact_filename));
            if (!run_exact_solver(&solver, solver.time.t, exact_filename)) {
                solver_destroy(&solver);
                return 1;
            }
        }
    }

    // 输出最终数值解
    write_tecplot(&solver, output_filename);

    // 输出最终时刻对应的精确解，便于 Tecplot 或 Python 后处理对比
    make_exact_filename(output_filename, exact_filename, sizeof(exact_filename));
    if (!run_exact_solver(&solver, solver.time.t, exact_filename)) {
        solver_destroy(&solver);
        return 1;
    }

    // 释放内存，程序结束
    solver_destroy(&solver);
    return 0;
}
