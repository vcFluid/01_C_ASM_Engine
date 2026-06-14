#include <stdio.h>

#include <stdlib.h>

#include <math.h>

#include <string.h>


typedef struct Riemann_1D_MacC_solver solver;

/*
守恒量Q = [rho, rho*u, rho*E]
通量F = [rho*u, rho*u^2 + p, rho*u*E + p*u]

这里有两个变量储存的思路
第一个是将Q、F中的变量都作为未知数，求解的最后再反解出rho、p、u、E
另一个是直接将rho、p、u、E作为未知数，代入方程中求解

对于程序而言，两种思路是否有差别？
猜测：第一种方案更好，因为MacCormack有一个预估步，用第一种方法非常丝滑，第二种方法相当于每一步都需要
        而且用第一种方法，只需要储存3x3个变量，节约内存，最后输出只需要写一个简单的数学运算
        3x3 是因为看离散方程，每一个变量除了当前量，下一时刻量以外还有一个预估步

*/

struct Riemann_1D_MacC_solver {
    double *F;       //待定的用于计算人工粘性的物理量（从密度、速度、压强中选），后续在main函数中确定（只需要在每一步执行一个赋值就行）
    double *q1;   //守恒量_1 rho*u
    double *q1_F; //前插
    double *q1_Maybe; //预估步


    double *q2; //守恒量_2 rho*u^2 + p
    double *q2_F; //前插
    double *q2_Maybe; //预估步

    double *q3;   //守恒量_3 rho*u*E + p*u
    double *q3_F; //前插
    double *q3_Maybe; //预估步


};