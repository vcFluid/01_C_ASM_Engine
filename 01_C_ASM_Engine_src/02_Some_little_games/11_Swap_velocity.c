#include <stdio.h>
#include <stdlib.h>

double v_1 = 5.0;
double v_2 = 10.0;

int swap_velocity(double *a, double *b)
{
    double temp;
    temp = *a;
    *a = *b;
    *b = temp;
    return 0;
}

int main(void)
{
    printf("Before swapping: v_1 = %.2f, v_2 = %.2f\n", v_1, v_2);
    swap_velocity(&v_1, &v_2);
    printf("After swapping: v_1 = %.2f, v_2 = %.2f\n", v_1, v_2);
    system("pause");
    return 0;
}