# C 语言语法速查

本文只解释教学代码中实际出现的语法，不试图覆盖完整 C 语言。

## 1. 编译过程

一个 `.c` 文件通常经过：

```text
preprocess -> compile -> assemble -> link
```

- preprocess：处理 `#include`、`#define` 和条件编译；
- compile：把 C 翻译为汇编；
- assemble：生成 object file；
- link：把 object file 和 library 组合为 executable。

命令：

```powershell
gcc source.c -std=c11 -O2 -Wall -Wextra -o program.exe -lm
```

- `-std=c11`：使用 C11；
- `-O2`：启用常见优化；
- `-Wall -Wextra`：打开较多编译警告；
- `-o`：指定输出文件；
- `-lm`：链接 math library。

编译警告不等于运行错误，但不应在没有理解的情况下忽略。

## 2. 声明的基本读法

```c
double x;
double *p;
const double *input;
double values[501];
```

- `double x`：一个 double 对象；
- `double *p`：指向 double 的指针；
- `const double *input`：不能通过该指针修改目标 double；
- `double values[501]`：包含 501 个 double 的数组。

复杂声明应从变量名向外读：

```c
int (*allocate)(solver *self);
```

`allocate` 是指针，指向“接收 `solver *` 并返回 `int`”的函数。

## 3. `&`、`*` 与 `->`

```c
solver s;
solver *p = &s;
```

- `&s`：取得 `s` 的地址；
- `*p`：访问 `p` 指向的对象；
- `p->nx`：等价于 `(*p).nx`；
- `s.nx`：对象本身使用 `.` 访问成员。

不要把声明中的 `*` 和表达式中的 `*` 混为一谈：

```c
double *p;  /* 声明 p 是 pointer */
double x = *p; /* dereference，读取 p 指向的值 */
```

## 4. 数组与指针

函数参数：

```c
void f(double *q);
```

调用：

```c
double q[10];
f(q);
```

多数表达式中，数组名 `q` 转换为指向首元素的指针。于是：

```c
q[i] == *(q + i)
```

但数组和指针并非完全相同：

- 数组本身包含全部元素；
- 指针对象只保存地址；
- 在数组定义所在 scope，`sizeof(q)` 是整个数组大小；
- 作为函数参数后，`sizeof(q)` 通常只得到 pointer 大小。

## 5. `const`

```c
const double *p;
```

不能写 `*p = value`，但可以让 `p` 改指向。

```c
double *const p = address;
```

不能让 `p` 改指向，但可以修改 `*p`。

```c
const double *const p = address;
```

指针和目标都不能通过该名称修改。

教学代码主要使用第一种：输入数组只读，输出数组可写。

## 6. `static`

file-scope function：

```c
static int helper(void);
```

表示 internal linkage，该函数名只在当前 translation unit 可链接。

local static variable：

```c
void f(void) {
    static int count;
}
```

`count` 只初始化一次，具有 static storage duration，但作用域仍在函数内部。

这两种 `static` 的重点不同。

## 7. `typedef` 与 forward declaration

```c
typedef struct Riemann_1D_MacC_solver solver;
```

同时完成：

1. 声明一个尚未展开的结构体 tag；
2. 建立别名 `solver`。

之后可声明：

```c
solver *self;
```

在结构体完整定义前只能安全声明 pointer，不能访问成员或创建完整对象。

## 8. `enum`

```c
typedef enum {
    SENSOR_RHO = 1,
    SENSOR_U = 2
} Sensor;
```

枚举为整数状态提供名字。C enum 仍可与整数混用，因此外部输入必须验证范围。

## 9. 条件和循环

```c
if (condition) {
} else {
}
```

C 中 0 为 false，非 0 为 true。

```c
for (int i = 0; i < n; i++) {
}
```

顺序为：

1. 初始化 `i=0`；
2. 判断 `i<n`；
3. 执行 body；
4. 执行 `i++`；
5. 回到条件判断。

```c
while (condition) {
}
```

每轮开始前检查条件。

## 10. 短路求值

```c
if (pointer != NULL && pointer[0] != '\0')
```

若左侧为 false，右侧不会执行。这可以防止空指针解引用。

```c
if (failed_a || failed_b)
```

若左侧为 true，右侧不会执行。

不要把逻辑运算符 `&& || !` 与位运算符 `& | ~` 混用。

## 11. 字符、字符串与转义

```c
'a'   /* char */
"a"   /* char array: {'a', '\0'} */
```

常见转义：

```text
\n newline
\t tab
\\ backslash
\" double quote
\0 null character
```

Windows 路径：

```c
"folder\\file.dat"
```

运行时才是：

```text
folder\file.dat
```

## 12. format string

`printf` family：

```text
%d      int
%s      char *
%c      character
%f      floating point
%.10f   fixed point with 10 digits after decimal
%.17g   up to 17 significant digits
%02d    width 2, padded with zero
```

`scanf` family：

```text
%d  -> int *
%lf -> double *
```

format 与参数类型不匹配可能造成 undefined behavior。

## 13. 动态内存

```c
double *q = calloc(n, sizeof(double));
if (q == NULL) {
    /* allocation failed */
}
free(q);
q = NULL;
```

规则：

- allocation 成功后必须明确 ownership；
- ownership 结束时 `free`；
- 不重复 `free`；
- `free` 后不再读取或写入；
- `free(NULL)` 合法。

## 14. `memcpy` 与 `memset`

```c
memset(&s, 0, sizeof(s));
```

把对象的 bytes 清零。

```c
memcpy(destination, source, count);
```

复制 bytes。source 和 destination 不得重叠；重叠时用 `memmove`。

两者都不理解对象的物理含义，也不执行数值格式。

## 15. 文件 I/O

```c
FILE *fp = fopen(path, "w");
if (fp == NULL) {
    /* failed */
}
fprintf(fp, "value = %.10f\n", value);
fclose(fp);
```

常用 mode：

- `"r"`：读取文本；
- `"w"`：重写文本；
- `"a"`：追加文本；
- `"rb"`：读取 binary。

必须检查 `fopen` 返回值。

## 16. `system()`

```c
int status = system(command);
```

它把字符串交给系统 command processor。优点是简单，缺点是：

- quoting 复杂；
- process startup 有成本；
- 错误信息有限；
- 不可信字符串可能造成 command injection；
- 平台相关。

本项目使用它保持 Project 0 和 Project 2 的 executable 独立性。

## 17. Undefined behavior

以下行为不保证得到可预测结果：

- 数组越界；
- 除以零；
- 空指针解引用；
- use-after-free；
- double free；
- format string 与参数类型不匹配；
- 对 0 执行 integer remainder；
- 通过不兼容类型的 function pointer 调用函数。

编译成功不代表没有 undefined behavior。

