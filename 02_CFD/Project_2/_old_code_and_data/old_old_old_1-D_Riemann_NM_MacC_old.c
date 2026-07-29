#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define INPUT_LINE_LEN 128
#define OUTPUT_NAME_LEN 128
#define COMMAND_LINE_LEN 1024
#define EXACT_SOLVER_EXE "_Analysical_Solution_Solver\\riemann_exact.exe" // 调用先前的Project_0.c程序

#define GAMMA 1.4

typedef enum {  // 用于判断人工粘性系数用什么算的特征数
    VISCOSITY_SENSOR_RHO = 1,   // 用密度算
    VISCOSITY_SENSOR_U = 2,     // 用速度算
    VISCOSITY_SENSOR_P = 3      // 用压强算
} ViscositySensorType;

typedef struct Riemann_1D_MacC_solver solver; //这么写语义更明确，未来一眼就知道solver是Riemann_1D求解器，而且后面结构体里方法函数指针的定义要指向solver，所以没法用缩写法

/*
    1-D Euler equations:
        原始量 W = [rho, u, p] (一维流动，所以速度变量只有一维)
        守恒量 Q = [rho, rho*u, rho*E] 
        矢通量 F = [rho*u, rho*u*u + p, u*(rho*E + p)]

    MacCormack 格式使用两个步骤:

        预测步 Predictor: U_pre_i = U_i - dt/dx * (F_{i+1} - F_i)
        矫正步 Corrector: U_i^{n+1} = 0.5 * (U_i + U_pre_i
                              - dt/dx * (F_pre_i - F_pre_{i-1}))
*/

// 一共有两个技术细节，它们是①主变量选取 ②如何储存信息，具体讨论已经放在README.md中

/*
    类似于project 1_1-D_LinearAdvection_Solver.c，这里也采用OOP的思想，把对象和方法打包在一起，方便后续优化和调试，这里就不赘述了
*/

/*
    由于这个问题比较复杂，我们用OOP的三步法拆解：
    ① 定义对象和方法
    ② 从main函数中提取主干操作并解耦出来封装成局部私有化函数 static function
    ③ 执行main函数
*/
struct Riemann_1D_MacC_solver {
    int nx;             //网格总数
    int step_count;    //迭代次数
    int output_interval;    //快照采用频率 (隔多少个时间步骤输出一次流场数据)

    // 沙箱的几何长度与时间长度
    double xmin; //左边界
    double xmax; //右边界
    double x0; //非连续面坐标
    double dx;      //空间步长
    double dt;      //时间步长
    double t;       //绝对时间
    double t_max;   //整个模拟的时间长度
    double cfl;     //CFL条件数 - Index: 冯诺依曼数值稳定性分析

    double gamma;   //气体比热比

    double left_rho;    //初始左rho
    double left_u;      //初始左u
    double left_p;      //初始左p
    double right_rho;   //初始右rho
    double right_u;     //初始右u
    double right_p;     //初始右p

    double artificial_viscosity_k;  //人工粘性系数
    int use_artificial_viscosity;   //人工粘性系数的开关
    ViscositySensorType viscosity_sensor_type;
    double rho_floor;               //
    double p_floor;                 //

    //  守恒量: q1=rho, q2=rho*u, q3=rho*E
    double *q1; // q1=rho
    double *q2; // q2=rho*u
    double *q3; // q3=rho*E

    //  预测步变量, pre表示上加一线，对应ppt里的描述
    double *q1_pre; // 这是预测步量，注意守恒量q1=rho
    double *q2_pre; // 这是预测步量，注意守恒量q2=rho*u
    double *q3_pre; // 这是预测步量，注意守恒量q3=rho*E

    //  矫正步，也即 at the corre time step
    double *q1_corre;    // 这是矫正步变量，注意守恒量q1=rho
    double *q2_corre;    // 这是矫正步变量，注意守恒量q2=rho*u
    double *q3_corre;    // 这是矫正步变量，注意守恒量q3=rho*E

    //  当前步和预测步的失通量
    double *f1;         //失通量
    double *f2;         //失通量
    double *f3;         //失通量
    double *f1_pre;     //预测步，失通量
    double *f2_pre;     //预测步，失通量
    double *f3_pre;     //预测步，失通量

    //  用于输出的原始量, 中间变量（注意到我们实际的每一步推进的是Q）[rho u p E a], and viscosity sensors
    double *rho;
    double *u;
    double *p;
    double *E;
    double *a;      //局部的声速，用来算谱半径
    double *visc_sensor;

    //  方法函数（指针）
    //Example:在结构体里定义名为“allocate”的函数指针变量（未来必然指向某个真实函数    且该函数返回整型    接收solver *self为参数    
    /*所以它可以指向
        void solver_step_maccormack(solver *self) {
            ...
        }
        这样的真实函数
    */
    //这个求解器的方法太多，所以需要一些独特的处理
    int  (*allocate)(solver *self);         // 单独定义一函数来开辟内存空间
    void (*bind_methods)(solver *self);     // 单独定义一函数来进行函数指针与具体函数的绑定（即实现“方法”）
    void (*init_sod)(solver *self);     //初始化sod问题
    void (*update_primitives)(solver *self);        //更新原始量(primitives) Q
    void (*compute_flux)(       //计算矢通量
        solver *self,
        const double *q1,   //用const保证只读，注意到这里是传址调用，以防Q在该函数内部被修改
        const double *q2,
        const double *q3,
        double *f1,
        double *f2,
        double *f3
    );
    double (*compute_dt)(solver *self);     //根据CFL和空间步，算时间步
    void (*apply_boundary)(     //设置边界条件
        solver *self,
        double *q1,
        double *q2,
        double *q3
    );
    /*
    注意到声速的计算式a = \sqrt{Gamma*p/rho}要求我们得到的p、rho应该组合出一个正数（数学要求），
    然而震荡的数值计算过程中很有可能组合出一个负数
    所以应该加一个保护函数，但这里我们想讨论的正是这种情况发生的可能性，所以我先注释掉这一块代码
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
    void (*destroy)(solver *self);  //释放allocate在整个程序中，在堆上圈的所有内存，程序结束
};

/*
======================== ↓开始写Method↓ =========================
*/

//static - 局部私有化，需要区别于静态数组的概念
//目前的通俗理解：①使用权只存在于当前文件内部 ② 生命周期只局限于这个程序被运行时
//也就是说如果这个时候有一个联立的外部函数也有一个同名函数，它们不会起冲突。

static double pressure_from_q( // 用守恒量反算压强 利用公式 E = \frac{p}{rho(gamma - 1 )} + u*u/2
    solver *self,
    double q1,
    double q2,
    double q3
) {
    double kinetic_energy_density = 0.5 * q2 * q2 / q1;
    return (self->gamma - 1.0) * (q3 - kinetic_energy_density);
}

static double total_energy_density( // 算单位体积流体的总能量（E*rho）
    solver *self,
    double rho,
    double u,
    double p
) {
    return p / (self->gamma - 1.0) + 0.5 * rho * u * u;
}

static double *alloc_double_array(size_t n) { //连续开辟空间，具体功能联系solver_allocate看
    return (double *)calloc(n, sizeof(double));
}

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
老分配内存空间的 malloc 写法
(double *)malloc(n * sizeof(double));
这里的 n * sizeof(double) 是在传参之前由 CPU 直接计算的。
如果网格数 n 极度庞大（或者由于人工算法出错变成了一个诡异的极大正数），
这个乘法结果可能会发生整数溢出（Integer Overflow）。

新写法：
(double *)calloc(n, sizeof(double));
calloc 会在系统内部安全地执行乘法检查。
一旦判定 n * size 溢出了系统寻址极限，它会直接拦截并安全地返回 NULL（空指针），触发你后面写的 if (!self->q1) 守卫，让程序优雅地终止，而不是带着内伤继续“裸奔”。
*/


int solver_allocate(solver *self) { //给所有方法分配内存，记得绑定为allocate
    size_t n = (size_t)self->nx;    //这里把网格数的长度标准化了，并显式地在solver_allocate里定义成n

    // 连续地给各个变量在堆上开辟内存空间，内存上 = 变量个数*空间长度，（堆(Heap)上的空间是永恒的，栈(Stack)上的空间是缓存）
    self->q1 = alloc_double_array(n);
    self->q2 = alloc_double_array(n);
    self->q3 = alloc_double_array(n);

    self->q1_pre = alloc_double_array(n);
    self->q2_pre = alloc_double_array(n);
    self->q3_pre = alloc_double_array(n);

    self->q1_corre = alloc_double_array(n);
    self->q2_corre = alloc_double_array(n);
    self->q3_corre = alloc_double_array(n);

    self->f1 = alloc_double_array(n);
    self->f2 = alloc_double_array(n);
    self->f3 = alloc_double_array(n);

    self->f1_pre = alloc_double_array(n);
    self->f2_pre = alloc_double_array(n);
    self->f3_pre = alloc_double_array(n);

    self->rho = alloc_double_array(n);
    self->u = alloc_double_array(n);
    self->p = alloc_double_array(n);
    self->E = alloc_double_array(n);
    self->a = alloc_double_array(n);
    self->visc_sensor = alloc_double_array(n);

    if (!self->q1 || !self->q2 || !self->q3 ||
        !self->q1_pre || !self->q2_pre || !self->q3_pre ||
        !self->q1_corre || !self->q2_corre || !self->q3_corre ||
        !self->f1 || !self->f2 || !self->f3 ||
        !self->f1_pre || !self->f2_pre || !self->f3_pre ||
        !self->rho || !self->u || !self->p || !self->E ||
        !self->a || !self->visc_sensor) {
        return 0;
    }   //一个验点，判断内存分配这个过程中是否出错，出错就报错 0

    return 1;
}

void solver_update_primitives(solver *self) { //更新原始量
    for (int i = 0; i < self->nx; i++) {
        self->rho[i] = self->q1[i];
        self->u[i] = self->q2[i] / self->q1[i];
        self->p[i] = pressure_from_q(self, self->q1[i], self->q2[i], self->q3[i]);
        self->E[i] = self->q3[i] / self->q1[i];

        if (self->p[i] > 0.0 && self->rho[i] > 0.0) {
            self->a[i] = sqrt(self->gamma * self->p[i] / self->rho[i]);
        } else {
            self->a[i] = 0.0;
        }
    }
}

void solver_compute_flux( //用每一步的Q求F
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

        f1[i] = q2[i];
        f2[i] = q2[i] * u + p;
        f3[i] = (q3[i] + p) * u;
    }
}

double solver_compute_dt(solver *self) { //根据条件数和步长算dt
    double max_speed = 0.0;

    self->update_primitives(self);

    for (int i = 0; i < self->nx; i++) {
        double speed = fabs(self->u[i]) + self->a[i];
        if (speed > max_speed) {
            max_speed = speed;
        }
    }

    if (max_speed <= 0.0) {
        return 1.0e-8;
    }

    return self->cfl * self->dx / max_speed;
}

//写边界条件
void solver_apply_boundary(
    solver *self, 
    double *q1, 
    double *q2, 
    double *q3
) {
    int last = self->nx - 1;

    q1[0] = q1[1];
    q2[0] = q2[1];
    q3[0] = q3[1];

    q1[last] = q1[last - 1];
    q2[last] = q2[last - 1];
    q3[last] = q3[last - 1];
}

    /*
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

void solver_apply_artificial_viscosity(solver *self) { // 算人工粘性
    if (!self->use_artificial_viscosity || self->artificial_viscosity_k <= 0.0) {
        return;
    }

    self->update_primitives(self);
    const double *sensor_values = viscosity_sensor_values(self);

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

    for (int i = 1; i < self->nx - 1; i++) {
        double eps = self->visc_sensor[i];

        self->q1_corre[i] += eps * (self->q1[i + 1] - 2.0 * self->q1[i] + self->q1[i - 1]);
        self->q2_corre[i] += eps * (self->q2[i + 1] - 2.0 * self->q2[i] + self->q2[i - 1]);
        self->q3_corre[i] += eps * (self->q3[i + 1] - 2.0 * self->q3[i] + self->q3[i - 1]);
    }
}

void solver_init_sod(solver *self) {    //初值条件定义
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

        self->q1[i] = rho;
        self->q2[i] = rho * u;
        self->q3[i] = total_energy_density(self, rho, u, p);
    }

    self->apply_boundary(self, self->q1, self->q2, self->q3);
        //self->enforce_physical_state(self, self->q1, self->q2, self->q3);
    self->update_primitives(self);
}

void solver_step_maccormack(solver *self) { // 预估步算法
    double lambda = 0.0;

    self->dt = self->compute_dt(self);
    if (self->t + self->dt > self->t_max) {
        self->dt = self->t_max - self->t;
    }
    lambda = self->dt / self->dx;

    self->compute_flux(self, self->q1, self->q2, self->q3,
                       self->f1, self->f2, self->f3);

    for (int i = 1; i < self->nx - 1; i++) {
        self->q1_pre[i] = self->q1[i] - lambda * (self->f1[i + 1] - self->f1[i]);
        self->q2_pre[i] = self->q2[i] - lambda * (self->f2[i + 1] - self->f2[i]);
        self->q3_pre[i] = self->q3[i] - lambda * (self->f3[i + 1] - self->f3[i]);
    }

    self->apply_boundary(self, self->q1_pre, self->q2_pre, self->q3_pre);
        //self->enforce_physical_state(self, self->q1_pre, self->q2_pre, self->q3_pre);

    self->compute_flux(self, self->q1_pre, self->q2_pre, self->q3_pre,
                       self->f1_pre, self->f2_pre, self->f3_pre);

    for (int i = 1; i < self->nx - 1; i++) {
        self->q1_corre[i] = 0.5 * (self->q1[i] + self->q1_pre[i]
            - lambda * (self->f1_pre[i] - self->f1_pre[i - 1]));
        self->q2_corre[i] = 0.5 * (self->q2[i] + self->q2_pre[i]
            - lambda * (self->f2_pre[i] - self->f2_pre[i - 1]));
        self->q3_corre[i] = 0.5 * (self->q3[i] + self->q3_pre[i]
            - lambda * (self->f3_pre[i] - self->f3_pre[i - 1]));
    }

    self->apply_artificial_viscosity(self);
    self->apply_boundary(self, self->q1_corre, self->q2_corre, self->q3_corre);
        //self->enforce_physical_state(self, self->q1_corre, self->q2_corre, self->q3_corre);

    memcpy(self->q1, self->q1_corre, sizeof(double) * (size_t)self->nx);
    memcpy(self->q2, self->q2_corre, sizeof(double) * (size_t)self->nx);
    memcpy(self->q3, self->q3_corre, sizeof(double) * (size_t)self->nx);

    self->t += self->dt;
    self->step_count++;
}

// 输出结果为tecplot格式
void solver_write_tecplot(solver *self, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("[Error] Cannot open output file: %s\n", filename);
        return;
    }

    self->update_primitives(self);

    fprintf(fp, "TITLE = \"1D Euler MacCormack Result\"\n");
    fprintf(fp, "VARIABLES = \"x\", \"rho\", \"u\", \"p\", \"E\", \"q1\", \"q2\", \"q3\"\n");
    fprintf(fp, "ZONE T=\"t=%.8f\", I=%d, F=POINT\n", self->t, self->nx);

    for (int i = 0; i < self->nx; i++) {
        double x = self->xmin + i * self->dx;
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

    fclose(fp);
}

// 释放堆上的内存
void solver_destroy(solver *self) {
    free(self->q1);
    free(self->q2);
    free(self->q3);
    free(self->q1_pre);
    free(self->q2_pre);
    free(self->q3_pre);
    free(self->q1_corre);
    free(self->q2_corre);
    free(self->q3_corre);
    free(self->f1);
    free(self->f2);
    free(self->f3);
    free(self->f1_pre);
    free(self->f2_pre);
    free(self->f3_pre);
    free(self->rho);
    free(self->u);
    free(self->p);
    free(self->E);
    free(self->a);
    free(self->visc_sensor);

    memset(self, 0, sizeof(*self));
}

void solver_bind_methods(solver *self) { // 将上邻struct中的函数指针绑定到实际的函数，以实现OOP的"方法"
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

static solver solver_create_default(void) {
    solver s;
    memset(&s, 0, sizeof(s));   // 清空整片流场所需的空间，置值为0

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

    solver_bind_methods(&s);
    return s;
} //这里考虑交互的话其中需要删掉一些内容

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
    char line[INPUT_LINE_LEN];
    int value = default_value;

    while (1) {
        printf("%s [%d]: ", prompt, default_value);
        if (!read_line(line, sizeof(line)) || line[0] == '\0') {
            return default_value;
        }

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

        if (sscanf(line, "%lf", &value) == 1 && value >= min_value) {
            return value;
        }

        printf("[Input Guard] Please input a value >= %.6g.\n", min_value);
    }
}

static int ask_yes_no(const char *prompt, int default_value) {  // 提高交互性，用户输入Y/N, 转录为 1/0,
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

        printf("[Input Guard] Please input y or n.\n");
    }
}

static void ask_output_filename(
    char *filename,
    size_t size,
    int case_id
) {  // 用户输入决定最终输出的文件名
    char line[OUTPUT_NAME_LEN];
    char default_filename[OUTPUT_NAME_LEN];

    snprintf(default_filename, sizeof(default_filename),
             "runs\\case_%02d_numerical.dat", case_id);
    printf("-> Output filename [%s]: ", default_filename);
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
    const solver *self,
    double output_time,
    const char *exact_filename
) {
    char command[COMMAND_LINE_LEN];
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
    fflush(stdout);
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
    printf("[Step 1] Select Riemann Initial Condition\n");
    printf("1 - Sod shock tube\n");
    printf("2 - Lax shock tube\n");
    printf("3 - Subsonic double-expansion test\n");
    printf("4 - Sjogreen supersonic expansion test\n");
    printf("5 - Contact discontinuity with double expansion\n");
    printf("6 - Contact discontinuity with double shock\n");
    printf("7 - Pure contact discontinuity\n");

    *case_id = ask_int_range("Input case index", 1, 1, 7);

    if (*case_id == 1) {
        self->left_rho = 1.0;
        self->left_u = 0.0;
        self->left_p = 1.0;
        self->right_rho = 0.125;
        self->right_u = 0.0;
        self->right_p = 0.1;
        self->t_max = 0.2;
    } else if (*case_id == 2) {
        self->left_rho = 0.445;
        self->left_u = 0.698;
        self->left_p = 3.528;
        self->right_rho = 0.5;
        self->right_u = 0.0;
        self->right_p = 0.571;
        self->t_max = 0.16;
    } else if (*case_id == 3) {
        self->left_rho = 1.0;
        self->left_u = -2.0;
        self->left_p = 4.0;
        self->right_rho = 1.0;
        self->right_u = 2.0;
        self->right_p = 4.0;
        self->t_max = 0.15;
    } else if (*case_id == 4) {
        self->left_rho = 1.0;
        self->left_u = -2.0;
        self->left_p = 0.4;
        self->right_rho = 1.0;
        self->right_u = 2.0;
        self->right_p = 0.4;
        self->t_max = 0.15;
    } else if (*case_id == 5) {
        self->left_rho = 1.0;
        self->left_u = -0.2;
        self->left_p = 0.5;
        self->right_rho = 0.5;
        self->right_u = 0.5;
        self->right_p = 0.5;
        self->t_max = 0.2;
    } else if (*case_id == 6) {
        self->left_rho = 0.4;
        self->left_u = 0.5;
        self->left_p = 1.0;
        self->right_rho = 1.0;
        self->right_u = -0.5;
        self->right_p = 0.9;
        self->t_max = 0.2;
    } else {
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
    printf("[Step 2] Configure Domain & Grid\n");
    self->xmin = ask_double_min("-> xmin", self->xmin, -1.0e30);
    self->xmax = ask_double_min("-> xmax", self->xmax, self->xmin + 1.0e-12);
    self->x0 = ask_double_min("-> discontinuity position x0", self->x0, self->xmin);

    if (self->x0 > self->xmax) {
        printf("[Input Guard] x0 is outside domain, reset to domain center.\n");
        self->x0 = 0.5 * (self->xmin + self->xmax);
    }

    self->nx = ask_int_range("-> grid points nx", self->nx, 5, 200000);
    self->dx = (self->xmax - self->xmin) / (double)(self->nx - 1);

    printf("--> dx auto computed as %.10f\n\n", self->dx);
}

static void configure_numerics(solver *self) {
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
        self->viscosity_sensor_type = (ViscositySensorType)
            ask_int_range("   Input sensor index",
                          (int)self->viscosity_sensor_type,
                          (int)VISCOSITY_SENSOR_RHO,
                          (int)VISCOSITY_SENSOR_P);
    } else {
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

int main(void) {
    solver my_solver = solver_create_default();
    char output_filename[OUTPUT_NAME_LEN];
    char exact_filename[OUTPUT_NAME_LEN + 16];
    char snapshot_filename[OUTPUT_NAME_LEN + 32];
    int case_id = 1;

    print_banner();
    configure_riemann_case(&my_solver, &case_id);
    configure_domain_and_grid(&my_solver);
    configure_numerics(&my_solver);
    ask_output_filename(output_filename, sizeof(output_filename), case_id);
    make_exact_filename(output_filename, exact_filename, sizeof(exact_filename));
    printf("\n");

    print_run_summary(&my_solver, case_id, output_filename);

    if (!my_solver.allocate(&my_solver)) {
        fprintf(stderr, "[Fatal Error] Memory allocation failed.\n");
        my_solver.destroy(&my_solver);
        return 1;
    }

    my_solver.init_sod(&my_solver);

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
        if (!run_exact_solver(&my_solver, my_solver.t, exact_filename)) {
            my_solver.destroy(&my_solver);
            return 1;
        }
    }

    printf("[Calculating] MacCormack time marching started...\n");
    while (my_solver.t < my_solver.t_max && my_solver.step_count < 200000) {
        my_solver.step_maccormack(&my_solver);
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

    if (my_solver.step_count >= 200000) {
        printf("[Warning] Step limit reached before t_max.\n");
    }

    my_solver.write_tecplot(&my_solver, output_filename);

    printf("[Done] t=%.8f, steps=%d\n", my_solver.t, my_solver.step_count);
    printf("[Output] %s\n", output_filename);

    make_exact_filename(output_filename, exact_filename, sizeof(exact_filename));
    if (!run_exact_solver(&my_solver, my_solver.t, exact_filename)) {
        my_solver.destroy(&my_solver);
        return 1;
    }

    my_solver.destroy(&my_solver);
    return 0;
}
