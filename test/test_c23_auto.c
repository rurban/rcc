// C23 __auto_type / auto type inference
// https://en.cppreference.com/c/language/auto

#include <stdio.h>
#include <string.h>

int main(void)
{
    // __auto_type infers int from literal
    __auto_type a = 42;
    if (a != 42) return 1;

    // __auto_type infers pointer from string
    __auto_type b = "hello";
    if (strcmp(b, "hello") != 0) return 2;

    // __auto_type infers double
    __auto_type c = 3.14;
    if (c < 3.13 || c > 3.15) return 3;

    // __auto_type infers from expression
    __auto_type d = a + 8;
    if (d != 50) return 4;

    // __auto_type infers unsigned from cast
    __auto_type e = (unsigned)42;
    if (e != 42) return 5;

    printf("PASS\n");
    return 0;
}
