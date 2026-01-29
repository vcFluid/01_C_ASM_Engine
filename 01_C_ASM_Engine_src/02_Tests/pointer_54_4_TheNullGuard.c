/*
 * 【题目 8：零点保护 (The NULL Guard)】
 * 目的：建立防御性编程思维，防止自适应网格中的空指针崩溃。
 * 任务：
 * 1. 模拟一个可能返回 NULL 的探测器指针；
 * 2. 在执行 *(ptr + 1) 前必须进行 if(ptr != NULL) 检查。
 * 物理本质：防止非法触碰硬件保护区，确保“格鲁特”引擎持续运行。
 */

#include <stdio.h>

int main(void){
    double a[2][3] = {
        {1.1, 2.2, 3.3},
        {4.4, 5.5, 6.6}
    };
    double *sensor = NULL;
    sensor = a[0];
    if (sensor != NULL) {
    *(sensor + 1);
    
    printf("Value at this point: %f\n", *(sensor + 1));
    getchar(); // 暂停，等待用户按下回车键
    return 0;
}