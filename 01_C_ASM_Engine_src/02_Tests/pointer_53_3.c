/*
 * 【题目 3：边界条件的“坍缩” (Decay & Boundaries)】
 * 目的：观测数组在跨越函数边界时的信息丢失现象。
 * 任务：
 * 1. 在 main 中定义 double pressure[100]，打印 sizeof(pressure)；
 * 2. 将数组传给函数 update_boundary(double field[])；
 * 3. 在函数内打印 sizeof(field)，观测其坍缩为 8 字节（指针大小）的现象。
 * 物理本质：函数内部无法感知网格规模 N，必须手动传递“度量衡”。
 */

/*
题目 3：边界条件的“坍缩” (Decay & Boundaries)
理解为什么在函数内部无法直接获取流场网格的总规模.
任务：
1、创建一个函数 void update_boundary(double field[]).
2、在该函数内，使用 sizeof 操作符打印 field 的大小.
3、在 main 函数中，定义一个 double pressure[100] 数组，并打印其大小.
4、从 main 函数调用 update_boundary，传入 pressure 数组.
5、溯源： 观察两次打印结果的差异。
如果要在函数内安全地更新第 100 个格点的边界值，你除了传递指针，还必须额外传递什么参数？
*/




#include <stdio.h>

void update_boundary(double field[])
    {
        sizeof(field);
        printf("Size of field in update_boundary: %zu\n", sizeof(field));
    }

int main(void){
    double pressure[100];
    sizeof(pressure);
    printf("Size of pressure in main: %zu\n", sizeof(pressure));
    update_boundary(pressure);
    getchar(); // 暂停，等待用户按下回车键
    return 0;
}

/*
if we want to renew the 100th grid point boundary value in the function safely,
    we must also pass the size of the array as an additional parameter.

    like this:
*/
    void update_boundary_safe(double field[], size_t size)
    {
        // Now we can safely access the array up to 'size' elements
        field[size - 1] = 0.0; // Example: setting the last element to 0.0
    };