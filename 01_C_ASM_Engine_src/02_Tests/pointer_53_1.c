#include <stdio.h>

/*
 * 【题目 1：步长的物理探测 (The Scaling Logic)】
 * 目的：理解不同数据类型在内存中的跳转步长，识别十六进制进位幻觉。
 * 任务：
 * 1. 定义 double pressure[5]；
 * 2. 打印 p, p+1, p+2 的十六进制地址 (%p)；
 * 3. 验证相邻地址的差值是否恒为 8 字节（需考虑十六进制 0x08 -> 0x10 的进位）。
 * 物理本质：CFD 网格在内存中是绝对等距分布的线性序列。
 */

/*任务：编写一个程序，定义一个长度为 5 的 double 数组（模拟压力场 $P$）。
使用指针 p 指向数组首元素。利用循环打印出：p、p+1、p+2 的十六进制地址。

思考： 连续两个地址之间的差值（以十进制字节为单位）是多少？
这个差值和你定义的 double 类型有什么物理联系？*/


int main(void){
    double a[5];
    double *p = a; // 指向数组的第一个元素
    for(int i = 0; i < 5; i++){
        printf("a[%d]'s adress: %p\n", i, (void*)(p + i)); // 通过指针访问数组元素的地址
        p[i] = (i + 1); // 通过指针访问和修改数组元素
    }
    getchar(); // 暂停，等待用户按下回车键
    return 0;
    
}

/*
Expected Output:
a[0]'s adress: 0x7fff5fbff6d0
a[1]'s adress: 0x7fff5fbff6d8
a[2]'s adress: 0x7fff5fbff6e0
a[3]'s adress: 0x7fff5fbff6e8
a[4]'s adress: 0x7fff5fbff6f0
this is 16 bytes apart for double type,
not 10 bytes apart for char type.
*/