#include <stdio.h>
#define swap(t, a, b) { \
    t temp = a;     \
    a = b;          \
    b = temp;       \
}

int main(void){
    int x = 5;
    int y = 10;
    printf("Before swap: x = %d, y = %d\n", x, y);
    swap(int, x, y);
    printf("After swap: x = %d, y = %d\n", x, y);

    double p = 1.5;
    double q = 2.5;
    printf("Before swap: p = %f, q = %f\n", p, q);
    swap(double, p, q);
    printf("After swap: p = %f, q = %f\n", p, q);

    printf("PS: The advantage of using macros is that they can handle different data types \n -without needing separate functions for each type.\n");

    getchar(); // 暂停，等待用户按下回车键
    return 0;
}