#include <stdio.h>

int main(void){
    double *sensor = NULL; // 模拟搜索失败或没有子网格的情况

    // 1. 无保护访问（物理爆炸）
    // printf("%f", *(sensor + 1)); // 如果取消注释这行，程序会立即崩溃(Segfault)

    // 2. 你的保护逻辑（安全整流）
    if (sensor != NULL) {
        printf("Value: %f\n", *(sensor + 1));
    } else {
        printf("warning, sensor pointer is NULL, skipping access to avoid crash.\n");
    }
    getchar(); // 暂停，等待用户按下回车键
    return 0;
}