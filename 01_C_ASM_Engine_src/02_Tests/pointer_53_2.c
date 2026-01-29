/*
 * 【题目 2：手动差分算子 (Manual Stencil)】
 * 目的：不依赖下标 []，直接操作地址实现数值梯度计算。
 * 任务：
 * 1. 声明指针 ptr 指向数组 u[10] 的中心 u[5]；
 * 2. 仅利用 *(ptr + offset) 形式计算一阶中心差分：(u[i+1] - u[i-1]) / (2 * dx)；
 * 3. 观测并处理 i=9 时的边界溢出风险。
 * 物理本质：Basilisk 源码中算子的底层实现即为指针偏移。
 */s

/*
题目 2：手动差分算子 (Manual Stencil)
在 centered.h 的离散化中，计算梯度需要访问邻居节点。
任务： 给定一个一维数组 double u[10]，代表速度场。
1、声明一个指针 ptr 指向 u[5]（场中心）。
2、不许使用方括号 []，仅利用指针算术和解引用 *，计算该点的一阶中心差分：
3、要求： 代码形式应类似于 ( *(ptr + 1) - *(ptr - 1) ) / ...。
*/

#include <stdio.h>
int main(void){
    double u[10] = {1.0, 2.1, 1.2, 3.3, 2.4, 1.5, 3.6, 1.7, 2.8, 1.9};
    double *ptr = &u[5];
    for(int i = 0; i < 5; i++){
        double gradient = ( *(ptr + i + 1) - *(ptr + i - 1) ) / 2.0;
        printf("Gradient at u[%d]: %f\n", 5 + i, gradient);
    }
    getchar(); // 暂停，等待用户按下回车键
    return 0;
}