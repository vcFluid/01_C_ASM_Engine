/*
 * 【题目 5：格点距离探测器 (Distance Logic)】
 * 目的：理解指针相减的物理意义（逻辑距离 vs 物理距离）。
 * 任务：
 * 1. 指针 p_start 指向 pressure[10]，p_end 指向 pressure[60]；
 * 2. 计算 (p_end - p_start) 观测结果是否为格点数 50；
 * 3. 强制类型转换 (char *) 后相减，观测物理字节数是否为 400。
 * 物理本质：C 语言地址算术自带“量纲”，自动抵消数据类型大小。
 */

#include <stdio.h>

int main(void){
    double pressure[100];
    double *p_start = &pressure[10];
    double *p_end = &pressure[60];
    long elements = (long)(p_end - p_start);
    printf("elements between pressure[10] and pressure[60]: %ld\n bytes", elements);
    long bytes = (60 - 10) * sizeof(double);
    printf("\nExpected bytes calculation: %ld bytes\n", bytes);
    long incorrect_distance = &p_end - &p_start;
    printf("Incorrect bytes using addresses of pointers: %ld bytes\n", incorrect_distance);
    long correct_distance = (long)((char*)p_end - (char*)p_start);
    printf("Correct bytes using char pointer casting: %ld bytes\n", correct_distance);

    getchar(); // 暂停，等待用户按下回车键
    return 0;
}