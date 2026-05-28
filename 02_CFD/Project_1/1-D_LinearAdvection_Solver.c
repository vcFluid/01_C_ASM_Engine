/*
这个问题的本质是求解一个大型方程组(离散后的1D advection Eqn)
可以回顾C求解方程组的数学案例
*/

# include <stdio.h>

# include <stdlib.h>

# include <math.h>

# include <string.h>

// 在C++或Java中，使用类将数据和方法（函数）绑定，实现面向对象编程
// C和C99中没有类的概念，只能用结构体存数据，但可以仿照类的方法，实现伪OOP哲学
// 定义一个结构体储存物理量与求解方法，对于求解方法可以用指针实现分装

/*
在 C 语言中，struct 的本质是自定义的复合数据类型（Composite Data Type）。
在内存中开辟了一块连续且结构化的存储空间（“储物柜”），用于封装不同类型的数据成员（如 int、double）以及行为载体（函数指针）。

为了在纯过程式语言中模拟面向对象的“方法（Method）”，需要打破函数与数据的边界。
通过在普通的 C 函数中引入一个显式的上下文指针参数（习惯命名为 self 或 this），
并在函数内部通过**箭头运算符（->）**访问或修改该实例的属性，从而在语法和逻辑层面上模拟出面向对象的“对象调用方法”这一哲学。

tips:
struct 声明本身并不分配内存，
Example: struct device {...}只是规定了数据的排列方式和偏移量，此时没有任何“储物柜”被建立

*/

typedef struct AdvectionSolver1D_t solver; //前向声明AdvectionSolver1D_t并定义别名

struct AdvectionSolver1D_t { //定义内容
    int nx;             //离散空间点数
    double dx;          //空间步长
    double dt;          //时间步长
    double a;           //对流速度or特征线速度（对流项前方的系数）
    double *u;          //USE *u NOT u[N][N] !!! 定义当前时间应变量,对应 n , why use *u, 见README
    double *u_F;        //下一时刻应变量，对应 n+1 ,
    double *u_B;        //上一时刻应变量，对应 n-1 ，预留，虽然题设5种格式都没有用到时间前插，但便于未来发展


    //定义方法函数指针，我们已经前向声明了，就不用struct solver*了，直接用solver*就行，优雅
    void (*FTBS)(solver* self);
    void (*LAX)(solver* self);
    void (*LAX_WENDROFF)(solver* self);
    void (*WARMING_BEAM)(solver* self);
    void (*ROE)(solver* self);
}; //已经命名为solver了就不要写别名了，以防变量污染

            /*
                函数指针的定义语法：
                void (*FTBS)(solver* self);
                返回值类型 (*指针变量名)(参数列表)

                (*FTBS)     --> 确认not函数or普通变量，而是指针变量
                void        --> 明确目标，指向的函数返回值必须为空
                (solver)    --> 明确路径, 如何传参，明确了只能接收类型为 solver*的指针作为形参，而solver*
            */



/*定义求解方法（函数），通过指向函数的指针实现*/

void method_FTBS(solver* self) {

    //计算当前网格下的CFL数（库朗数），可以用量纲分析判断这是一个无量纲数，一个是便于程序编译，另一个是便于判断稳定性
    double nu = self->a * self->dt / self->dx;
    
    //空间遍历（由于使用 i-1，循环从 1 开始）
    for (int i = 1; i < self->nx; i++) {
        self->u_F[i] = self->u[i] - nu * (self->u[i] - self->u[i-1]);   //FTBS离散格式代数方程
    }
    
    self->u[0] = 0.0;
    self->u_F[0] = 0.0; //偷懒做法，正确的做法是把BC解耦出来，与IC并列存在
}

void method_LAX(solver* self) {
    double nu = self->a * self->dt / self->dx;
    
    // Lax格式需要 i-1 和 i+1，所以循环必须从 1 到 nx-2
    for (int i = 1; i < self->nx - 1; i++) {
        self->u_F[i] = 0.5 * (self->u[i-1] + self->u[i+1]) - 0.5 * nu * (self->u[i+1] - self->u[i-1]);
    }
    
    // 数值边界条件：首节点固定（或由物理边界决定），尾节点采用零梯度外推防止越界
    self->u_F[0] = 0.0;
    self->u_F[self->nx - 1] = self->u[self->nx - 2]; 
}

void method_LAX_WENDROFF(solver* self) {
    double nu = self->a * self->dt / self->dx;
    
    // Lax-Wendroff 同样需要 i+1 和 i-1 节点
    for (int i = 1; i < self->nx - 1; i++) {
        self->u_F[i] = self->u[i] - 0.5 * nu * (self->u[i+1] - self->u[i-1]) 
                     + 0.5 * nu * nu * (self->u[i+1] - 2.0 * self->u[i] + self->u[i-1]);
    }
    
    // 数值边界条件处理
    self->u_F[0] = 0.0;
    self->u_F[self->nx - 1] = self->u[self->nx - 2]; 
}

void method_WARMING_BEAM(solver* self) {
    double nu = self->a * self->dt / self->dx;
    
    // Warming-Beam 格式需要 i-1 和 i-2 节点。
    // 因此对于 i=1 节点，没有 i-2 可用，需要退化（降阶）为一阶迎风格式 (FTBS)
    self->u_F[1] = self->u[1] - nu * (self->u[1] - self->u[0]);
    
    // 从 i=2 开始使用纯正的二阶 Warming-Beam
    for (int i = 2; i < self->nx; i++) {
        self->u_F[i] = self->u[i] - 0.5 * nu * (3.0 * self->u[i] - 4.0 * self->u[i-1] + self->u[i-2]) 
                     + 0.5 * nu * nu * (self->u[i] - 2.0 * self->u[i-1] + self->u[i-2]);
    }
    
    self->u_F[0] = 0.0;
}

void method_ROE(solver* self) {
    double nu = self->a * self->dt / self->dx;
    // Roe格式的核心是取波速的绝对值来构建耗散项，实现“智能迎风”
    double nu_abs = fabs(self->a) * self->dt / self->dx; 
    
    // 需要 i+1 和 i-1 节点
    for (int i = 1; i < self->nx - 1; i++) {
        // 通量差分分裂形式化简
        self->u_F[i] = self->u[i] - 0.5 * nu * (self->u[i+1] - self->u[i-1]) 
                     + 0.5 * nu_abs * (self->u[i+1] - 2.0 * self->u[i] + self->u[i-1]);
    }
    
    // 数值边界条件
    self->u_F[0] = 0.0;
    self->u_F[self->nx - 1] = self->u[self->nx - 2];
}

/* ==================== 【预处理：初值剥离函数】 ==================== */
// 在这里统一管理初始流场，可以通过 type 随时切换波形
void init_convection_fields(solver* self, int type) {

    const double PI = acos(-1.0); //计算一个pi值, 产生一个关于初值条件的截断误差

    printf ("The pi we used is equal to %f\n", PI);

    for (int i = 0; i < self->nx; i++) {
        double x = i * self->dx; // 计算当前网格点的物理空间坐标

        if (type == 1) {
            // Square Wave
            if (x >= 0 && x <= 1) {
                self->u[i] = 1.0;
            } else {
                self->u[i] = 0.0;
            }

        } 
        else if (type == 2) {
            // Gauss Sin Wave
            if (x >= 0.0 && x <= 1.0) {
                double gauss_term = exp(-16.0 * pow(x - 0.5, 2));   // 高斯项
                double sin_term = sin(40.0 * PI * x);            // 正弦项
                self->u[i] = gauss_term * sin_term;
            } else {
                self->u[i] = 0.0;                               // otherwise 对应的情形
            }
        }
        else if (type == 3){
            // Dont Know Wave
            if (x >= 0.0 && x <= 1.0) {
                double poly_term = -64.0 * pow(x, 3) * pow(x - 1.0, 3); // 多项式项
                double gauss_term = exp(-16.0 * pow(x - 0.5, 2));        // 高斯项
                self->u[i] = poly_term * gauss_term;
            } else {
                self->u[i] = 0.0; // otherwise
            }
        }

        // 预处理：让 Future 数组初始时与 Present 保持一致
        self->u_F[i] = self->u[i];
    }
    printf("--> [Preprocessing] Initialize Successfully!\n");
}



int main() {
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
    printf("      1-D Linear Advection CFD Solver made    \n");
    printf("==================================================\n");

    //交互 - 选择初始波形
    int type;
    printf("\n[Step 1] Select Initial Condition:\n");
    printf("1 - Square Wave (Best for testing dispersion)\n");
    printf("2 - Gauss Sin Wave (High frequency)\n");
    printf("3 - Gauss Wave\n");
    printf("If there is another IC needed, get into the code and find \"init_convection_fields\", creat new one");
    printf("Input (1/2/3): ");
    scanf("%d", &type);

    //交互 - 选择离散格式
    int schame; //交互参数
    printf("\n[Step 2] Select Discretization Schame:\n");
    printf("1 - FTBS\n");
    printf("2 - Lax\n");
    printf("3 - Lax-Wendroff\n");
    printf("4 - Warming-Beam\n");
    printf("5 - Roe\n");
    printf("Input (1/2/3/4/5): ");
    if (scanf("%d", &schame) != 1 || schame < 1 || schame > 5) {
        printf("\n[Error] Wrong schame index input!\n");
        return 1;
    }

    //物理与网格参数输入
    double input_dx; //步长
    double input_a; //对流速度
    double input_t_max; //最长模拟时间
    double domain_buffer; //

    printf("\n[Step 3] Configure Physics & Grid parameters:\n");
    printf("-> Input Grid Spatil Advance (dx, e.g., 0.01): ");   scanf("%lf", &input_dx);
    printf("-> Input Convection Velocity (a, e.g., 1.0): "); scanf("%lf", &input_a);
    printf("-> Input Total Simulation Time (t_max, e.g., 2.0): "); scanf("%lf", &input_t_max);
    printf("-> Input Domain Buffer Factor (>=1.0, e.g., 2.5):\n");
    printf("   (Explanation: Domain Length L = a * t_max * Factor, ensures wave won't run out)\n");
    printf("   Input Factor: "); scanf("%lf", &domain_buffer);

    //实例化与参数配置
    solver my_solver; //由蓝图struct sovler /struct AdvectionSolver1D_t 实际创建储物柜
    my_solver.dx = input_dx;   // 求解域设置，
    my_solver.a  = input_a;    // 特征线速度

    double adaptive_L = fabs(my_solver.a * input_t_max) * domain_buffer;
    my_solver.nx = (int)ceil(adaptive_L / my_solver.dx) + 1;
    printf ("Domain nx = %d\n", my_solver.nx);
    

    double input_CFL;


    // 💡 CFL 核心交互接口
    printf("-> Input Target CFL Number (Courant Number, e.g., 0.5):\n");
    printf("   (Warning: For explicit scheme like FTBS, CFL must <= 1.0 to avoid severe NaN explosion!)\n");
    printf("   Input CFL: "); scanf("%lf", &input_CFL);

    double target_cfl = input_CFL;     //自动计算 dt (CFL = 0.5) 待交互
    my_solver.dt = target_cfl * my_solver.dx / my_solver.a; 

    //使用malloc动态分配内存，将野指针初始化

        /* malloc动态分配 v.s. 静态分配

        struct AdvectionSolver1D_t {
        int nx;
        double u[400]; //极其僵化的硬编码设计
        };

        ① 栈 v.s. 堆 的理解!
        ② 内存溢出 v.s. 内存覆盖
        */

    my_solver.u   = (double*)malloc(sizeof(double) * (size_t)my_solver.nx); //malloc "Memory Allocation"
    my_solver.u_F = (double*)malloc(sizeof(double) * (size_t)my_solver.nx);
    my_solver.u_B = (double*)malloc(sizeof(double) * (size_t)my_solver.nx);

        /*
        malloc语法回顾：
            malloc函数接收unsinged long long值，返回连续内存的第一个字节通用地址void* 
            (double*)                   强制类型转换，malloc返回的是 void*类型，需要类型转换为double*供编译器理解，CPU才能在内存条上正常跳步。虽然C程序理解，但C++不理解
            sizeof(double) * ...        具体的分配空间大小
            size_t                      无符号64位整型，符合malloc接收标准，这部分需要参考C的变量运算法则
            内存释放 free(my_solver.u)   由 malloc 申请的堆内存，它不会像栈内存那样在函数结束时自动销毁           
        */


        /*
        Question 1： Why not in function "init_convection_fields"?
        这是两个不同的“初始化”概念，Physic v.s. Code

        Question 2: Why not use u[nx]? -> 这是变长数组的概念
        ① 这是一个语法错误，Keyword: 编译器，机器语，高级语，程序确定唯一性原理
        ② 本质上依然是存放在栈上
        ③ C99 v.s. C++
        */

    if (my_solver.u == NULL) { //防御编程
    fprintf(stderr, "[Fatal Error] 堆内存分配失败！系统运存不足。\n");
    exit(1); // 终止程序，防止内存越界灾难
    }

    /*
    内存越界灾难：
    ①   虚拟内存哲学
    ②   高性能计算底层 or STM32单片机 or 裸机开发
    ③   数据覆盖
    
    */

    //绑定方法，伪OOP的关键步骤，桥梁！！！
    my_solver.FTBS         = method_FTBS;
    my_solver.LAX          = method_LAX;
    my_solver.LAX_WENDROFF = method_LAX_WENDROFF;
    my_solver.WARMING_BEAM = method_WARMING_BEAM;
    my_solver.ROE          = method_ROE;

    //初始化流场
    printf("\n--------------------------------------------------\n");
    init_convection_fields(&my_solver, type);

    //根据输入，动态挂载解算引擎，利用函数指针 和 switch函数
    void (*run_step)(solver*) = NULL;
    switch (schame) {
        case 1: run_step = my_solver.FTBS; 
                printf ("Now Using FTBS Schame");
        break;
        case 2: run_step = my_solver.LAX;
                printf ("Now Using LAX Schame");
        break;
        case 3: run_step = my_solver.LAX_WENDROFF;
                printf ("Now Using LAX_WENDROFF Schame");
        break;
        case 4: run_step = my_solver.WARMING_BEAM;
                printf ("Now Using WARMING_BEAM Schame");
        break;
        case 5: run_step = my_solver.ROE;
                printf ("Now Using ROE Schame");
        break;
    }

    //时间循环
    double t = 0.0; //待交互
    double t_max = input_t_max;
    int step_count = 0; // 初始化迭代步数变量，监控，＆采样精度

    printf("--> [Calculating...]\n");
    while (t < t_max) {
        run_step(&my_solver); //run_step指向我们选取的离散格式
        
        // 迭代更新应变量u
        for (int i = 0; i < my_solver.nx; i++) {
            my_solver.u[i] = my_solver.u_F[i];
        }

        t += my_solver.dt;
        step_count++;
    }
    printf("--> [Done] 计算完成！物理时间 t = %.4f, 迭代步数 = %d\n", t, step_count);

    //导出为 Tecplot ASCII 格式 (.dat)
    FILE *fp = fopen("cfd_result.dat", "w");
    if (fp != NULL) {
        fprintf(fp, "TITLE = \"1D Linear Advection Result\"\n");
        fprintf(fp, "VARIABLES = \"x\", \"u\"\n");
        fprintf(fp, "ZONE T=\"Time=%.4f\", I=%d, F=POINT\n", t, my_solver.nx);
        for (int i = 0; i < my_solver.nx; i++) {
            fprintf(fp, "%f %f\n", i * my_solver.dx, my_solver.u[i]);
        }
        fclose(fp);
        printf("--> [Output] 流场数据已成功导出至 cfd_result.dat，可直接导入 Tecplot！\n");
    } else {
        printf("--> [Error] 文件打开失败！\n");
    }

    //释放内存
    free(my_solver.u);
    free(my_solver.u_F);
    free(my_solver.u_B);

    return 0;
}