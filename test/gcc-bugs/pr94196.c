/* GCC Bug #94196 - Multiple issues with attributes
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=94196
 */


#include <stdio.h>

int __attribute__((pure,noipa)) my_pure_func(int x) {
//     printf("pure func called with %d\n", x);
    return x;
}

int __attribute__((const,noipa)) my_const_func(int x) {
//     printf("const func called with %d\n", x);
    return x;
}

int __attribute__((pure)) (*pure_ptr)(int) = my_pure_func;
int __attribute__((const)) (*const_ptr)(int) = my_const_func;

int a,b,c,d;

void foo(void) {
    a = pure_ptr(1) + pure_ptr(1);
    b = my_pure_func(2) + my_pure_func(2);
    c = const_ptr(3) + const_ptr(3);
    d = my_const_func(4) + my_const_func(4);
}

int main(void) {
//     foo();
    return 0;
}
//    13 | int __attribute__((pure)) (*pure_ptr)(int) = my_pure_func;

// and indeed `my_pure_func(1)` is called twice.  The others are handled correctly and only called once.


