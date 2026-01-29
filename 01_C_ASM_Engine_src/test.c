#include <stdio.h>
#include <stdlib.h>

int main(void) {

    system("chcp 65001"); // 切换控制台到 UTF-8 编码，解决乱码

    int value = 2026;       // 存入一个年份
    int *ptr = &value;      // 定义一个指针，指向这个年份的地址

    printf("Hello XJTU! 环境配置成功。\n");
    printf("数值内容: %d\n", value);


    printf("数值的内存地址 (指针内容): %p\n", (void *)ptr);// 观察这个神秘的十六进制地址
    getchar(); // 暂停，等待用户按下回车键
    return 0;
}