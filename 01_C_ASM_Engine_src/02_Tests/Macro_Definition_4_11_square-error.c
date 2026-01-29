#include <stdio.h>
#define SQUARE(x) x * x

int main() {
    int a = 5;
    int result = SQUARE(a + 1); // 预期结果应为 36
    printf("for a = %d, Result of SQUARE(a + 1): %d\n", a, result); // 实际输出为 16
    printf("Expected result is 36 because (a + 1) * (a + 1) = 6 * 6 = 36\n");
    printf("However, due to macro expansion, it becomes a + 1 * a + 1 = %d\n", a + 1 * a + 1);
    printf("This demonstrates the importance of using parentheses in macro definitions.\n");
    printf("Because the logic of Macro is just text replacement, \n -lack of parentheses can lead to unexpected operator precedence issues.\n");
    printf("Correct macro definition should be: #define SQUARE(x) ((x) * (x))\n");
    getchar(); // 暂停，等待用户按下回车键
    return 0;
}