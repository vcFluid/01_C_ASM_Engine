/*
 * ============================================================================
 * 1-D Euler Riemann problem: MacCormack teaching edition
 * ============================================================================
 *
 * 这是一份“可编译的教学注释版”，对应：
 *
 *     ../1-D_Riemann_NM_MacC.c
 *
 * 本文件不追求工业软件式封装，而用于回答四类问题：
 *
 * 1. [Physics] 这一变量对应什么物理量？
 * 2. [Numerics] 这一语句对应离散格式的哪一步？
 * 3. [C syntax] 这一写法在 C 语言中是什么意思？
 * 4. [Risk] 哪些条件下它会失效、失稳或产生未定义行为？
 *
 * 建议阅读方式：
 *
 *     先顺着 main() 阅读总流程；
 *     再回到各函数查看每一步的数学与语法细节。
 *
 * 核心数据流：
 *
 *     initial primitive variables W = [rho, u, p]
 *                         |
 *                         v
 *     conservative variables Q = [rho, rho*u, rho*E]
 *                         |
 *                         v
 *        MacCormack predictor -> Q_bar
 *                         |
 *                         v
 *        MacCormack corrector -> Q_next
 *                         |
 *                         v
 *      Q_next becomes the current Q for the next time step
 *
 * 注意：教学版中的大量注释不会改变 C 语句本身；编译器会忽略注释。
 */

/*
 * [C syntax: #include]
 *
 * #include 是 C preprocessor directive（预处理指令），发生在正式编译之前。
 * 它可以粗略理解为：把指定头文件中公开的声明引入当前 translation unit。
 *
 * “声明”告诉编译器函数或类型长什么样；真正的函数实现通常位于 C runtime library。
 */
#include <stdio.h>
/* stdio.h:
 * FILE, fopen, fclose, fprintf, printf, snprintf, fgets 等标准输入输出接口。
 */
#include <stdlib.h>
/* stdlib.h:
 * calloc, free, system 等通用工具和内存管理接口。
 */
#include <math.h>
/* math.h:
 * sqrt, fabs 等数学函数。使用 GCC/MinGW 链接时通常需要命令行参数 -lm。
 */
#include <string.h>
/* string.h:
 * strcspn, strrchr 等 C 字符串处理函数。
 */

/*
 * [C syntax: object-like macro]
 *
 * #define 在预处理阶段执行文本替换。这里没有创建 C 变量，也不占运行期内存。
 * 使用带语义的名字代替裸数字，可以让数组长度和缓冲区用途更清楚。
 *
 * [Risk]
 * 宏没有类型。若需要带类型、作用域和调试信息的常量，现代 C 中也可考虑：
 *
 *     static const size_t input_line_len = 128;
 *
 * 本作业继续使用宏，以便接触常见 C 项目写法。
 */
#define INPUT_LINE_LEN 128
#define OUTPUT_NAME_LEN 128
#define COMMAND_LINE_LEN 1024

/*
 * Windows 路径中的反斜杠必须写成 "\\"。
 *
 * [C syntax: escape sequence]
 * 在字符串字面量中，单个反斜杠用于引入转义字符，例如：
 *
 *     "\n"  newline
 *     "\t"  horizontal tab
 *     "\\"  一个真正的反斜杠字符
 *
 * 所以下面的 C 字符串在运行时实际是：
 * _Analysical_Solution_Solver\riemann_exact.exe
 */
#define EXACT_SOLVER_EXE "_Analysical_Solution_Solver\\riemann_exact.exe"

/*
 * 这是早期遗留的宏。当前 solver 实际使用 self->gamma，默认值在
 * solver_create_default() 中设为 1.4。
 *
 * [Design]
 * 同一物理参数不应长期同时存在“宏常量”和“对象成员”两个权威来源，否则可能不一致。
 */
#define GAMMA 1.4

/*
 * [C syntax: enum]
 *
 * enum 为一组相关的整数常量提供名字。
 *
 *     VISCOSITY_SENSOR_RHO == 1
 *     VISCOSITY_SENSOR_U   == 2
 *     VISCOSITY_SENSOR_P   == 3
 *
 * 相比直接在程序中散布 1、2、3，枚举值能表达选择的物理含义。
 * C 的 enum 不是 C++ 那种强类型 enum；其值仍可与整数互相转换，因此输入后仍需限制范围。
 */
typedef enum {
    VISCOSITY_SENSOR_RHO = 1,
    VISCOSITY_SENSOR_U = 2,
    VISCOSITY_SENSOR_P = 3
} ViscositySensorType;

/*
 * [C syntax: forward declaration + typedef]
 *
 * 先声明存在一个名为 struct Riemann_1D_MacC_solver 的结构体类型，但暂时不展开成员。
 * 同时为它建立短别名 solver。
 *
 * 后面可以写：
 *
 *     solver *self;
 *
 * 而不必每次写：
 *
 *     struct Riemann_1D_MacC_solver *self;
 *
 * 为什么需要提前声明？
 * 因为结构体内部保存了“参数中包含 solver *”的函数指针。编译器在解析这些函数指针
 * 时必须已经知道 solver 是一个类型名称，但不需要提前知道结构体的完整大小。
 *
 * 只声明、不展开的类型称为 incomplete type。可以声明指向它的指针，因为任意对象指针
 * 的大小在当前平台上是已知的；但在类型完整定义之前，不能创建 solver 对象或访问成员。
 */
typedef struct Riemann_1D_MacC_solver solver;

/*
    1-D Euler equations:
        原始量 W = [rho, u, p] (一维流动，所以速度变量只有一维)
        守恒量 Q = [rho, rho*u, rho*E] 
        矢通量 F = [rho*u, rho*u*u + p, u*(rho*E + p)]

    MacCormack 格式使用两个步骤:

        预测步 Predictor: U_bar_i = U_i - dt/dx * (F_{i+1} - F_i)
        矫正步 Corrector: U_i^{n+1} = 0.5 * (U_i + U_bar_i
                              - dt/dx * (F_bar_i - F_bar_{i-1}))
*/

/*
 * [Numerics: main state]
 *
 * 时间推进的主变量选择守恒量：
 *
 *     Q = [rho, rho*u, rho*E]
 *
 * 而不是直接推进：
 *
 *     W = [rho, u, p]
 *
 * 这是因为 Euler equations 的 conservation form 正是：
 *
 *     partial Q / partial t + partial F(Q) / partial x = 0
 *
 * rho、u、p 仍会长期储存，但它们是由当前 Q 反算得到的同步辅助状态。
 */

/*
    类似于project 1_1-D_LinearAdvection_Solver.c，这里也采用OOP的思想，把对象和方法打包在一起，方便后续优化和调试，这里就不赘述了
*/

/*
    由于这个问题比较复杂，我们用OOP的三步法拆解：
    ① 定义对象和方法
    ② 从main函数中提取主干操作并解耦出来封装成局部私有化函数 static function
    ③ 执行main函数
*/
/*
 * [C syntax: struct]
 *
 * struct 把相关数据成员组合成一个对象。C 不提供 C++ class、成员函数、构造函数和
 * private/public 访问控制，但可以用“结构体 + 接收 self 指针的函数”组织相似逻辑。
 *
 * [Memory]
 * 结构体本身只保存标量和指针。q1、rho 等数组的数据不嵌在结构体内部，而是在运行时
 * 由 calloc 分配到 heap；结构体成员只保存这些数组的首地址。
 */
struct Riemann_1D_MacC_solver {
    /*
     * [C syntax: int]
     * int 用于离散计数和开关。它适合网格点数量、步数和选项，不适合连续物理量。
     */
    int nx;                 // 网格点总数，不是网格单元数；dx = L / (nx - 1)
    int step_count;         // 已经完成的时间步数
    int output_interval;    // 每隔多少步输出快照；0 表示只输出最终结果

    /*
     * [Physics / Numerics]
     * double 是双精度浮点数，通常为 IEEE 754 binary64。
     * 它适合连续物理量，但只能近似表示多数十进制小数。
     */
    double xmin;    // 计算域左端
    double xmax;    // 计算域右端
    double x0;      // t=0 时初始间断的位置
    double dx;      // 均匀网格间距：(xmax - xmin) / (nx - 1)
    double dt;      // 当前时间步长；每一步根据 CFL 重新计算
    double t;       // 当前物理时间
    double t_max;   // 目标终止时间
    double cfl;     // 目标 Courant number，用于约束显式时间步长

    double gamma;   // 理想气体比热比 gamma = cp/cv，Sod baseline 使用 1.4

    /*
     * Riemann initial condition:
     *
     *     W(x,0) = W_L, x < x0
     *              W_R, x >= x0
     */
    double left_rho;
    double left_u;
    double left_p;
    double right_rho;
    double right_u;
    double right_p;

    /*
     * [Numerics: artificial viscosity]
     *
     * artificial_viscosity_k 是当前代码和报告中讨论的经验参数 beta。
     * use_artificial_viscosity 采用 C 常见约定：0 为 false，非 0 为 true。
     */
    double artificial_viscosity_k;
    int use_artificial_viscosity;
    ViscositySensorType viscosity_sensor_type; // rho、u 或 p 作为 switch sensor
    double rho_floor;  // 预留的密度下限；当前教学算法没有主动修补流场
    double p_floor;    // 预留的压力下限；用于识别非物理解风险

    /*
     * [C syntax: pointer]
     *
     * double *q1 表示“指向 double 的指针”。分配后，它保存一段连续 double 数组的
     * 首地址，因此 q1[i] 等价于 *(q1 + i)。
     *
     * [Physics]
     * q3 = rho*E，其中 E 是单位质量总能：
     *
     *     E = e + u^2/2
     *     rho*E = p/(gamma-1) + rho*u^2/2
     */
    double *q1; // rho
    double *q2; // rho*u
    double *q3; // rho*E

    /*
     * Predictor 得到的临时状态 Q_bar。
     * bar 数组必须与当前 Q 分开，因为 corrector 同时需要 Q^n 和 Q_bar。
     */
    double *q1_bar;
    double *q2_bar;
    double *q3_bar;

    /*
     * Corrector 得到的新时间层 Q^(n+1)。
     * 先写入 next 数组，再整体复制回当前 Q，避免循环内部覆盖仍待使用的 Q^n。
     */
    double *q1_next;
    double *q2_next;
    double *q3_next;

    /*
     * “矢通量”应写作 flux vector，而不是“失通量”。
     *
     * F(Q) = [rho*u, rho*u^2+p, u*(rho*E+p)]
     *
     * f*_bar 是由预测状态 Q_bar 计算得到的通量。
     */
    double *f1;
    double *f2;
    double *f3;
    double *f1_bar;
    double *f2_bar;
    double *f3_bar;

    /*
     * 原始量与派生量数组。
     * 它们不是独立推进的第二套解，而应始终由当前守恒量 Q 同步更新。
     */
    double *rho;
    double *u;
    double *p;
    double *E;              // 单位质量总能 E = q3/rho
    double *a;              // 局部声速 sqrt(gamma*p/rho)
    double *visc_sensor;    // 每个网格点的人工粘性系数 epsilon_i

    /*
     * [C syntax: function pointer]
     *
     *     int (*allocate)(solver *self);
     *
     * 由内向外读：
     *
     * 1. allocate 前面的 * 表示 allocate 是一个指针；
     * 2. (*allocate)(...) 表示它指向函数，而不是普通数据；
     * 3. 该函数接收 solver *；
     * 4. 该函数返回 int。
     *
     * bind_methods() 后会执行：
     *
     *     self->allocate = solver_allocate;
     *
     * 于是：
     *
     *     my_solver.allocate(&my_solver);
     *
     * 最终调用 solver_allocate(&my_solver)。
     *
     * [Design]
     * 这是一种 C 风格的 method dispatch，用于学习 OOP-like organization。
     * 当前只有一种 solver 实现，因此直接调用普通函数会更简单；函数指针主要体现接口思想，
     * 并为未来替换算法实现预留位置。
     */
    int  (*allocate)(solver *self);
    void (*bind_methods)(solver *self);
    void (*init_sod)(solver *self);
    void (*update_primitives)(solver *self);
    void (*compute_flux)(
        solver *self,
        /*
         * [C syntax: pointer to const]
         *
         * const double *q1 表示函数不能通过 q1 修改其指向的 double。
         * 指针 q1 自身是形参副本，可以在函数内部改指向，但这里没有这样做。
         */
        const double *q1,
        const double *q2,
        const double *q3,
        double *f1,
        double *f2,
        double *f3
    );
    double (*compute_dt)(solver *self);
    void (*apply_boundary)(
        solver *self,
        double *q1,
        double *q2,
        double *q3
    );
    /*
    [Risk: intentionally disabled physical-state repair]

    声速 a = sqrt(gamma*p/rho) 要求 rho > 0 且 p > 0。
    激波附近的非物理振荡可能破坏该条件。

    本教学作业希望直接观察失稳，因此没有通过 floor 强行修补守恒量。代价是某些参数组合
    会产生 NaN，并应被实验脚本标记为 unstable。

    void (*enforce_physical_state)(
        solver *self,
        double *q1,
        double *q2,
        double *q3
    );
    */
    void (*apply_artificial_viscosity)(solver *self);
    void (*step_maccormack)(solver *self);
    void (*write_tecplot)(solver *self, const char *filename);
    void (*destroy)(solver *self); // 对每个成功分配的 heap array 调用 free
};

/*
======================== ↓开始写方法↓ =========================
*/

/*
 * [C syntax: static on a file-scope function]
 *
 * 这里的 static 表示 internal linkage：
 * 函数名只在当前 translation unit 内可见，不会作为可供其他 .c 文件链接的外部符号。
 *
 * 必须纠正一个常见混淆：
 *
 * - file-scope function 的 static 主要控制 linkage/可见性；
 * - local variable 的 static 主要改变 storage duration；
 * - “函数生命周期只局限于程序运行时”不是这里最有区分度的解释，因为普通函数也是如此。
 */

/*
 * [Physics: conservative -> pressure]
 *
 * q1 = rho
 * q2 = rho*u
 * q3 = rho*E
 *
 * kinetic energy per volume:
 *
 *     rho*u^2/2 = (rho*u)^2 / (2*rho) = q2^2/(2*q1)
 *
 * ideal-gas internal energy per volume:
 *
 *     rho*e = p/(gamma-1)
 *
 * 因此：
 *
 *     p = (gamma-1) * [q3 - q2^2/(2*q1)]
 *
 * [C syntax: ->]
 * self 是指向 solver 的指针，self->gamma 等价于 (*self).gamma。
 *
 * [Risk]
 * q1 必须大于零；若 q1=0，这里的除法会产生 Inf/NaN。当前教学程序故意不隐藏失稳。
 */
static double pressure_from_q(
    solver *self,
    double q1,
    double q2,
    double q3
) {
    double kinetic_energy_density = 0.5 * q2 * q2 / q1;
    return (self->gamma - 1.0) * (q3 - kinetic_energy_density);
}

/*
 * [Physics: primitive -> conservative energy]
 *
 * 输入 rho、u、p，返回单位体积总能量 rho*E：
 *
 *     rho*E = p/(gamma-1) + rho*u^2/2
 *
 * 它用于把 Riemann initial primitive states 转换成时间推进需要的 Q。
 */
static double total_energy_density(
    solver *self,
    double rho,
    double u,
    double p
) {
    return p / (self->gamma - 1.0) + 0.5 * rho * u * u;
}

/*
 * [Memory: calloc]
 *
 * calloc(n, sizeof(double)) 请求一段能容纳 n 个 double 的连续 heap memory，并把每个
 * byte 初始化为 0。成功时返回首地址；失败时返回 NULL。
 *
 * size_t 是专门表示对象大小和数组长度的无符号整数类型，sizeof 的结果也是 size_t。
 *
 * [C syntax: cast]
 * C 中 void * 可以隐式转换为其他 object pointer，因此 `(double *)` 并非必需。
 * 保留显式 cast 是当前原代码风格，但在纯 C 项目中常写为：
 *
 *     return calloc(n, sizeof(double));
 *
 * [Memory ownership]
 * 返回的数组由 solver 拥有，最终必须在 solver_destroy() 中 free。
 */
static double *alloc_double_array(size_t n) {
    return (double *)calloc(n, sizeof(double));
}

/*
 * [C syntax: const on returned data]
 *
 * 返回类型 const double * 表示调用者通过该返回值只读 sensor 数组。
 * 函数没有复制数组，只返回 self->rho、self->u 或 self->p 中某一个已有首地址。
 *
 * [C syntax: switch]
 * switch 根据一个整数/枚举表达式跳转到匹配的 case。每个 return 会立即结束函数，
 * 因此这里不需要 break。
 */
static const double *viscosity_sensor_values(const solver *self) {
    switch (self->viscosity_sensor_type) {
        case VISCOSITY_SENSOR_RHO:
            return self->rho;
        case VISCOSITY_SENSOR_U:
            return self->u;
        case VISCOSITY_SENSOR_P:
            return self->p;
        default:
            return self->rho;
    }
}

/*
 * 把枚举值转换为用于日志输出的 C string。
 *
 * [C syntax: string literal]
 * "rho" 的类型可视为 char array，表达式中通常退化为指向首字符的指针。
 * 字符串字面量具有 static storage duration，不需要调用者 free。
 *
 * 返回 const char * 可阻止调用者试图修改字符串字面量。
 */
static const char *viscosity_sensor_name(ViscositySensorType type) {
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

/*
 * [Memory: malloc vs calloc]
 *
 * 常见写法：
 *
 *     malloc(n * sizeof(double))
 *     calloc(n, sizeof(double))
 *
 * 关键区别首先是初始化：
 *
 * - malloc 得到的 bytes 未初始化；
 * - calloc 把分配区域清零。
 *
 * 很多实现还会在 calloc 内检查 n*size 是否溢出，但 C 标准层面应关注其可观察语义：
 * 成功时提供足够空间并清零，失败时返回 NULL。无论使用哪种函数，都必须检查返回值。
 *
 * [Risk]
 * “heap 上的空间是永恒的”是错误说法。动态内存会一直存在到 free，或进程结束时由
 * operating system 回收。如果反复分配却不 free，就会形成 memory leak。
 */


/*
 * ============================================================================
 * Block 2: memory allocation and Q <-> W state conversion
 * ============================================================================
 */

/*
 * [Design: allocation as one solver operation]
 *
 * 该函数一次性为所有长度为 nx 的数组分配内存。
 *
 * 返回值约定：
 *
 *     1 -> 所有数组均分配成功
 *     0 -> 至少一个数组分配失败
 *
 * 这是一种 C 常见 error-code interface。调用者不能只调用 allocate 而忽略返回值。
 *
 * [C syntax: explicit conversion]
 * self->nx 是 int，数组长度接口使用 size_t，因此显式转换：
 *
 *     size_t n = (size_t)self->nx;
 *
 * 当前 nx 在交互阶段已经被限制为正数。若把负 int 直接转换为 size_t，会变成很大的
 * 无符号数，因此“先验证，再转换”很重要。
 *
 * [Memory estimate]
 * 当前共分配 21 个 double arrays。粗略内存为：
 *
 *     21 * nx * sizeof(double)
 *
 * nx=501 时约 84 KiB；nx=200000 时约 32 MiB。
 */
int solver_allocate(solver *self) {
    size_t n = (size_t)self->nx;

    /* 当前时间层的三个守恒量。每次 calloc 都是一次独立 heap allocation。 */
    self->q1 = alloc_double_array(n);
    self->q2 = alloc_double_array(n);
    self->q3 = alloc_double_array(n);

    /* Predictor 临时状态。 */
    self->q1_bar = alloc_double_array(n);
    self->q2_bar = alloc_double_array(n);
    self->q3_bar = alloc_double_array(n);

    /* Corrector 产生的新时间层。 */
    self->q1_next = alloc_double_array(n);
    self->q2_next = alloc_double_array(n);
    self->q3_next = alloc_double_array(n);

    /* 当前状态的 Euler flux。 */
    self->f1 = alloc_double_array(n);
    self->f2 = alloc_double_array(n);
    self->f3 = alloc_double_array(n);

    /* Predictor 状态对应的 Euler flux。 */
    self->f1_bar = alloc_double_array(n);
    self->f2_bar = alloc_double_array(n);
    self->f3_bar = alloc_double_array(n);

    /* 原始量、派生量和人工粘性 switch。 */
    self->rho = alloc_double_array(n);
    self->u = alloc_double_array(n);
    self->p = alloc_double_array(n);
    self->E = alloc_double_array(n);
    self->a = alloc_double_array(n);
    self->visc_sensor = alloc_double_array(n);

    /*
     * [C syntax: logical operators and null pointer test]
     *
     * !pointer 在 pointer==NULL 时为 true。
     * || 是 short-circuit logical OR：左侧为 true 后，右侧不再计算。
     *
     * 这里只要任意一个分配失败，就报告整体失败。
     *
     * [Memory]
     * 此时已经成功分配的数组暂时仍由 self 保存。main 随后调用 destroy，因此不会泄漏。
     */
    if (!self->q1 || !self->q2 || !self->q3 ||
        !self->q1_bar || !self->q2_bar || !self->q3_bar ||
        !self->q1_next || !self->q2_next || !self->q3_next ||
        !self->f1 || !self->f2 || !self->f3 ||
        !self->f1_bar || !self->f2_bar || !self->f3_bar ||
        !self->rho || !self->u || !self->p || !self->E ||
        !self->a || !self->visc_sensor) {
        return 0;
    }

    return 1;
}

/*
 * [Physics: conservative variables -> primitive variables]
 *
 * 对每个网格点执行：
 *
 *     rho = q1
 *     u   = q2/q1
 *     p   = (gamma-1) * (q3 - q2^2/(2*q1))
 *     E   = q3/q1
 *     a   = sqrt(gamma*p/rho)
 *
 * [C syntax: for loop scope]
 * `int i` 只在 for loop 及其 body 中有效。循环条件 i < nx，因此最后访问的是 nx-1。
 *
 * [Design]
 * 该函数是 Q 与 W 之间的单向同步点。不要独立修改 rho/u/p 后假设 Q 会自动变化。
 *
 * [Risk]
 * 当 q1<=0 或 p<=0 时，状态已经失去理想气体 Euler 方程的物理意义。
 * 代码把声速置零只是避免立即对负数开平方；它没有修复 q1、u、p，也不能恢复稳定性。
 */
void solver_update_primitives(solver *self) {
    for (int i = 0; i < self->nx; i++) {
        self->rho[i] = self->q1[i];
        self->u[i] = self->q2[i] / self->q1[i];
        self->p[i] = pressure_from_q(self, self->q1[i], self->q2[i], self->q3[i]);
        self->E[i] = self->q3[i] / self->q1[i];

        if (self->p[i] > 0.0 && self->rho[i] > 0.0) {
            self->a[i] = sqrt(self->gamma * self->p[i] / self->rho[i]);
        } else {
            /*
             * 这是 diagnostic fallback，不是 positivity-preserving scheme。
             * 后续 flux 中仍可能因 rho<=0 而产生 Inf/NaN。
             */
            self->a[i] = 0.0;
        }
    }
}

/*
 * ============================================================================
 * Block 3: Euler flux, CFL, boundary condition and artificial viscosity
 * ============================================================================
 */

/*
 * [Physics: Euler flux]
 *
 * 一维 Euler conservation law：
 *
 *     partial Q / partial t + partial F(Q) / partial x = 0
 *
 *     Q = [rho, rho*u, rho*E]
 *     F = [rho*u,
 *          rho*u^2 + p,
 *          u*(rho*E+p)]
 *
 * 三个通量分别代表：
 *
 * 1. mass flux；
 * 2. momentum flux；
 * 3. total-energy flux。
 *
 * [C design]
 * 同一个函数既可计算 Q 的 flux，也可计算 Q_bar 的 flux。输入数组由 const pointer
 * 提供，输出写入调用者指定的 f arrays。这避免复制整套状态。
 *
 * self 参数主要提供 nx 和 gamma；真正参与计算的状态由 q1/q2/q3 参数决定。
 */
void solver_compute_flux(
    solver *self,
    const double *q1,
    const double *q2,
    const double *q3,
    double *f1,
    double *f2,
    double *f3
) {
    for (int i = 0; i < self->nx; i++) {
        double rho = q1[i];
        double u = q2[i] / rho;
        double p = pressure_from_q(self, q1[i], q2[i], q3[i]);

        /* rho*u 就是 q2，所以质量通量可以直接赋值。 */
        f1[i] = q2[i];
        /* q2*u = rho*u^2。 */
        f2[i] = q2[i] * u + p;
        /* q3+p 是 total enthalpy per volume: rho*E+p。 */
        f3[i] = (q3[i] + p) * u;
    }
}

/*
 * [Numerics: CFL time step]
 *
 * 一维 Euler 系统三个 characteristic speeds 为：
 *
 *     u-a, u, u+a
 *
 * 最大信号传播速度的绝对值可写成：
 *
 *     max(|u|+a)
 *
 * 显式格式使用：
 *
 *     dt = CFL * dx / max_i(|u_i| + a_i)
 *
 * 这不是“Von Neumann 分析本身”，而是由双曲 PDE 信息传播速度得到的 CFL 限制。
 *
 * [Risk]
 * CFL<1 不自动保证非线性激波问题稳定；格式、人工粘性、边界和 positivity 同样重要。
 */
double solver_compute_dt(solver *self) {
    double max_speed = 0.0;

    /* CFL 使用 u 和 a，因此先由最新 Q 同步 primitive arrays。 */
    self->update_primitives(self);

    for (int i = 0; i < self->nx; i++) {
        double speed = fabs(self->u[i]) + self->a[i];
        if (speed > max_speed) {
            max_speed = speed;
        }
    }

    if (max_speed <= 0.0) {
        /*
         * 静止且声速也为零通常意味着无效/退化状态。返回极小正数可避免除零，
         * 但这不是严格的物理解法。正常 Riemann gas state 不应进入该分支。
         */
        return 1.0e-8;
    }

    return self->cfl * self->dx / max_speed;
}

/*
 * [Numerics: transmissive / zero-gradient boundary]
 *
 * 端点直接复制相邻内部点：
 *
 *     Q_0     = Q_1
 *     Q_{N-1} = Q_{N-2}
 *
 * 这相当于离散零法向梯度，常作为 shock-tube 中波尚未到达边界时的简单 outflow boundary。
 *
 * [C syntax: arrays are modified in place]
 * q1/q2/q3 是指针，赋值会修改调用者的数组。
 *
 * [Risk]
 * 这不是一般意义上无反射边界。波到达边界后可能产生数值反射；报告应保证分析时间内
 * 主要波系仍处于计算域内部。
 */
void solver_apply_boundary(
    solver *self, 
    double *q1, 
    double *q2, 
    double *q3
) {
    int last = self->nx - 1;

    /* 左边界复制第一个内部点。nx 至少为 5，因此索引 1 合法。 */
    q1[0] = q1[1];
    q2[0] = q2[1];
    q3[0] = q3[1];

    q1[last] = q1[last - 1];
    q2[last] = q2[last - 1];
    q3[last] = q3[last - 1];
}

    /*
    [Teaching note: disabled positivity repair]

    下面函数若启用，会把 rho 和 p 强制限制为正数。它可阻止部分 NaN，但同时改变
    conservative state 和实验结论。对于本作业，“β=0 发生失稳”是需要观察的现象，
    因而暂时不启用。

    更高级的 positivity-preserving method 应从数值通量或 time-step restriction
    保证 admissible state，而不是简单截断结果。

    void solver_enforce_physical_state(
        solver *self, 
        double *q1, 
        double *q2, 
        double *q3
    ) {
        for (int i = 0; i < self->nx; i++) {
            if (q1[i] < self->rho_floor) {
                q1[i] = self->rho_floor;
            }

            double u = q2[i] / q1[i];
            double p = pressure_from_q(self, q1[i], q2[i], q3[i]);

            if (p < self->p_floor) {
                q3[i] = self->p_floor / (self->gamma - 1.0) + 0.5 * q1[i] * u * u;
            }
        }
    }
    */

/*
 * [Numerics: nonlinear artificial viscosity]
 *
 * 当前实现先根据 sensor phi（rho/u/p）构造局部系数：
 *
 *                 |phi_{i+1} - 2 phi_i + phi_{i-1}|
 * epsilon_i = beta ---------------------------------
 *                 |phi_{i+1}|+2|phi_i|+|phi_{i-1}|+delta
 *
 * numerator 是离散二阶差分：光滑线性区域中接近 0，在间断和强曲率区域增大。
 * denominator 做无量纲归一化；delta=1e-12 防止全零 sensor 时除零。
 *
 * 然后对 corrector state 增加：
 *
 *     Q_i^(n+1) <- Q_i^(n+1)
 *                  + epsilon_i (Q_{i+1}^n - 2Q_i^n + Q_{i-1}^n)
 *
 * 二阶差分具有 smoothing/diffusion 效果，可抑制 centered MacCormack 在间断附近的
 * dispersive oscillation。
 *
 * [Important]
 * 这里的 epsilon 没显式包含 dt/dx^2，因此 beta 是与当前离散写法绑定的经验参数，
 * 不能不加说明地当成连续 PDE 中具有物理单位的 viscosity coefficient。
 */
void solver_apply_artificial_viscosity(solver *self) {
    /*
     * [C syntax: ! and ||]
     * 关闭开关或 beta<=0 时直接 return，后续数组完全不修改。
     * beta=0 因此严格对应“该人工粘性步骤无贡献”。
     */
    if (!self->use_artificial_viscosity || self->artificial_viscosity_k <= 0.0) {
        return;
    }

    /* sensor 可取 primitive variable，因此确保 rho/u/p 与当前 Q 一致。 */
    self->update_primitives(self);
    const double *sensor_values = viscosity_sensor_values(self);

    /*
     * 从 i=1 到 i=nx-2，因为公式访问 i-1 和 i+1。
     * 写成 i < nx-1 可保证最大 i 为 nx-2。
     */
    for (int i = 1; i < self->nx - 1; i++) {
        double denominator = fabs(sensor_values[i + 1])
                           + 2.0 * fabs(sensor_values[i])
                           + fabs(sensor_values[i - 1])
                           + 1.0e-12;
        self->visc_sensor[i] = self->artificial_viscosity_k
            * fabs(sensor_values[i + 1]
                 - 2.0 * sensor_values[i]
                 + sensor_values[i - 1])
            / denominator;
    }

    self->visc_sensor[0] = 0.0;
    self->visc_sensor[self->nx - 1] = 0.0;

    /*
     * 对三个守恒量使用同一个 epsilon，保持 artificial viscosity 以 conservative
     * variables 为对象。注意右侧使用 Q^n，而被修改的是 q_next。
     */
    for (int i = 1; i < self->nx - 1; i++) {
        double eps = self->visc_sensor[i];

        self->q1_next[i] += eps * (self->q1[i + 1] - 2.0 * self->q1[i] + self->q1[i - 1]);
        self->q2_next[i] += eps * (self->q2[i + 1] - 2.0 * self->q2[i] + self->q2[i - 1]);
        self->q3_next[i] += eps * (self->q3[i + 1] - 2.0 * self->q3[i] + self->q3[i - 1]);
    }
}

/*
 * [Physics: piecewise-constant Riemann initial condition]
 *
 * 函数名保留 init_sod，但实际使用 self 中的 left/right states，所以七个预设算例
 * 都通过同一函数初始化。更准确的名字应是 solver_init_riemann。
 *
 * [C syntax: local variables]
 * 每次循环先把局部 rho/u/p 设为右状态；若 x<x0，再覆盖为左状态。
 * x==x0 被分配到右状态。这只影响恰好位于间断上的一个网格点。
 */
void solver_init_sod(solver *self) {
    for (int i = 0; i < self->nx; i++) {
        double x = self->xmin + i * self->dx;

        double rho = self->right_rho;
        double u = self->right_u;
        double p = self->right_p;

        if (x < self->x0) {
            rho = self->left_rho;
            u = self->left_u;
            p = self->left_p;
        }

        /* primitive W -> conservative Q */
        self->q1[i] = rho;
        self->q2[i] = rho * u;
        self->q3[i] = total_energy_density(self, rho, u, p);
    }

    /*
     * 初始化后立即施加边界，再同步 primitive arrays。
     * 对 piecewise-constant shock tube，端点复制不会改变左右常状态。
     */
    self->apply_boundary(self, self->q1, self->q2, self->q3);
        //self->enforce_physical_state(self, self->q1, self->q2, self->q3);
    self->update_primitives(self);
}

/*
 * ============================================================================
 * Block 4: one complete MacCormack predictor-corrector time step
 * ============================================================================
 *
 * 对 conservation law：
 *
 *     Q_t + F(Q)_x = 0
 *
 * 当前实现使用：
 *
 * Predictor, forward flux difference:
 *
 *     Qbar_i = Q_i^n
 *              - dt/dx * (F_{i+1}^n - F_i^n)
 *
 * Corrector, backward flux difference:
 *
 *     Q_i^(n+1) = 1/2 [
 *         Q_i^n + Qbar_i
 *         - dt/dx * (Fbar_i - Fbar_{i-1})
 *     ]
 *
 * forward/backward 两种偏置在一个完整时间步内配对，得到时空二阶的 MacCormack scheme
 * （对足够光滑的解成立）。在 shock/contact discontinuity 处，经典阶数分析不再直接适用，
 * 且 centered nature 会引起 Gibbs-like dispersive oscillations。
 *
 * [Data dependency]
 *
 *     Q^n
 *      | compute F^n
 *      v
 *     Qbar
 *      | compute Fbar
 *      v
 *     Qnext
 *      | artificial viscosity + boundary
 *      v
 *     copy Qnext -> Q
 */
void solver_step_maccormack(solver *self) {
    double lambda = 0.0;

    /*
     * dt 每一步重新计算，因为 |u|+a 随流场演化。
     * compute_dt 内部也会更新 primitive arrays。
     */
    self->dt = self->compute_dt(self);

    /*
     * [Numerics: final-step clipping]
     * 如果正常 CFL time step 会越过 t_max，就缩短最后一步，使最终时间精确停在 t_max。
     * 没有这段时，最终输出时间通常会略大于目标时间，精确解比较也会错位。
     */
    if (self->t + self->dt > self->t_max) {
        self->dt = self->t_max - self->t;
    }

    /* lambda 是离散公式中反复出现的 dt/dx。它的量纲为 time/length。 */
    lambda = self->dt / self->dx;

    /* Stage 1: F^n = F(Q^n). */
    self->compute_flux(self, self->q1, self->q2, self->q3,
                       self->f1, self->f2, self->f3);

    /*
     * Stage 2: predictor.
     *
     * 只更新内部点 1..nx-2，因为 forward difference 会读取 i+1。
     * q_bar[0] 和 q_bar[nx-1] 随后由 boundary condition 填充。
     *
     * 三条语句分别离散 mass、momentum、energy conservation equations。
     */
    for (int i = 1; i < self->nx - 1; i++) {
        self->q1_bar[i] = self->q1[i] - lambda * (self->f1[i + 1] - self->f1[i]);
        self->q2_bar[i] = self->q2[i] - lambda * (self->f2[i + 1] - self->f2[i]);
        self->q3_bar[i] = self->q3[i] - lambda * (self->f3[i + 1] - self->f3[i]);
    }

    /*
     * Predictor 产生的边界点没有通过上述 loop 计算，必须先填充，之后才能安全计算
     * 整个 Qbar 的 flux，特别是 corrector 需要 fbar[i-1]。
     */
    self->apply_boundary(self, self->q1_bar, self->q2_bar, self->q3_bar);
        //self->enforce_physical_state(self, self->q1_bar, self->q2_bar, self->q3_bar);

    /* Stage 3: Fbar = F(Qbar). */
    self->compute_flux(self, self->q1_bar, self->q2_bar, self->q3_bar,
                       self->f1_bar, self->f2_bar, self->f3_bar);

    /*
     * Stage 4: corrector.
     *
     * backward difference 使用 fbar[i]-fbar[i-1]，与 predictor 的 forward difference
     * 配对。外层 0.5 同时完成：
     *
     * 1. 对 Q^n 与 Qbar 取平均；
     * 2. 对两个方向的空间离散误差做对称组合。
     *
     * q_next 与 q 分离，保证计算任何 i 时，右侧 Q^n 都没有被提前覆盖。
     */
    for (int i = 1; i < self->nx - 1; i++) {
        self->q1_next[i] = 0.5 * (self->q1[i] + self->q1_bar[i]
            - lambda * (self->f1_bar[i] - self->f1_bar[i - 1]));
        self->q2_next[i] = 0.5 * (self->q2[i] + self->q2_bar[i]
            - lambda * (self->f2_bar[i] - self->f2_bar[i - 1]));
        self->q3_next[i] = 0.5 * (self->q3[i] + self->q3_bar[i]
            - lambda * (self->f3_bar[i] - self->f3_bar[i - 1]));
    }

    /*
     * Stage 5: stabilizer.
     *
     * 人工粘性是在完成 MacCormack corrector 后加到 Qnext 上的 operator-like correction。
     * 它不是原始 predictor/corrector 公式的一部分。
     */
    self->apply_artificial_viscosity(self);

    /* 填充新时间层的两个边界点。 */
    self->apply_boundary(self, self->q1_next, self->q2_next, self->q3_next);
        //self->enforce_physical_state(self, self->q1_next, self->q2_next, self->q3_next);

    /*
     * Stage 6: accept the new time level.
     *
     * [C syntax: memcpy]
     * memcpy(destination, source, byte_count) 按 byte 复制，不理解 double 或物理量。
     *
     *     sizeof(double) * nx
     *
     * 是一个完整数组的 byte 数。
     *
     * 这里 source/destination 是不同 allocation，不重叠，所以 memcpy 合法。
     * 若区域可能重叠，应使用 memmove。
     *
     * [Numerics]
     * memcpy 不是数值积分公式；它只是把已经算出的 Q^(n+1) 设为下一轮的当前 Q。
     */
    memcpy(self->q1, self->q1_next, sizeof(double) * (size_t)self->nx);
    memcpy(self->q2, self->q2_next, sizeof(double) * (size_t)self->nx);
    memcpy(self->q3, self->q3_next, sizeof(double) * (size_t)self->nx);

    self->t += self->dt;  // floating-point addition，最终一步已通过 clipping 对齐 t_max
    self->step_count++;   // ++ 等价于 step_count = step_count + 1
}

/*
 * ============================================================================
 * Block 5A: Tecplot ASCII output and solver resource cleanup
 * ============================================================================
 */

/*
 * [I/O: Tecplot ASCII]
 *
 * 输出格式包含：
 *
 *     TITLE
 *     VARIABLES
 *     ZONE
 *     one row per grid point
 *
 * F=POINT 表示每行依次给出一个点的全部变量。
 *
 * [C syntax: FILE *]
 * FILE 是 C standard library 管理文件流的 opaque type。调用者不访问其内部成员，
 * 只通过 fopen/fprintf/fclose 操作。
 *
 * fopen(filename, "w"):
 *
 * - 文件不存在：创建；
 * - 文件存在：截断为零长度后重写；
 * - 失败：返回 NULL。
 */
void solver_write_tecplot(solver *self, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("[Error] Cannot open output file: %s\n", filename);
        return;
    }

    /* 输出的是当前 Q 对应的 W，因此写文件前强制同步一次。 */
    self->update_primitives(self);

    /*
     * [C syntax: format string]
     *
     * %s      C string
     * %d      int
     * %.8f    double，fixed-point，保留 8 位小数
     * %.10f   double，保留 10 位小数
     *
     * fprintf 的第一个参数指定写入哪个 file stream；printf 默认写 stdout。
     */
    fprintf(fp, "TITLE = \"1D Euler MacCormack Result\"\n");
    fprintf(fp, "VARIABLES = \"x\", \"rho\", \"u\", \"p\", \"E\", \"q1\", \"q2\", \"q3\"\n");
    fprintf(fp, "ZONE T=\"t=%.8f\", I=%d, F=POINT\n", self->t, self->nx);

    for (int i = 0; i < self->nx; i++) {
        double x = self->xmin + i * self->dx;
        /*
         * 同时输出 primitive 和 conservative variables，便于教学检查：
         *
         *     q1 ?= rho
         *     q2 ?= rho*u
         *     q3 ?= rho*E
         */
        fprintf(fp, "%.10f %.10f %.10f %.10f %.10f %.10f %.10f %.10f\n",
                x,
                self->rho[i],
                self->u[i],
                self->p[i],
                self->E[i],
                self->q1[i],
                self->q2[i],
                self->q3[i]);
    }

    /*
     * fclose 刷新 userspace buffer 并释放 FILE resource。
     * 忘记 fclose 可能导致尾部数据尚未写入磁盘，也会泄漏 file handle。
     */
    fclose(fp);
}

/*
 * [Memory: destructor-like cleanup]
 *
 * free(pointer) 释放对应 heap allocation。free(NULL) 是安全 no-op，因此 allocate
 * 中途失败后也可以直接调用 destroy。
 *
 * 每次成功 calloc 必须恰好对应一次 free：
 *
 * - 少 free -> memory leak；
 * - 重复 free -> undefined behavior；
 * - free 后继续解引用 -> use-after-free。
 */
void solver_destroy(solver *self) {
    free(self->q1);
    free(self->q2);
    free(self->q3);
    free(self->q1_bar);
    free(self->q2_bar);
    free(self->q3_bar);
    free(self->q1_next);
    free(self->q2_next);
    free(self->q3_next);
    free(self->f1);
    free(self->f2);
    free(self->f3);
    free(self->f1_bar);
    free(self->f2_bar);
    free(self->f3_bar);
    free(self->rho);
    free(self->u);
    free(self->p);
    free(self->E);
    free(self->a);
    free(self->visc_sensor);

    /*
     * [C syntax: sizeof(*self)]
     * sizeof 不会实际解引用 self，它在编译时取得 solver 对象的大小。
     *
     * 清零结构体可把已释放指针变成 NULL，并清除旧参数，降低 accidental reuse 风险。
     * 但调用 destroy 后，所有 method pointers 也会归零，因此该对象不能继续调用方法，
     * 除非重新初始化并 bind。
     */
    memset(self, 0, sizeof(*self));
}

/*
 * [Design: bind concrete functions to interface slots]
 *
 * 函数名在表达式中通常自动转换为 function pointer，因此不必写 &solver_allocate。
 *
 * 绑定前：
 *
 *     self->allocate == NULL
 *
 * 绑定后：
 *
 *     self->allocate(&object)
 *
 * 会间接调用 solver_allocate(&object)。
 *
 * [Risk]
 * 函数指针类型必须与实际函数签名兼容；错误转换后调用会导致 undefined behavior。
 * 当前编译器会检查这些直接赋值。
 */
void solver_bind_methods(solver *self) {
    self->allocate = solver_allocate;
    self->bind_methods = solver_bind_methods;
    self->init_sod = solver_init_sod;
    self->update_primitives = solver_update_primitives;
    self->compute_flux = solver_compute_flux;
    self->compute_dt = solver_compute_dt;
    self->apply_boundary = solver_apply_boundary;
        //self->enforce_physical_state = solver_enforce_physical_state;
    self->apply_artificial_viscosity = solver_apply_artificial_viscosity;
    self->step_maccormack = solver_step_maccormack;
    self->write_tecplot = solver_write_tecplot;
    self->destroy = solver_destroy;
}

/*
=============================方法结束~ 接下来进入程序主干=================================================
*/

//  方法定义完了，接下来处理一些main函数中进行的杂事，所以都用static限制在这个程序的运行生命周期以内
//  它们分别是
/*
static solver solver_create_defaulf(void){...}
    建立初始程序骨架和默认参数，在内存中临时创建一个求解器结构体

static int ask_yes_no(const char *prompt, int default_value) {...}

static void ask_output_filename(char *filename, size_t size) {...}

static void make_snapshot_filename(...){...}

static void print_banner(void) {...}

static void configure_riemann_case(solver *self, int *case_id) {...}

static void configure_domain_and_grid(solver *self) {...}

static void configure_numerics(solver *self) {...}

static void print_run_summary(...) {...}

*/

/*
 * ============================================================================
 * Block 5B/6: object construction, text input, filenames and process coupling
 * ============================================================================
 */

/*
 * [Design: constructor-like factory]
 *
 * C 没有 constructor。该函数：
 *
 * 1. 在 stack 上创建局部 solver s；
 * 2. 清零全部 bytes；
 * 3. 写入默认参数；
 * 4. 绑定 method pointers；
 * 5. 按值返回完整结构体。
 *
 * [C syntax: return struct by value]
 * C 允许返回结构体。编译器负责复制或使用 return-value optimization。
 * 此时数组尚未分配，所以复制的主要是标量、NULL pointers 和 function pointers。
 */
static solver solver_create_default(void) {
    solver s;
    /*
     * &s 取得对象地址。
     * memset(...,0,...) 将所有 bytes 置零，使 pointers 初始为 null representation
     * （在本目标平台成立，也是常见实践）。
     */
    memset(&s, 0, sizeof(s));

    /* Default spatial discretization: 501 points -> 500 equal intervals. */
    s.nx = 501;
    s.output_interval = 0;
    s.xmin = 0.0;
    s.xmax = 1.0;
    s.x0 = 0.5;
    s.dx = (s.xmax - s.xmin) / (double)(s.nx - 1);
    s.t = 0.0;
    s.t_max = 0.2;
    s.cfl = 0.5;
    s.gamma = 1.4;

    s.left_rho = 1.0;
    s.left_u = 0.0;
    s.left_p = 1.0;
    s.right_rho = 0.125;
    s.right_u = 0.0;
    s.right_p = 0.1;

    s.artificial_viscosity_k = 0.25;
    s.use_artificial_viscosity = 1;
    s.viscosity_sensor_type = VISCOSITY_SENSOR_RHO;
    s.rho_floor = 1.0e-10;
    s.p_floor = 1.0e-10;

    /* allocation 尚未发生，但 methods 必须先可调用。 */
    solver_bind_methods(&s);
    return s;
}

/*
 * [C strings: read one complete line]
 *
 * C string 是以 null character '\0' 结尾的 char sequence。
 * fgets 最多读取 size-1 个字符，并自动追加 '\0'。
 *
 * fgets 会保留输入中的 newline，所以随后用 strcspn 找到第一个 '\r' 或 '\n'：
 *
 *     buffer[index] = '\0';
 *
 * 相当于在那里截断字符串。
 *
 * 返回 0 表示 EOF/read failure，返回 1 表示取得一行。
 */
static int read_line(char *buffer, size_t size) {
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
    /*
     * line 是固定长度 local array，位于当前函数 stack frame。
     * 函数返回后它失效，但解析出的 value 是独立 int，不受影响。
     */
    char line[INPUT_LINE_LEN];
    int value = default_value;

    /* while(1) 创建无限循环，只通过 return 离开。 */
    while (1) {
        printf("%s [%d]: ", prompt, default_value);
        /*
         * || short-circuit：
         * 若 read_line 返回 0，则不会再访问 line[0]。
         * 空行代表用户接受 default。
         */
        if (!read_line(line, sizeof(line)) || line[0] == '\0') {
            return default_value;
        }

        /*
         * sscanf 从 string 而非 stdin 解析。
         * "%d" 要求 int *，所以传 &value。
         * 返回 1 表示成功完成一个 conversion。
         */
        if (sscanf(line, "%d", &value) == 1 &&
            value >= min_value &&
            value <= max_value) {
            return value;
        }

        printf("[Input Guard] Please input an integer in [%d, %d].\n",
               min_value, max_value);
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
        printf("%s [%.6g]: ", prompt, default_value);
        if (!read_line(line, sizeof(line)) || line[0] == '\0') {
            return default_value;
        }

        /*
         * scanf family 中 double * 对应 "%lf"；注意 printf 输出 double 使用 "%f"。
         */
        if (sscanf(line, "%lf", &value) == 1 && value >= min_value) {
            return value;
        }

        printf("[Input Guard] Please input a value >= %.6g.\n", min_value);
    }
}

/*
 * [C convention: boolean without stdbool.h]
 *
 * 返回 int：1 代表 yes，0 代表 no。
 * conditional operator `condition ? A : B` 根据条件选择一个表达式结果。
 */
static int ask_yes_no(const char *prompt, int default_value) {
    char line[INPUT_LINE_LEN];

    while (1) {
        /*
         * 'y' 是单个 char literal；"y" 是包含 y 和 '\0' 的 string literal。
         * %c 接收提升后的 char，%s 接收 char pointer。
         */
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

        printf("[Input Guard] Please input y or n.\n");
    }
}

static void ask_output_filename(
    char *filename,
    size_t size,
    int case_id
) {
    char line[OUTPUT_NAME_LEN];
    char default_filename[OUTPUT_NAME_LEN];

    /*
     * snprintf 最多写 size bytes，包括末尾 '\0'，可避免 sprintf 的 buffer overflow。
     * %02d 表示十进制整数至少占两位，不足时补零：1 -> 01。
     */
    snprintf(default_filename, sizeof(default_filename),
             "runs\\case_%02d_numerical.dat", case_id);
    printf("-> Output filename [%s]: ", default_filename);
    if (!read_line(line, sizeof(line)) || line[0] == '\0') {
        /*
         * filename 指向 main 中的 output_filename array。
         * 因为传入的是地址，此处写入会被 main 看到。
         */
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
    /*
     * 目标：
     *
     *     path/numerical.dat -> path/numerical_exact.dat
     *
     * strrchr(string, character) 返回最后一次出现该字符的位置，找不到则返回 NULL。
     * 同时检查 '\\' 和 '/'，使路径兼容 Windows separator 与 portable separator。
     */
    const char *dot = strrchr(numerical_filename, '.');
    const char *slash = strrchr(numerical_filename, '\\');
    const char *forward_slash = strrchr(numerical_filename, '/');
    const char *separator = slash;

    /*
     * 同一字符串数组中的 pointers 可以比较位置。选择更靠后的 separator。
     */
    if (forward_slash != NULL && (separator == NULL || forward_slash > separator)) {
        separator = forward_slash;
    }

    /*
     * dot 必须位于最后一个 path separator 之后，才被视为扩展名。
     * 例如 directory.name/file 中的目录点号不能当作文件扩展名。
     */
    if (dot != NULL && (separator == NULL || dot > separator)) {
        /*
         * 两个指向同一 char array 的 pointers 相减，结果是元素距离，类型本来是 ptrdiff_t。
         * 这里转成 int 供 precision field 使用；文件名长度已限制为小型 buffer。
         *
         * "%.*s" 中 * 表示字符串最大输出长度由额外 int 参数 prefix_len 提供。
         */
        int prefix_len = (int)(dot - numerical_filename);
        snprintf(exact_filename, size, "%.*s_exact%s",
                 prefix_len, numerical_filename, dot);
    } else {
        snprintf(exact_filename, size, "%s_exact.dat", numerical_filename);
    }
}

static int run_exact_solver(
    const solver *self,
    double output_time,
    const char *exact_filename
) {
    /*
     * [OS process coupling]
     *
     * Project 2 没有链接 Project 0 的函数或 object file，而是构造 command line，
     * 通过 system() 启动独立 executable：
     *
     *     Project 2 process
     *         |
     *         +-- operating system creates Project 0 process
     *                 |
     *                 +-- writes exact .dat
     *
     * 优点：两个教学项目保持独立，各自拥有 main 和编译流程。
     * 代价：参数通过文本 command line 传递，错误处理较弱，process startup 有额外开销。
     */
    char command[COMMAND_LINE_LEN];

    /*
     * 先以 binary read mode "rb" 尝试打开 executable，仅用于存在性检查。
     * 这不是执行文件。
     */
    FILE *exact_executable = fopen(EXACT_SOLVER_EXE, "rb");

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

    /*
     * [Windows quoting]
     *
     * 外层字符串最终交给 command processor。可执行路径和输出路径都可能包含空格，
     * 因而需要双引号。C source 中要写 \" 才能在运行时生成一个 "。
     *
     * 开头和结尾的额外双引号适配 Windows cmd.exe 对 quoted executable path 的解析：
     *
     *     ""path\riemann_exact.exe" --batch ... "output.dat""
     *
     * "%.17g" 为 double 提供足够有效数字，通常可 round-trip 回同一 binary64 值。
     */
    snprintf(
        command,
        sizeof(command),
        "\"\"%s\" --batch "
        "%.17g %.17g %.17g "
        "%.17g %.17g %.17g "
        "%.17g %.17g %.17g %.17g %d %.17g \"%s\"\"",
        EXACT_SOLVER_EXE,
        self->left_rho, self->left_u, self->left_p,
        self->right_rho, self->right_u, self->right_p,
        self->gamma,
        self->xmin, self->xmax, self->x0,
        self->nx, output_time,
        exact_filename
    );

    printf("[Exact Solver] Launching external Project 0 process...\n");
    /*
     * stdout 可能被 buffer。启动子进程前 flush，可保证提示先显示，避免父子进程输出乱序。
     */
    fflush(stdout);

    /*
     * system 返回 command processor 的状态编码；这里把任何非零值视为失败。
     *
     * [Security]
     * system 不适合处理不可信输入，因为 filename 可能注入 shell metacharacters。
     * 本作业只处理本地受控参数。工业代码应使用 CreateProcess/exec 类 API 并传 argument list。
     */
    if (system(command) != 0) {
        fprintf(stderr, "[Exact Solver Error] External process failed.\n");
        return 0;
    }

    printf("[Exact Output] %s\n", exact_filename);
    return 1;
}

static void make_snapshot_filename(     // 保存快照，中继节点，以便未来从真正值得关注的时间段开始，而非从头再算
    const char *final_filename,
    int step,
    char *snapshot_filename,
    size_t size
) {
    /*
     * 目标：
     *
     *     numerical.dat + step 100
     *     -> numerical_step_000100.dat
     *
     * `%06d` 用前导零固定步号宽度，使 lexicographic filename order 与 step order 一致。
     */
    const char *dot = strrchr(final_filename, '.');
    const char *slash = strrchr(final_filename, '\\');
    const char *forward_slash = strrchr(final_filename, '/');
    const char *separator = slash;
    int prefix_len = 0;

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

static void print_banner(void) {
    /*
     * chcp 65001 把 Windows console code page 改为 UTF-8。
     * `> nul` 把 chcp 自己的提示重定向到 null device。
     *
     * [Portability]
     * 这是 Windows-specific command；在 Linux/macOS 上不应使用。
     * 它只影响界面字符显示，不参与数值计算。
     */
    system("chcp 65001 > nul");
    // 界面打印
    printf("  　 　 ＿＿＿\n");
    printf("  　 ／＞　　フ\n");
    printf("    | 　_　 _ |\n");
    printf("   ／` ミ＿xノ\n");
    printf("   /　　　 　 |\n");
    printf("  /　 ヽ　　 ﾉ\n");
    printf(" │　 　|　|　|\n");
    printf("／￣   |  |　|\n");
    printf("| (￣ヽ＿_ヽ_)__)\n");
    printf("＼二つ                \n");

    printf("==================================================\n");
    printf("      1-D Euler MacCormack CFD Solver\n");
    printf("==================================================\n");
    printf("      Data lives in struct. Methods live nearby. -> OOP~!\n");
    printf("==================================================\n\n");
}

static void configure_riemann_case(solver *self, int *case_id) {
    /*
     * [C syntax: output parameter]
     *
     * case_id 是 int *。函数通过 `*case_id = ...` 修改 main 中的 case_id。
     * `*` 在表达式中是 dereference operator，访问 pointer 指向的对象。
     *
     * self 同样是 output/in-out parameter：函数修改 solver 中的 left/right states。
     */
    printf("[Step 1] Select Riemann Initial Condition\n");
    printf("1 - Sod shock tube\n");
    printf("2 - Lax shock tube\n");
    printf("3 - Subsonic double-expansion test\n");
    printf("4 - Sjogreen supersonic expansion test\n");
    printf("5 - Contact discontinuity with double expansion\n");
    printf("6 - Contact discontinuity with double shock\n");
    printf("7 - Pure contact discontinuity\n");

    *case_id = ask_int_range("Input case index", 1, 1, 7);

    /*
     * 每个分支只设置 Riemann primitive states 和建议 final time。
     * domain、grid、gamma、CFL、beta 等在后续步骤独立设置。
     *
     * [Design]
     * 当前使用 if/else-if chain，适合仅七个教学 case。
     * 更多 case 时可改为 const table of structs，但会增加初学阶段的抽象负担。
     */
    if (*case_id == 1) {
        /* Sod: left rarefaction, contact discontinuity, right shock. */
        self->left_rho = 1.0;
        self->left_u = 0.0;
        self->left_p = 1.0;
        self->right_rho = 0.125;
        self->right_u = 0.0;
        self->right_p = 0.1;
        self->t_max = 0.2;
    } else if (*case_id == 2) {
        /* Lax shock tube: 更强的速度和压力差。 */
        self->left_rho = 0.445;
        self->left_u = 0.698;
        self->left_p = 3.528;
        self->right_rho = 0.5;
        self->right_u = 0.0;
        self->right_p = 0.571;
        self->t_max = 0.16;
    } else if (*case_id == 3) {
        /* Symmetric subsonic double expansion. */
        self->left_rho = 1.0;
        self->left_u = -2.0;
        self->left_p = 4.0;
        self->right_rho = 1.0;
        self->right_u = 2.0;
        self->right_p = 4.0;
        self->t_max = 0.15;
    } else if (*case_id == 4) {
        /*
         * Sjogreen supersonic expansion.
         * 两侧流体背向运动且压力低，中心可能接近/形成 vacuum；经典 solver 可能失效。
         */
        self->left_rho = 1.0;
        self->left_u = -2.0;
        self->left_p = 0.4;
        self->right_rho = 1.0;
        self->right_u = 2.0;
        self->right_p = 0.4;
        self->t_max = 0.15;
    } else if (*case_id == 5) {
        /* Equal pressure with density/velocity jump, producing contact + expansions. */
        self->left_rho = 1.0;
        self->left_u = -0.2;
        self->left_p = 0.5;
        self->right_rho = 0.5;
        self->right_u = 0.5;
        self->right_p = 0.5;
        self->t_max = 0.2;
    } else if (*case_id == 6) {
        /* Opposing states producing compressive waves / double-shock behavior. */
        self->left_rho = 0.4;
        self->left_u = 0.5;
        self->left_p = 1.0;
        self->right_rho = 1.0;
        self->right_u = -0.5;
        self->right_p = 0.9;
        self->t_max = 0.2;
    } else {
        /*
         * Pure contact discontinuity:
         * u_L=u_R, p_L=p_R，仅 rho 跳跃。
         * exact solution 是以速度 u 平移的密度间断，速度和压力应保持常数。
         */
        self->left_rho = 10.0;
        self->left_u = 1.0;
        self->left_p = 2.0;
        self->right_rho = 1.0;
        self->right_u = 1.0;
        self->right_p = 2.0;
        self->t_max = 0.2;
    }

    printf("\n");
}

static void configure_domain_and_grid(solver *self) {
    /*
     * [Numerics: uniform nodal grid]
     *
     * nx 是网格点数，区间数为 nx-1，因此：
     *
     *     dx = (xmax-xmin)/(nx-1)
     *
     * 若误写成除以 nx，最后一点将到不了 xmax。
     */
    printf("[Step 2] Configure Domain & Grid\n");
    self->xmin = ask_double_min("-> xmin", self->xmin, -1.0e30);
    self->xmax = ask_double_min("-> xmax", self->xmax, self->xmin + 1.0e-12);
    self->x0 = ask_double_min("-> discontinuity position x0", self->x0, self->xmin);

    /*
     * ask_double_min 已保证 x0>=xmin；这里只需处理 x0>xmax。
     * 当前策略是 reset 到中心，而不是终止程序重新询问。
     */
    if (self->x0 > self->xmax) {
        printf("[Input Guard] x0 is outside domain, reset to domain center.\n");
        self->x0 = 0.5 * (self->xmin + self->xmax);
    }

    self->nx = ask_int_range("-> grid points nx", self->nx, 5, 200000);
    /*
     * (double) 强制 floating-point division。
     * 分子本来已经是 double，因此即使不 cast 结果也是 double；显式写出可强调数学意图。
     */
    self->dx = (self->xmax - self->xmin) / (double)(self->nx - 1);

    printf("--> dx auto computed as %.10f\n\n", self->dx);
}

static void configure_numerics(solver *self) {
    /*
     * 将 gas model、time integration 和 stabilizer 参数集中配置。
     *
     * gamma>1 是 calorically perfect ideal gas 公式 p=(gamma-1)rho e 的要求。
     * 这里只限制 CFL>0，没有强制 CFL<=1，目的是允许教学实验观察不稳定参数。
     */
    printf("[Step 3] Configure Time Marching & Stabilizer\n");
    self->gamma = ask_double_min("-> gamma", self->gamma, 1.000001);
    self->cfl = ask_double_min("-> target CFL", self->cfl, 1.0e-12);
    self->t_max = ask_double_min("-> total simulation time", self->t_max, 0.0);
    self->use_artificial_viscosity =
        ask_yes_no("-> enable artificial viscosity? (recommended near shocks)",
                   self->use_artificial_viscosity);

    if (self->use_artificial_viscosity) {
        self->artificial_viscosity_k =
            ask_double_min("-> artificial viscosity coefficient k",
                           self->artificial_viscosity_k,
                           0.0);
        printf("-> artificial viscosity sensor variable:\n");
        printf("   1 - rho\n");
        printf("   2 - u\n");
        printf("   3 - p\n");
        /*
         * ask_int_range 返回 int。显式 cast 为 enum 表明该整数现在表示 sensor type。
         * 范围已经限制在枚举定义的 1..3。
         */
        self->viscosity_sensor_type = (ViscositySensorType)
            ask_int_range("   Input sensor index",
                          (int)self->viscosity_sensor_type,
                          (int)VISCOSITY_SENSOR_RHO,
                          (int)VISCOSITY_SENSOR_P);
    } else {
        /*
         * 关闭开关时同步把 beta 清零，避免 summary 中出现“off, k=0.25”的歧义。
         */
        self->artificial_viscosity_k = 0.0;
    }

    self->output_interval =
        ask_int_range("-> snapshot output interval steps (0 means final only)",
                      self->output_interval,
                      0,
                      1000000);

    printf("\n");
}

static void print_run_summary(solver *self, int case_id, const char *filename) {
    /*
     * 该函数只读取状态，不修改 solver。更严格的接口可写 `const solver *self`；
     * 当前保留原函数签名以与开发版逐句一致。
     *
     * [Reproducibility]
     * 正式参数扫描还会把这些参数保存为 JSON/CSV。console summary 适合人工运行检查，
     * 但不应成为唯一实验记录。
     */
    printf("[Step 4] Final Check Before Launch\n");
    printf("case_id      = %d\n", case_id);
    printf("domain       = [%.6f, %.6f], x0 = %.6f\n",
           self->xmin, self->xmax, self->x0);
    printf("grid         = nx %d, dx %.10f\n", self->nx, self->dx);
    printf("time         = t_max %.6f, CFL %.6f\n", self->t_max, self->cfl);
    printf("gas          = gamma %.6f\n", self->gamma);
    printf("left state   = rho %.6f, u %.6f, p %.6f\n",
           self->left_rho, self->left_u, self->left_p);
    printf("right state  = rho %.6f, u %.6f, p %.6f\n",
           self->right_rho, self->right_u, self->right_p);
    printf("viscosity    = %s, k %.6f, sensor %s\n",
           self->use_artificial_viscosity ? "on" : "off",
           self->artificial_viscosity_k,
           viscosity_sensor_name(self->viscosity_sensor_type));
    printf("snapshots    = every %d steps\n", self->output_interval);
    printf("output       = %s\n", filename);
    printf("--------------------------------------------------\n");
}

/*
 * ============================================================================
 * Program entry point and complete lifetime
 * ============================================================================
 *
 * Hosted C program 从 main 开始执行。`int main(void)` 明确表示不接收命令行参数。
 *
 * 返回操作系统：
 *
 *     0 -> success
 *     nonzero -> failure
 *
 * 本程序完整生命周期：
 *
 *     construct defaults
 *     -> configure
 *     -> allocate
 *     -> initialize
 *     -> time marching / snapshots
 *     -> final numerical output
 *     -> exact solver process
 *     -> destroy
 */
int main(void) {
    /*
     * my_solver 是 main stack frame 中的结构体对象。
     * 其中动态数组稍后位于 heap；对象本身和数组不是同一块内存。
     */
    solver my_solver = solver_create_default();

    /*
     * char arrays 作为 mutable string buffers。
     * +16/+32 为追加 "_exact"、"_step_000000" 等 suffix 预留空间。
     */
    char output_filename[OUTPUT_NAME_LEN];
    char exact_filename[OUTPUT_NAME_LEN + 16];
    char snapshot_filename[OUTPUT_NAME_LEN + 32];
    int case_id = 1;

    /* Phase 1: configuration. */
    print_banner();
    configure_riemann_case(&my_solver, &case_id);
    configure_domain_and_grid(&my_solver);
    configure_numerics(&my_solver);
    ask_output_filename(output_filename, sizeof(output_filename), case_id);
    /*
     * sizeof(array) 在 array 仍处于当前 scope 时返回整个 buffer byte count。
     * 若传入函数后再对 parameter 使用 sizeof，只会得到 pointer size。
     */
    make_exact_filename(output_filename, exact_filename, sizeof(exact_filename));
    printf("\n");

    print_run_summary(&my_solver, case_id, output_filename);

    /*
     * Phase 2: resource acquisition.
     *
     * `my_solver.allocate` 是 function pointer。
     * `&my_solver` 把当前对象地址作为显式 self 参数传入。
     *
     * `!result` 把 allocate 返回 0 的失败状态变为 true。
     */
    if (!my_solver.allocate(&my_solver)) {
        fprintf(stderr, "[Fatal Error] Memory allocation failed.\n");
        /*
         * 即使只分配了一部分，destroy 也安全，因为剩余 pointers 初始为 NULL，
         * 而 free(NULL) 合法。
         */
        my_solver.destroy(&my_solver);
        return 1;
    }

    /* Phase 3: construct Q(x,0) from selected left/right primitive states. */
    my_solver.init_sod(&my_solver);

    /*
     * Phase 4A: optional t=0 snapshot.
     *
     * output_interval>0 代表用户要求 transient history。step_count 初始为 0，
     * 因此先保存 initial condition，动画才真正从 t=0 开始。
     */
    if (my_solver.output_interval > 0) {
        make_snapshot_filename(output_filename,
                               my_solver.step_count,
                               snapshot_filename,
                               sizeof(snapshot_filename));
        my_solver.write_tecplot(&my_solver, snapshot_filename);
        printf("[Snapshot] %s\n", snapshot_filename);

        make_exact_filename(snapshot_filename,
                            exact_filename,
                            sizeof(exact_filename));
        /*
         * 每个 numerical frame 都立即生成同一物理时间的 exact frame。
         * 任一 exact process 失败便清理 solver 并退出，避免留下看似完整但时间不配对的数据集。
         */
        if (!run_exact_solver(&my_solver, my_solver.t, exact_filename)) {
            my_solver.destroy(&my_solver);
            return 1;
        }
    }

    printf("[Calculating] MacCormack time marching started...\n");
    /*
     * Phase 4B: explicit time marching loop.
     *
     * `&&` 要求两个条件都为 true：
     *
     * 1. 尚未到 final time；
     * 2. 尚未达到 emergency step cap。
     *
     * 200000 不是数学收敛标准，只是防止 dt 异常缩小后无限运行的 guard。
     */
    while (my_solver.t < my_solver.t_max && my_solver.step_count < 200000) {
        my_solver.step_maccormack(&my_solver);

        /*
         * `%` 是 integer remainder operator。
         * step_count % interval == 0 表示当前步号能被 interval 整除。
         *
         * 前面的 output_interval>0 必须保留，避免对 0 取余导致 undefined behavior。
         * `&&` short-circuit 保证 interval==0 时不会计算右侧 remainder。
         */
        if (my_solver.output_interval > 0 &&
            my_solver.step_count % my_solver.output_interval == 0) {
            make_snapshot_filename(output_filename,
                                   my_solver.step_count,
                                   snapshot_filename,
                                   sizeof(snapshot_filename));
            my_solver.write_tecplot(&my_solver, snapshot_filename);
            printf("[Snapshot] %s\n", snapshot_filename);

            make_exact_filename(snapshot_filename,
                                exact_filename,
                                sizeof(exact_filename));
            if (!run_exact_solver(&my_solver, my_solver.t, exact_filename)) {
                my_solver.destroy(&my_solver);
                return 1;
            }
        }
    }

    /*
     * 若触发 step cap，程序当前仍会输出当时的非最终状态，并调用对应时刻的 exact solver。
     * beta sweep 后处理会读取 ZONE time，将其标记为 unstable/incomplete，而不是计算伪误差。
     */
    if (my_solver.step_count >= 200000) {
        printf("[Warning] Step limit reached before t_max.\n");
    }

    /* Phase 5: final (or step-cap) numerical field. */
    my_solver.write_tecplot(&my_solver, output_filename);

    printf("[Done] t=%.8f, steps=%d\n", my_solver.t, my_solver.step_count);
    printf("[Output] %s\n", output_filename);

    /* Generate exact solution at the actual numerical output time. */
    make_exact_filename(output_filename, exact_filename, sizeof(exact_filename));
    if (!run_exact_solver(&my_solver, my_solver.t, exact_filename)) {
        my_solver.destroy(&my_solver);
        return 1;
    }

    /*
     * Phase 6: release all owned resources.
     * 成功路径和所有 allocation 后的失败路径都必须经过 destroy。
     */
    my_solver.destroy(&my_solver);
    return 0;
}
