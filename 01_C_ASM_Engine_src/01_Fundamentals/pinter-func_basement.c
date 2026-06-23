/*
函数指针 = 一个变量，里面存的是函数的入口地址。

int a = 10;
int *p = &a;

void hello(void) {
    printf("OK!\n");
}

void (*fp)(void) = hello;
fp();
这里 fp 不是函数，它是一个变量。它里面存着 hello(函数) 的地址。
*/