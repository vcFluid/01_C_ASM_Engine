/*
 * 【题目 6：全自动流场扫描 (The while Scanner)】
 * 目的：熟练掌握 *p++ 这一高效的流场遍历逻辑。
 * 任务：
 * 1. 编写 clear_field(double *start, double *end) 函数；
 * 2. 使用 while(start < end) { *start++ = 0.0; } 逻辑清空区域。
 * 物理本质：模拟 Basilisk foreach() 宏底层对计算域的线性扫描。
 */

#include <stdio.h>

void clear_field(double *start, double *end)
{
    double *ptr = start;
    while(ptr < end)
    {
        *ptr = 0.0;
        ptr++;
    }
}

int main(void){
    double field[100];
    for(int i = 0; i < 100; i++){
        field[i] = (double)(i + 1);
        printf("field[%d] = %f\n", i, field[i]);
    }
    clear_field(&field[0], &field[100]);
    for(int i = 0; i < 100; i++){
        printf("field[%d] = %f\n", i, field[i]);
    }
    getchar(); // 暂停，等待用户按下回车键
    return 0;
}