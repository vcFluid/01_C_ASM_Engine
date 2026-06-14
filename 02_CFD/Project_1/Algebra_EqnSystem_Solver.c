#include <stdio.h>
#include <stdlib.h>

// 范式：定义一个专门的矩阵求解函数
// 参数包括：矩阵维度 N，系数矩阵 A，未知数/结果向量 x，右端项 b
int solve_linear_system(int N, const double *A, double *x, const double *b) {
    // 1. 复制 A 和 b 的数据，因为消元过程会破坏原始数据
    double *A_tmp = (double*)malloc(sizeof(double) * (size_t)N * (size_t)N);
    for(int i = 0; i < N * N; i++) A_tmp[i] = A[i];
    for(int i = 0; i < N; i++) x[i] = b[i]; // 将 b 复制到结果向量 x 中进行原位消元

    // 2. 前向消元 (Forward Elimination) -> 将矩阵化为上三角矩阵
    for (int k = 0; k < N - 1; k++) {
        // 主元检查（防止除以 0，实际工程中需要进行主元提取 Pivot）
        if (A_tmp[k * N + k] == 0.0) {
            free(A_tmp);
            return -1; // 求解失败，矩阵奇异
        }
        for (int i = k + 1; i < N; i++) {
            double factor = A_tmp[i * N + k] / A_tmp[k * N + k];
            for (int j = k; j < N; j++) {
                A_tmp[i * N + j] -= factor * A_tmp[k * N + j];
            }
            x[i] -= factor * x[k]; // 同时更新右端项
        }
    }

    // 3. 回代求解 (Back Substitution) -> 从最后一行倒着算出所有未知数
    for (int i = N - 1; i >= 0; i--) {
        for (int j = i + 1; j < N; j++) {
            x[i] -= A_tmp[i * N + j] * x[j];
        }
        x[i] /= A_tmp[i * N + i];
    }

    free(A_tmp);
    return 0; // 求解成功
}

int main() {
    int N = 3; // 3阶方程组
    
    // 物理/数学定义好矩阵 A 和向量 b
    double A[] = {
        2,  1, -1,
       -3, -1,  2,
       -2,  1,  2
    };
    double b[] = {8, -11, -3};
    double *x = (double*)malloc(sizeof(double) * (size_t)N); // 存储解

    // 调用求解范式
    if (solve_linear_system(N, A, x, b) == 0) {
        printf("方程组的解为:\n");
        for(int i = 0; i < N; i++) printf("x[%d] = %f\n", i, x[i]);
    } else {
        printf("矩阵奇异，无唯一解。\n");
    }

    free(x);
    return 0;
}