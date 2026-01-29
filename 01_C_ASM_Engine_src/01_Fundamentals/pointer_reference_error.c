#include <stdio.h>

int main (void){
    int a = 10;
    int *p; //声明p是一个指向整数的指针变量
    *p = a; //将a的值赋给p所指向的内存单元（未初始化指针，可能导致未定义行为）
    printf("Value of a: %d\n", a);
    printf("Address of a: %p\n", (void*)&a);
    printf("Value of p (before assignment): %p\n", (void*)p);
    p = NULL; //将指针p初始化为NULL，避免悬空指针
    *p = &a;//将a的地址赋给p所指向的内存单元（未初始化指针，可能导致未定义行为）
    printf("Value of p (after assignment): %p\n", (void*)p);
    printf("Value pointed to by p: %d\n", *p);
    p = NULL; //将指针p初始化为NULL，避免悬空指针
    p = a;//将a的值赋给指针变量p（类型不匹配，可能导致未定义行为）
    printf("Value of p (after assigning a to p): %d\n", p);
    p = NULL; //将指针p初始化为NULL，避免悬空指针
    p = &a;//将a的地址赋给指针变量p
    printf("Value pointed to by p (after assigning address of a to p): %d\n", *p);

    getchar(); // 暂停，等待用户按下回车键
    return 0;
}