#include <stdio.h>

#include <stdlib.h>

#include <math.h>

#define GAMMA 1.4

#define XMAX 1.0
#define XMIN -1.0

// 左右行激波和稀疏波关系统一式子 教材p77 式3.6.17
double f_wave(double p_mid, double p_j, double rho_j) {
    double a_j = sqrt(GAMMA * p_j / rho_j);
    if (p_mid > p_j) { 
        double A = rho_j * a_j * sqrt((GAMMA + 1.0) / (2.0 * GAMMA) * (p_mid / p_j) + (GAMMA - 1.0) / (2.0 * GAMMA));
        return (p_mid - p_j) / A;
    } else { 
        return (2.0 * a_j / (GAMMA - 1.0)) * (pow(p_mid / p_j, (GAMMA - 1.0) / (2.0 * GAMMA)) - 1.0);
    }
}//注意到公式在p_mid = p_j 时是连续的，且函数值为0.

double sound_speed(double p, double rho) {
    return sqrt(GAMMA * p / rho);
}

double calcul_A (double p_mid, double p_j, double rho_j) 
{
    double a_j = sqrt(GAMMA * p_j / rho_j);
    double A = rho_j * a_j * sqrt((GAMMA + 1.0) / (2.0 * GAMMA) * (p_mid / p_j) + (GAMMA - 1.0) / (2.0 * GAMMA));
    return A;
}

int main() {
    system("chcp 65001 > nul");
  //初始条件
    double rho_L = 0.445, u_L = 0.698, p_L = 3.528;
    double rho_R = 0.5, u_R = 0.0, p_R = 0.571;
    double x_0 = 0.5;

    double time;

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
    printf("      1-D Riemann Solver - Initial Conditions    \n");
    printf("==================================================\n");

    printf("Step 1: Input LEFT state (rho_L  u_L  p_L)\n");
    printf("        [Separate by spaces and then press Enter]: ");
    if (scanf("%lf %lf %lf", &rho_L, &u_L, &p_L) != 3) {
        printf("\n[Error]!\n");
        return 1;
    }

    printf("\n");

    printf("Step 2: Input RIGHT state (rho_R  u_R  p_R)\n");
    printf("        [Separate by spaces and press Enter]: ");
    if (scanf("%lf %lf %lf", &rho_R, &u_R, &p_R) != 3) {
        printf("\n[Error]!\n");
        return 1;
    }

    printf("Step 3: Input Time variation\n");
    printf("        [Press Enter to continue]: ");
    if (scanf("%lf", &time) < 0){
        printf("\n[Error]!\n");
        return 0;
    }

    printf("\n==================================================\n");
    printf("Initialization successful! Lets Start solver QwQ...\n\n");

    //计算初始声速，用于后续① 判断真空条件（实际上是f_wave(0)的值） ② 求波头、波尾速度
    double a_L = sound_speed(p_L, rho_L);
    double a_R = sound_speed(p_R, rho_R);

    //真空条件判定 (Vacuum Check)
    double F_0 = f_wave(0.0, p_L, rho_L) + f_wave(0.0, p_R, rho_R); //对应p78 F(0)的值
    int is_vacuum = 0;
    if (F_0 >= u_L - u_R) {
        printf("Vacuum Generation!\n");
        is_vacuum = 1;
    }

    double p_mid = 0.0, u_mid = 0.0;
    double rho_mid_L = 0.0, rho_mid_R = 0.0;
    double a_mid_L = 0.0, a_mid_R = 0.0;

    double Z_L = 0.0, Z_R = 0.0; //特征线速度初始化

    if (!is_vacuum) {
        double p_low = 0.0;
        double p_high = fmax(p_L, p_R); 

        while (f_wave(p_high, p_L, rho_L) + f_wave(p_high, p_R, rho_R) + (u_R - u_L) < 0) {
            p_high *= 2.0;
        }

        //二分法找根
        double tol = 1e-8;
        while ((p_high - p_low) > tol) {
            p_mid = 0.5 * (p_low + p_high);
            double F_mid = f_wave(p_mid, p_L, rho_L) + f_wave(p_mid, p_R, rho_R) + (u_R - u_L);
            if (F_mid > 0) p_high = p_mid;
            else p_low = p_mid;
        }
        printf("The solution of p_mid is: %f \n", p_mid);

        //计算中间区间断解的速度和密度 
        u_mid = 0.5 * (u_L + u_R) + 0.5 * (f_wave(p_mid, p_R, rho_R) - f_wave(p_mid, p_L, rho_L));

        printf("The solution of u_mid is: %f \n", u_mid);

        /*
        截至目前，我们已经得到了中间区域的pressure p_mid 和 velocity u_mid，接下来我们需要根据p_mid和u_mid来判断中间区域的密度 rho_mid_L 和 rho_mid_R 的计算方式（是激波还是稀疏波），并计算出它们的数值。
        */
        if (p_mid > p_L) {
            double A_L = calcul_A(p_mid, p_L, rho_L);
            rho_mid_L = rho_L * A_L  / (A_L - rho_L * (u_L - u_mid)); //根据教材p78 3.6.27 来计算中间区的密度
            printf("Left is shock wave, rho_mid_L is: %f \n", rho_mid_L);
            Z_L = u_L - A_L / rho_L; //根据教材p79 3.6.26 来计算特征线的速度
            //虽然求解解的结构不需要Z_1的数值，但我们可以利用Z_1自适应地调整x轴采样宽度
        }
        else {
            a_mid_L = a_L + (GAMMA - 1.0) / 2.0 * (u_L - u_mid); 
            rho_mid_L = GAMMA*p_mid / (a_mid_L * a_mid_L); // p78 3.6.29a
            printf("Left is expansion wave, rho_mid_L is: %f \n", rho_mid_L);
            Z_L = u_L - a_L;
        } 

        if (p_mid > p_R) {
            double A_R = calcul_A(p_mid, p_R, rho_R);
            rho_mid_R = rho_R * A_R  / (A_R + rho_R * (u_R - u_mid)); //根据教材p78 3.6.27 来计算中间区的密度
            printf("Right is shock wave, rho_mid_R is: %f \n", rho_mid_R);
            Z_R = u_R + A_R / rho_R;
        }
        else {
            a_mid_R = a_R - (GAMMA - 1.0) / 2.0 * (u_R - u_mid);
            rho_mid_R = GAMMA*p_mid / (a_mid_R * a_mid_R); // p78 3.6.29a
            printf("Right is expansion wave, rho_mid_R is: %f \n", rho_mid_R);
            Z_R = u_R + a_R;
        }
    }

    if (Z_R - Z_L < 0.1) {
        printf("Warning! The wave structure is very narrow, please adjust the initial conditions to ensure a more clear wave structure for better visualization and analysis.\n");
    }

    double t = time;
    double x_min, x_max; 
    x_min = XMIN - (Z_R - Z_L)* t * 1.2;
    printf ("Z_R-Z_L = %f\n", Z_R - Z_L);
    printf ("x_min = %f\n", x_min);
    x_max = XMAX + (Z_R - Z_L)* t * 1.2; //这样取是为了确保能够采样到完整的解结构，是自适应思想的朴素形态
    printf ("x_max = %f\n", x_max);
    double dx = 0.002;
    int N_points = (int)((x_max - x_min) / dx) + 1; //采用int类型来确保N_points是一个整数，避免由于浮点数计算误差导致的采样点数不准确问题。

    FILE *fp = fopen("riemann_exact_tecplot.dat", "w"); 
    if (fp == NULL) {
        printf("无法创建输出文件！\n");
        return 1;
    }
    fprintf(fp, "TITLE = \"Correct 1D Riemann Exact Solution\"\n"); 
    fprintf(fp, "VARIABLES = \"X\", \"Density\", \"Velocity\", \"Pressure\"\n"); 
    fprintf(fp, "ZONE T=\"Exact\", I=%d, F=POINT\n", N_points); 

    //特征线采样循环 (整数循环保证 N_points 严格对应表头)
    for (int i = 0; i < N_points; i++) {
        double x = x_min + i * dx; // 反推精准 x 坐标
        double rho_out = 0, u_out = 0, p_out = 0;
        double xi = (x - x_0) / t; // 由物理坐标反推参考坐标 xi ,不用t - t_0是因为我们的物理建模默认t_0 = 0

        if (is_vacuum) {
            //真空特解
            double Z_L_tail = u_L + 2.0 * a_L / (GAMMA - 1.0); //教材 p81 3.6.42
            double Z_R_tail = u_R - 2.0 * a_R / (GAMMA - 1.0);
            
            //其他逻辑与两侧都是稀疏波的情况相同
            double Z_L_head = u_L - a_L;
            double Z_R_head = u_R + a_R;   

            if (xi <= Z_L_head) { //左未扰动，注意到参考坐标的定义已经除以了时间t，所以这里直接比较xi和Z_L_head即可
                rho_out = rho_L; u_out = u_L; p_out = p_L;

            } else if (xi > Z_L_head && xi < Z_L_tail) { // 左稀疏区
                u_out = (2.0 / (GAMMA + 1.0)) * (a_L + (GAMMA - 1.0) / 2.0 * u_L + xi); // 公式由特征线理论得来，详情见笔记
                double a_current = u_out - xi;
                p_out = p_L * pow(a_current / a_L, 2.0 * GAMMA / (GAMMA - 1.0));
                rho_out = GAMMA * p_out / (a_current * a_current);

            } else if (xi >= Z_L_tail && xi <= Z_R_tail) { // 真空
                rho_out = 0.0; u_out = 0.0; p_out = 0.0;
                
            } else if (xi > Z_R_tail && xi < Z_R_head) { // 右稀疏
                u_out = (2.0 / (GAMMA + 1.0)) * (-a_R + (GAMMA - 1.0) / 2.0 * u_R + xi);
                double a_current = xi - u_out;
                p_out = p_R * pow(a_current / a_R, (2.0 * GAMMA) / (GAMMA - 1.0));
                rho_out = GAMMA * p_out / (a_current * a_current);

            } else { // 右未扰动
                rho_out = rho_R; u_out = u_R; p_out = p_R;
            } 
        } 

        else {
            //正常中间区解
            if (xi < u_mid) {
                //接触间断左侧
                if (p_mid < p_L) { // 左侧稀疏波
                    double a_star_L = a_L + (GAMMA - 1.0) / 2.0 * (u_L - u_mid); //教材p80 3.6.29b
                    double Z_L_head = u_L - a_L; //这里重新算了一遍，虽然之前在求解p_mid时已经判断过了，更好的做法是把这个结果保存下来，避免重复计算，但目前能跑通就行，后续再优化
                    double Z_L_tail = u_mid - a_star_L;
                    if (xi < Z_L_head) {
                        rho_out = rho_L; u_out = u_L; p_out = p_L;
                    } else if (xi >= Z_L_head && xi < Z_L_tail) { //系数波区
                        u_out = (2.0 / (GAMMA + 1.0)) * (a_L + (GAMMA - 1.0) / 2.0 * u_L + xi);
                        double a_current = u_out - xi; // 因为左行特征线满足 u - a = xi 
                        p_out = p_L * pow(a_current / a_L, (2.0 * GAMMA) / (GAMMA - 1.0));
                        rho_out = GAMMA * p_out / (a_current * a_current);
                    } else {
                        rho_out = rho_mid_L; 
                        u_out = u_mid; 
                        p_out = p_mid;
                    }
                } else { // 左侧激波
                    double A_L = calcul_A(p_mid, p_L, rho_L);
                    double Z_L_shock = u_L - A_L / rho_L;
                    if (xi < Z_L_shock) {
                        rho_out = rho_L; u_out = u_L; p_out = p_L;
                    } else {
                        rho_out = rho_mid_L; u_out = u_mid; p_out = p_mid;
                    }
                }
            } else {
                //接触间断右侧
                if (p_mid > p_R) { //右侧激波
                    double A_R = calcul_A(p_mid, p_R, rho_R);
                    double Z_R_shock = u_R + A_R / rho_R;
                    if (xi >= Z_R_shock) {
                        rho_out = rho_R; 
                        u_out = u_R; 
                        p_out = p_R;
                    } else {
                        rho_out = rho_mid_R; u_out = u_mid; p_out = p_mid;
                    }
                } else { //右侧稀疏波
                    double a_star_R = a_R - (GAMMA - 1.0) / 2.0 * (u_R - u_mid); //教材p80 3.6.37a
                    double Z_R_head = u_R + a_R;
                    double Z_R_tail = u_mid + a_star_R;
                    
                    if (xi >= Z_R_head) { 
                        rho_out = rho_R; u_out = u_R; p_out = p_R;
                    } else if (xi > Z_R_tail && xi < Z_R_head) {
                        u_out = (2.0 / (GAMMA + 1.0)) * (-a_R + (GAMMA - 1.0) / 2.0 * u_R + xi);
                        double a_current = xi - u_out; 
                        p_out = p_R * pow(a_current / a_R, (2.0 * GAMMA) / (GAMMA - 1.0));
                        rho_out = GAMMA * p_out / (a_current * a_current);
                    } else {
                        rho_out = rho_mid_R; u_out = u_mid; p_out = p_mid;
                    }
                }
            }
        }
        //写入数据行
        fprintf(fp, "%f\t%f\t%f\t%f\n", x, rho_out, u_out, p_out);
    }

    fclose(fp);
    printf("Tecplot data generated successfully!\n");
    if (!is_vacuum) {
        printf("Validation -> p_mid: %f, u_mid: %f\n", p_mid, u_mid);
    }
    return 0;
}

/*
值得优化的点：
①用更优秀的迭代方案（例如牛顿迭代法）
②将物理量和方法封装为结构体，优化代码结构
*/

