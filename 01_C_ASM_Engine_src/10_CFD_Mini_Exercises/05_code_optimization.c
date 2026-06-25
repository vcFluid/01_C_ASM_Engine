#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* 掌握数值计算中最基础也最重要的**“Epsilon Check”（容差检测）**。 */

#define ERROR 1e-7 //精度

double error = -0.000000000000000005; //由具体情况决定

int main(void) {
if (fabs(error) < ERROR)
    {
        printf("OKAY\n");
    }
    else
    {
        for(int step = 0; step < 100; step++)
        {
            double k;
            k = (step > 50) ? 1.0 : 0.5;
            printf("Step = %d, k = %.2f\n", step, k);
        }
    }
}