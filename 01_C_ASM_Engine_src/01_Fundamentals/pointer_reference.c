#include <stdio.h>
#include <windows.h>
// 在 main 函数开头执行
int main(void) {
    SetConsoleOutputCP(65001);
    int a = 10;
    int b = 20;
    int *p = NULL; // 养成初始化为 NULL 的好习惯

    // 1. 正确姿势：指向变量 a
    p = &a; 
    printf("【1】p 指向了 a。p的值（地址）: %p, 指向的值: %d\n", (void*)p, *p);

    // 2. 正确姿势：通过指针修改 a 的值
    *p = 100; 
    printf("【2】修改后 a 的值变成了: %d\n", a);

    // 3. 正确姿势：切换指向，指向变量 b
    p = &b;
    printf("【3】p 现在改向了 b。指向的值: %d\n", *p);

    // 4. 关键实验：改变 *p 会影响谁？
    *p = 999;
    printf("【4】修改 *p 后，b 的值变成了: %d，而 a 还是: %d\n", b, a);

    getchar(); // 暂停，等待用户按下回车键
    return 0;
}