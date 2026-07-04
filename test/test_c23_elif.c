// C23 #elifdef / #elifndef
// https://en.cppreference.com/c/preprocessor/conditional

#include <stdio.h>

#define FOO 1

int main(void)
{
    int x = 0;

#if 0
    x = 1;
#elifdef FOO
    x = 42;
#else
    x = 99;
#endif

    if (x != 42) return 1;

#if 1
    x = 10;
#elifndef BAR
    x = 99;
#else
    x = 0;
#endif

    if (x != 10) return 2;

    // Test with undefined macro
#if 0
    x = 99;
#elifdef UNDEFINED_MACRO
    x = 1;
#elifndef FOO
    x = 2;
#else
    x = 77;
#endif

    if (x != 77) return 3;

    printf("PASS\n");
    return 0;
}
