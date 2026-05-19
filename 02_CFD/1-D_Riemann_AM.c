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

    double result_Shockwave, result_ExpansionWave;
    result_Shockwave = (p_mid - p_j) / (rho_j * a * sqrt(A*P + B));
    result_ExpansionWave = 2.0 * a / (GAMMA - 1.0) * (pow(P, (GAMMA - 1.0) / (2.0 * GAMMA)) - 1.0);

    if (P > 1.0) { 
        return result_Shockwave;
    } else { 
        return result_ExpansionWave;
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

    return u_x1, u_x2;
}

int main(){
    //由于教材中在判断解的状态时，假设了p1 <= p2，所以需要先用一个排序算法来确定状态1和状态2，确保p_1 <= p_2
    double p_1, p_2, u_1, u_2, rho_1, rho_2;
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


    //我们首先需要求解一个超越方程，形式为u_1 - u_2 = f_1(p_middle, p_1, rho_1) - f_1(p_middle, p_2, rho_2) = 0，其中 p_middle 是我们要求解的未知数，代表面两侧的压力。 教材p77 3.6.18
    double p_mid; //待求量

    double F2 = f_1(p_2, p_1, rho_1) + f_1(p_2, p_2, rho_2); //用于依照教材方法判断激波or系数波；这里严格满足教材的假设，即p_1 <= p_2，所以直接调用我们封装好的函数，并将p_2作为f_1函数的输入，来判断解的结构（激波还是稀疏波）
    double F1 = f_1(p_1, p_2, rho_2) + f_1(p_1, p_1, rho_1); //同上
    double F0 = f_1(0, p_1, rho_1) + f_1(0, p_2, rho_2); //用于判断解的结构是激波还是稀疏波；这里我们直接调用f_1函数，并将p_1作为输入，来判断解的结构（激波还是稀疏波）
    printf("The value of F2 is: %f ", F2);
    printf("The value of F1 is: %f ", F1);
    printf("The value of F0 is: %f ", F0);
    if (u_1 - u_2 > F2 || u_1 - u_2 == F2) {   
        printf("Both sides are shock waves.\n");
        p_mid = p_2;
    }
    else if (F2 > u_1 - u_2 &&(u_1 - u_2 > F1 || u_1 - u_2 == F1)) {
        printf("One side is shock wave, the other side is expansion wave.\n");
    }
    else if (F1 > u_1 - u_2 && (u_1 - u_2 > F0 || u_1 - u_2 == F0)) {
        printf("Both sides are expansion waves.\n");
    }
    else if (F0 > u_1 - u_2) {
        printf("The solution is vacuum.\n");
    }
    else{
        printf("The solution is unphysical.Maybe the error is in the judgment logic.\n");
    }
    

    //然后需要利用求解得到的 p_mid 来计算过渡状态的速度 u_X 和密度 rho_x

    double u_mid, rho_mid;
    u_mid = 1/2.0 * (u_1 + u_2 - f_1(p_mid, p_1, rho_1) + f_1(p_mid, p_2, rho_2)); //根据教材p79 式3.6.26
    printf("The solution of mid-velocity is: %f ", u_mid);
    //由于我们得到的是间断速度，还需要通过特征线理论来分析解的结构，进而得到整个解的分布情况（如激波、稀疏波等）

    return 0;
}

