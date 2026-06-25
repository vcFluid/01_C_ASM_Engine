#include <stdio.h>
#define MAX(a, b) ((a) > (b) ? (a) : (b))
int main() {
    int x = 5;
    int y = 10;
    int m = 15;
    int n = 20;
    int max_val = MAX(x + m, y + n);

    int i = 10;
    int j = 20;
    int max_ij = MAX(i++, j++); // 注意这里的副作用
    printf("Max of x + m = %d and y + n = %d is %d\n", x + m, y + n, max_val);
    printf("Max i++ and j++ of i = %d and j = %d is %d\n", 10, 20, max_ij);
    printf("After MAX macro, i = %d, j = %d\n", i, j); // 观察 i 和 j 的值
    getchar(); // 暂停，等待用户按下回车键
    return 0;
}

/*
result:
Max of x + m = 20 and y + n = 30 is 30
Max i++ and j++ of i = 10 and j = 20 is 21
After MAX macro, i = 11, j = 22
explanation:
The MAX macro evaluates its arguments multiple times,
leading to unintended side effects when the arguments have increment operations (like i++ and j++).
In this case, both i and j are incremented twice,
resulting in their final values being 11 and 22,respectively.
This demonstrates the importance of being cautious with macros that evaluate their parameters more than once.

*/