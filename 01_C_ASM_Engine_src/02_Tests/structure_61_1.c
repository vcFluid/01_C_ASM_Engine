#include <stdio.h>

int main (void){
    struct point1{
        int x1;
        double y1;
    };
    printf("Size of struct point(int x,double y): %zu bytes\n", sizeof(struct point1));
    
    struct point2{
    int x2;
    double y2;
    } pt;
    pt = (struct point2){10, 20.5};
    printf("pt.x2: %d, pt.y2: %.2f\n", pt.x2, pt.y2);
    
    int *p;
    double *q;
    p = &pt.x2;
    q = &pt.y2;
    printf("Address of pt.x2: %p, Address of pt.y2: %p\n", (void*)p, (void*)q);
    *p = 30;
    *q = 40.5;
    printf("After modification, pt.x2: %d, pt.y2: %.2f\n", pt.x2, pt.y2);

    struct point3{
        int x3;
        double y3;
    } pt_array[2] = { {1, 1.1}, {2, 2.2} };
    printf("pt_array[0]: (%d, %.2f), pt_array[1]: (%d, %.2f)\n", pt_array[0].x3, pt_array[0].y3, pt_array[1].x3, pt_array[1].y3);

    getchar(); // 暂停，等待用户按下回车键
    return 0;
}
/*
explanation:
①定义了多个结构体类型，并展示了如何声明结构体变量和数组。
②展示了如何访问结构体成员及其地址。
*/