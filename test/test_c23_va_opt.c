// C23 __VA_OPT__ variadic macro support
// https://en.cppreference.com/c/preprocessor/replace

#include <stdio.h>

#define LOG1(fmt, ...) printf(fmt __VA_OPT__(,) __VA_ARGS__)
#define LOG2(fmt, ...) printf("[" fmt "]" __VA_OPT__(, __VA_ARGS__))

int main(void)
{
    // __VA_OPT__ with empty args: comma suppressed
    LOG1("hello\n");

    // __VA_OPT__ with args: comma present
    LOG1("x=%d\n", 42);

    // __VA_OPT__ with content using __VA_ARGS__
    LOG2("test\n");

    LOG2("x=%d y=%d\n", 1, 2);

    printf("PASS\n");
    return 0;
}
