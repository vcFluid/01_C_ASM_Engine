/* 半解析方案 */
/*

由于这种方案我们是求解通过特征线理论分析Riemann 1D Problem得到的超越方程（相应解是一个半解析的稳态解），不是数值格式的离散方程，所以我们不需要考虑数值格式的稳定性问题（如CFL条件），
也不需要考虑数值耗散和数值色散等问题。我们只需要根据初始条件，关注相应超越方程的求解过程，确保求解器能够正确地找到满足方程的解即可.
从本质上来讲，这种方案更像在找一个方程的根，而非在进行数值模拟. 所以关键在于 **数值求解一个超越方程**

此外，这个超越方程描述的是稳态解，所以还需要特征线理论的分析来确定过渡状态的解，才能正确地构建整个解的结构（如激波、稀疏波等）。
因此，虽然我们不需要担心数值格式的稳定性，但我们需要确保求解器能够正确地处理超越方程的非线性特征，并且能够找到正确的根来描述物理现象。

*/

#include <stdio.h>

#include <stdlib.h>

#include <math.h>

#include <string.h>

//物理常数
#define GAMMA 1.4 //比热比

//初始状态(t=0)
double rho_L = 1.0;
double u_L = 0.0;   //注意: u_1 和 u_2 都是0，但u_1 和 u_2 的初始值也可以是非零的，我们这里根据教材，选择了静止的初始状态，是为了简化问题（实际上我的程序也能求解非零情况~~~~~~）
double p_L = 1.0;

double rho_R = 0.125; 
double u_R = 0.0;     
double p_R = 0.1;     

//注意到main中应该用一个排序算法确定 状态1和 状态2！！！！！

//定义超越方程的表达式 教材p77 式3.6.17
double f_1(double p_mid, double p_j, double rho_j) { //虽然已经定义了全局变量，但我们可以仍然完整罗列出函数的参数，保持函数的独立性和通用性，一个是良好编程习惯，另一个是便于main函数统一求解五种状态的解，而不用担心全局变量的干扰。
    double a = sqrt(GAMMA * p_j / rho_j); //声速
    double A = (GAMMA + 1.0) / (2.0 * GAMMA);
    double B = (GAMMA - 1.0) / (2.0 * GAMMA);
    double P = p_mid / p_j; //压力比
    double result_Shockwave;

    if (P > 1.0 || P == 1.0) { 
        return result_Shockwave = (p_mid - p_j) / (rho_j * a * sqrt(A*P + B));
    } else { 
        return -11; // P < 1.0时，返回-1，表示此时输入与函数类型不匹配
    }
}

double f_2(double p_mid, double p_j, double rho_j) {
    double a = sqrt(GAMMA * p_j / rho_j); //声速
    double P = p_mid / p_j; //压力比
    double result_ExpansionWave;

    if (P < 1.0 || P == 1.0) { 
        return result_ExpansionWave = 2.0 * a / (GAMMA - 1.0) * (pow(P, (GAMMA - 1.0) / (2.0 * GAMMA)) - 1.0);
    } else { 
        return -12; // P > 1.0时，返回-2，表示此时输入与函数类型不匹配
    }
}

//定义不同波区的解的结构求解C语言函数
//此时相当于得到了两个边值条件，如何求解流场内部物理场的分布 参考ppt Lesson02 p76
double field_constant(double p_mid, double u_1, double rho_1, double p_1, double u_2, double rho_2, double p_2)//常数状态区域的求解 常数状态区,alpha恒等于alpha_0, beta恒等于beta_0,所以我们可以直接通过初始状态的alpha和beta来求解常数状态区的物理量分布
{
    double alpha_1 = u_1 + 2/(GAMMA - 1.0) * sqrt(GAMMA * p_1 / rho_1);
    double beta_1 = u_1 - 2/(GAMMA - 1.0) * sqrt(GAMMA * p_1 / rho_1);

    double alpha_2 = u_2 + 2/(GAMMA - 1.0) * sqrt(GAMMA * p_2 / rho_2);
    double beta_2 = u_2 - 2/(GAMMA - 1.0) * sqrt(GAMMA * p_2 / rho_2);  

    double u_x1 = 0.5 * (alpha_1 + beta_1);
    double u_x2 = 0.5 * (alpha_2 + beta_2);

    double c_x1 = (GAMMA-1)/4.0 * (alpha_1 - beta_1);
    double c_x2 = (GAMMA-1)/4.0 * (alpha_2 - beta_2);

    return u_x1, u_x2; //Warning! C语言函数只能返回一个值，如果需要返回多个值，可以通过指针参数来实现，或者定义一个结构体来封装多个返回值。这里我们可以定义一个结构体来封装u_x1和u_x2，然后返回这个结构体的实例。
}

int main(){
    double p_1, p_2, u_1, u_2, rho_1, rho_2; //已知参数，尽管我们已经定义了全局变量，但我们仍然在main函数中定义了局部变量来存储状态1和状态2的物理量，这样做是为了保持代码的清晰和可维护性，避免直接使用全局变量可能带来的混淆和错误。
    double p_mid = 0.0; //待求量，赋值为0.0只是为了初始化，注意后续代码需要更新p_mid!
    //由于教材中在判断解的状态时，假设了p1 <= p2，所以需要先用一个排序算法来确定状态1和状态2，确保p_1 <= p_2
    if (p_L <= p_R) {
        p_1 = p_L;
        u_1 = u_L;
        rho_1 = rho_L;

        p_2 = p_R;
        u_2 = u_R;
        rho_2 = rho_R;
    } else {
        p_1 = p_R;
        u_1 = u_R;
        rho_1 = rho_R;

        p_2 = p_L;
        u_2 = u_L;
        rho_2 = rho_L;
    }

    //定义一些中间变量来存储声速等物理量
    double c_1 = sqrt(GAMMA * p_1 / rho_1);
    double c_2 = sqrt(GAMMA * p_2 / rho_2);

    double A_1 = rho_1 * c_1 * sqrt((GAMMA + 1.0) / (2.0 * GAMMA) * (p_mid / p_1) + (GAMMA - 1.0) / (2.0 * GAMMA)); //A的定义参照教材p76末尾
    double A_2 = rho_2 * c_2 * sqrt((GAMMA + 1.0) / (2.0 * GAMMA) * (p_mid / p_2) + (GAMMA - 1.0) / (2.0 * GAMMA));

    //我们首先需要求解一个超越方程，形式为u_1 - u_2 = f_1(p_middle, p_1, rho_1) - f_1(p_middle, p_2, rho_2) = 0，其中 p_middle 是我们要求解的未知数，代表面两侧的压力。 教材p77 3.6.18
    double F2 = f_1(p_2, p_1, rho_1); //用于依照教材方法判断激波or系数波；这里严格满足教材的假设，即p_1 <= p_2，所以直接调用我们封装好的函数，并将p_2作为函数的输入
    if (F2 == -11)
    {
        printf("Warning: The input for f_1 when calculating F2 is wrong!");
    }
    
    double F1 = f_2(p_1, p_2, rho_2); //同上
    if (F1 == -12)
    {
        printf("Warning: The input for f_2 when calculating F1 is wrong!");
    }

    double F0 = f_2(0, p_1, rho_1) + f_2(0, p_2, rho_2); //由于0必然小于p_1和p_2，所以直接调用f_2函数，并将0作为输入，来判断解的结构（激波还是稀疏波）
    printf("The value of F2 is: %f ", F2);
    printf("The value of F1 is: %f ", F1);
    printf("The value of F0 is: %f ", F0);

    int solution_type; //1: both sides are shock waves; 2: one side is shock wave, the other side is expansion wave; 3: both sides are expansion waves; 4: vacuum;

    if (u_1 - u_2 > F2 || u_1 - u_2 == F2) {   
        printf("Both sides are shock waves.\n");
        //利用二分法求解超越方程，来得到p_mid的数值解
        double tol = 1e-6; //精度
        double p_low = p_1; //下界
        double p_high = p_2; //上界
        while (p_high - p_low > tol) {
            p_mid = 0.5 * (p_low + p_high);
            double F_mid = f_1(p_mid, p_1, rho_1) + f_1(p_mid, p_2, rho_2) - (u_1 - u_2);
            if (F_mid > 0) {
                p_high = p_mid;
            } else {
                p_low = p_mid;
            }
        };
        solution_type = 1;
    }
    else if (F2 > u_1 - u_2 &&(u_1 - u_2 > F1 || u_1 - u_2 == F1)) {
        printf("1(which is the left side when the input p_L < p_R) side is shock wave, 2 is expansion wave.\n");
        //同理
        double tol = 1e-6; //精度
        double p_low = 0; //下界
        double p_high = p_2; //上界
        while (p_high - p_low > tol) {
            p_mid = 0.5 * (p_low + p_high);
            double F_mid = f_1(p_mid, p_1, rho_1) + f_2(p_mid, p_2, rho_2) - (u_1 - u_2);
            if (F_mid > 0) {
                p_high = p_mid;
            } else {
                p_low = p_mid;
            }
        };
        solution_type = 2;
    }
    else if (F1 > u_1 - u_2 && (u_1 - u_2 > F0 || u_1 - u_2 == F0)) {
        printf("Both sides are expansion waves.\n");
        //同理
        double tol = 1e-6; //精度
        double p_low = 0; //下界
        double p_high = p_1; //上界
        while (p_high - p_low > tol) {
            p_mid = 0.5 * (p_low + p_high);
            double F_mid = f_2(p_mid, p_1, rho_1) + f_2(p_mid, p_2, rho_2) - (u_1 - u_2);
            if (F_mid > 0) {
                p_high = p_mid;
            } else {
                p_low = p_mid;
            }
        };
        solution_type = 3;
    }
    else if (F0 > u_1 - u_2) {
        printf("The solution is vacuum.\n");
        //在这种情况下，p_mid的值没有物理意义，因为解是一个真空状态，在后续程序应该单独讨论这种情况，来构建相应的解的结构（两侧都是稀疏波，中间是一个真空区）
        solution_type = 4;

    }
    else{
        printf("The solution is unphysical.Maybe the error is in the judgment logic.\n");
    }
    

    //然后需要利用求解得到的 p_mid 来计算过渡状态的速度 u_X 和密度 rho_x, 然后通过特征线理论来分析解的结构，进而得到整个解的分布情况（如激波、稀疏波等)

    double u_mid, rho_mid_1, rho_mid_2;
    if (solution_type == 1) {
        u_mid = 1/2.0 * (u_1 + u_2 - f_1(p_mid, p_1, rho_1) + f_1(p_mid, p_2, rho_2)); //根据教材p79 式3.6.26
        rho_mid_1 = rho_1*A_1 / (A_1 - rho_1*(u_1-u_mid)); //根据教材p79  式3.6.27
        rho_mid_2 = rho_2*A_2 / (A_2 - rho_2*(u_2-u_mid)); //根据教材p79  式3.6.27

        double Z_1 = u_1 - A_1 / rho_1; 
        double Z_2 = u_2 + A_2 / rho_2;

    } else if (solution_type == 2) {
        u_mid = 1/2.0 * (u_1 + u_2 - f_1(p_mid, p_1, rho_1) + f_2(p_mid, p_2, rho_2)); //根据教材p79 式3.6.26
        double c_star_2 = c_2 - (GAMMA - 1.0) / 2.0 * (u_2 - u_mid); 
        rho_mid_1 = rho_1*A_1 / (A_1 - rho_1*(u_1-u_mid)); //根据教材p79  式3.6.27
        rho_mid_2 = GAMMA * p_mid / (c_star_2 * c_star_2); //根据教材p79  式3.6.29b 

        double Z_1 = u_1 - A_1 / rho_1;

        double Z_2_head = u_2 + c_2; //波头速度
        double Z_2_tail = u_mid + c_star_2; //波尾速度

        // in the expansion wave region between Z_2_head and Z_2_tail
        double c = ((GAMMA - 1.0)/(GAMMA + 1.0) * (u_1 - (x / t)) + 2*c_1/(GAMMA + 1.0)); //根据教材p79  式3.6.32a
        double u_x = x / t + c; //特征线速度
        double p_x = p_1 * (c / c_1);
        double rho_x = GAMMA * p_x / (c * c);

    } else if (solution_type == 3) {
        u_mid = 1/2.0 * (u_1 + u_2 - f_2(p_mid, p_1, rho_1) + f_2(p_mid, p_2, rho_2)); 

        double c_star_1 = c_1 + (GAMMA - 1.0) / 2.0 * (u_1 - u_mid); 
        double c_star_2 = c_2 - (GAMMA - 1.0) / 2.0 * (u_2 - u_mid); 

        rho_mid_1 = GAMMA * p_mid / (c_star_1 * c_star_1); 
        rho_mid_2 = GAMMA * p_mid / (c_star_2 * c_star_2); 

        double Z_1_head = u_1 - c_1; //波头速度
        double Z_1_tail = u_mid - c_star_1; //波尾速度

        double Z_2_head = u_2 + c_2; //波头速度
        double Z_2_tail = u_mid + c_star_2; //波尾速度

        // in the expansion wave region between Z_1_head and Z_1_tail
        double c_x1 = ((GAMMA - 1.0)/(GAMMA + 1.0) * (u_1 - (x / t)) + 2*c_1/(GAMMA + 1.0)); //根据教材p79  式3.6.32a
        double u_x1 = x / t + c_x1;
        double p_x1 = p_1 * pow((c_x1 / c_1), 2 * GAMMA / (GAMMA - 1));
        double rho_x1 = GAMMA * p_x1 / (c_x1 * c_x1);

        // in the expansion wave region between Z_2_head and Z_2_tail
        double c_x2 = ((GAMMA - 1.0)/(GAMMA + 1.0) * ((x / t) - u_2) + 2*c_2/(GAMMA + 1.0)); //根据教材p79  式3.6.32a
        double u_x2 = x / t - c_x2; //特征线速度
        double p_x2 = p_2 * pow((c_x2 / c_2), 2 * GAMMA / (GAMMA - 1));
        double rho_x2 = GAMMA * p_x2 / (c_x2 * c_x2);



    } else if (solution_type == 4) {
        //在真空状态下，过渡状态的速度和密度没有物理意义，可以保守地将它们设置为0或者其他特殊值，在后续程序中单独讨论这种情况，来构建相应的解的结构（两侧都是稀疏波，中间是一个真空区）
        u_mid = 0;
        double Z_1_tail = u_1 + 2*c_1/(GAMMA - 1.0);
        double Z_2_tail = u_2 - 2*c_2/(GAMMA - 1.0);

        double Z_1_head = u_1 - c_1; //波头速度
        double Z_2_head = u_2 + c_2; //波头速度

        // in the expansion wave region between Z_1_head and Z_1_tail
            double c_x1 = ((GAMMA - 1.0)/(GAMMA + 1.0) * (u_1 - (x / t)) + 2*c_1/(GAMMA + 1.0)); //根据教材p79  式3.6.32a
            double u_x1 = x / t + c_x1;
            double p_x1 = p_1 * pow((c_x1 / c_1), 2 * GAMMA / (GAMMA - 1));
            double rho_x1 = GAMMA * p_x1 / (c_x1 * c_x1);

        // in the expansion wave region between Z_2_head and Z_2_tail
            double c_x2 = ((GAMMA - 1.0)/(GAMMA + 1.0) * ((x / t) - u_2) + 2*c_2/(GAMMA + 1.0)); //根据教材p79  式3.6.32a
            double u_x2 = x / t - c_x2; //特征线速度
            double p_x2 = p_2 * pow((c_x2 / c_2), 2 * GAMMA / (GAMMA - 1));
            double rho_x2 = GAMMA * p_x2 / (c_x2 * c_x2);
    }
    printf("The solution of mid-velocity is: %f ", u_mid);

    //其实更优秀的编程方案是分别分析左侧和右侧的解的结构再组装，这样代码的可读性和可维护性会更好一些，当然也可以通过定义一些函数来封装不同波区的解的结构分析，这样可以使得main函数更加简洁和清晰。
    //但这样就失去了一定的物理意义，注意实际流动中，上下游的扰动可能会对解的结构产生影响，所以我采取了保守的方案，尽管代码可能会有一些冗余，但它更好地反映了物理现象的复杂性和多样性。


    //待做：将上述函数功能剥出一个自定义函数，然后在main函数中调用，这样可以使得main函数更加简洁和清晰，同时也更好地体现函数的独立性和通用性。
    //而且由于我们需要求解任意时刻的解，所以将上述功能封装成一个函数是非常有必要的，这样我们就可以通过调用这个函数，并传入不同的时间参数，来得到任意时刻的解的分布情况。
    //这样也不会导致复杂的嵌套，因为我们可以通过合理的函数设计和参数传递，来保持代码的清晰和可维护性。

    return 0;
}

