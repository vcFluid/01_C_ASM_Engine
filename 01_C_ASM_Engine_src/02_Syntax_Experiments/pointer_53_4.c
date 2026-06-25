/*
 * 【题目 4：二维流场的“拍扁”访问 (Dimensional Collapse)】
 * 目的：掌握多维逻辑坐标向一维物理内存地址的映射。
 * 任务：
 * 1. 定义 int grid[3][3] 并初始化；
 * 2. 声明一级指针 int *p = &grid[0][0]；
 * 3. 利用公式 *(p + i * cols + j) 访问特定元素（如 grid[2][1]）。
 * 物理本质：计算机不存在行与列，只有一条无限延伸的内存长管子。
 */

#include <stdio.h>
int main(void)
{
    int grid[3][3] = {
        {1, 2, 3},
        {4, 5, 6},
        {7, 8, 9}
    };
    int *p = &grid[0][0];
    printf("Value at grid[2][1] i.e. (3,2) = %p for address, %d for value\n", p + 7, *(p + 7));
    printf("the equation is: *(p + 2 * 3 + 1) = %d\n", *(p + 2 * 3 + 1));
    printf("the general equation is: *(p + row_index * num_columns + column_index)\n");
    printf("the reason is the logic of pointer for 2D array is to flatten it into 1D array in memory\n");
    getchar(); // 暂停，等待用户按下回车键
    return 0;
}