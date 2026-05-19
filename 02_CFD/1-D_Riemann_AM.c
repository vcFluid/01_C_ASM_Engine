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
double rho_1 = 1.0;
double u_1 = 0.0;   //注意: u_1 和 u_2 都是0，但u_1 和 u_2 的初始值也可以是非零的，我们这里根据教材，选择了静止的初始状态，是为了简化问题？
double p_1 = 1.0;

double rho_2 = 0.125; 
double u_2 = 0.0;     
double p_2 = 0.1;     

//定义超越方程的表达式 教材p77 式3.6.17
double f_1(double px, double p_j, double rho_j) { //虽然已经定义了全局变量，但我们可以仍然完整罗列出函数的参数，保持函数的独立性和通用性，一个是良好编程习惯，另一个是便于main函数统一求解五种状态的解，而不用担心全局变量的干扰。
    double a = sqrt(GAMMA * p_j / rho_j); //声速
    double A = (GAMMA + 1.0) / (2.0 * GAMMA);
    double B = (GAMMA - 1.0) / (2.0 * GAMMA);
    double P = px / p_j; //压力比

    double result_Shockwave, result_ExpansionWave;
    result_Shockwave = (px - p_j) / (rho_j * a * sqrt(A*P + B));
    result_ExpansionWave = 2.0 * a / (GAMMA - 1.0) * (pow(P, (GAMMA - 1.0) / (2.0 * GAMMA)) - 1.0);

    if (P > 1.0) { 
        return result_Shockwave;
    } else { 
        return result_ExpansionWave;
    }
}

int main(){
    //我们首先需要求解一个超越方程，形式为 f_1(px) = 0，其中 px 是我们要求解的未知数，代表面两侧的压力。
    //可以使用二分法或者牛顿法来求解这个方程。这里选择二分法，因为它比较简单且稳定。
    double px_left = 0.01; //压力的下界
    double px_right = 10.0; //压力的上界
    double dpx = 1e-6; //精度
    double px_mid;
        while ((px_right - px_left) > dpx) {
            px_mid = (px_left + px_right) / 2.0; //计算中点
            double f_mid = f_1(px_mid, p_1, rho_1) - f_1(px_mid, p_2, rho_2); //计算f(px_mid)

            if (f_1(px_left, p_1, rho_1) * f_mid < 0)
                px_right = px_mid;
            else
                px_left = px_mid;
        };
        printf("The solution of pressure is: %f ", px_mid / p_1);
    return 0;

    //然后需要利用求解得到的 px 来计算过渡状态的速度 u* 和密度 rho*



    //由于我们得到的是间断速度，还需要通过特征线理论来分析解的结构，进而得到整个解的分布情况（如激波、稀疏波等）

}