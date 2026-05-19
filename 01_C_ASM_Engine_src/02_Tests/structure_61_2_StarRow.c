#include <stdio.h>

struct StarRow {
    int offset; // 每一行前面的空格数（决定了斜率和位置）
    int count;  // 每一行要打印的星星数（你的要求是 3）
};
 //注意到这里的结构体必须在函数声明之前定义，因为我们要在函数draw_row调用中使用它。

void draw_row(struct StarRow *p) {
    // 1. 打印前面的空格 (由 offset 决定)
    for (int i = 0; i < p->offset; i++) {
        printf(" ");
    }
    // 2. 打印星星 (由 count 决定)
    for (int j = 0; j < p->count; j++) {
        printf("*");
    }
    // 3. 换行
    printf("\n");
}
//注意到被调用的函数draw_row接受一个指向StarRow结构体的指针参数，这样我们就可以通过指针访问和使用结构体成员。
//被调用函数内部使用了->运算符来访问结构体成员，因为它接收的是一个指针。
//同时被调用函数应该放在main函数之前定义，以确保在main中调用时已经知道它的存在。

int main() {
    // 初始化我们的“渲染对象”
    struct StarRow myLine = {0, 3}; // 初始偏移 0，每行 3 颗星

    int total_rows = 5; // 我们画 5 行来看看效果

    for (int i = 0; i < total_rows; i++) {
        // 调用引擎绘图
        draw_row(&myLine); 

        // 更新逻辑：为了让它变斜，每行偏移量 +1
        myLine.offset++; 
        getchar(); // 每画一行暂停，等待用户按下回车键
    }
    getchar();

    return 0;
}

/*
理解：①状态与行为的解耦
数据结构：struct StarRow 定义了每一“格点”的属性（物理状态：有无星星，位置偏移）。
计算算子：draw_row 是一个执行器，不关心数据是谁，只关心给定一个指针，按规矩干活
时间/空间演化：MyLine.offset++ 体现了时间推进下的状态变化（每行偏移增加）。对应时间步进(Time Stepping)

理解：②网格点是如何“偏移”的？
1、索引偏移
每次调用 draw_row 后，我们通过 myLine.offset++ 来增加偏移量，从而实现了星星行的斜率效果。
在代码中，offset是一个整数。在内存里，p -> offset 告诉程序：“跳过前N个内存单元再开始操作”
在basilisk的一维数组模拟中，u[i]到u[i+1]的偏移本质上就是指针在地址空间中移动了sizeof(double)个字节。

2、模板偏移

3、树状网格的层级偏移
*/