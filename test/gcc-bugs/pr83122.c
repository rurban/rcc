/* GCC Bug #83122 - -Wconversion and shifting bitwise
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=83122
 */


// gcc -c test.c -Wall -Wextra -pedantic -pedantic-errors -std=c99 -Wconversion

typedef __UINT64_TYPE__ uint64_t;

struct foo {
 uint64_t a :  3;
 uint64_t b : 61;
};

void set_a(struct foo *ptr, uint64_t val){
// 	ptr->b = val >> 3;
}
// ```
// The program above results in a warning about loss of precision:
// ```
// test.c: In function 'set_a':
// test.c:11:11: warning: conversion to 'long long unsigned int:61' from 'uint64_t {aka long long unsigned int}' may alter its value [-Wconversion]
//   ptr->b = val >> 3;
//            ^~~
// ```
// This is a false positive since the 3 MSBs of `val` are filled with zeroes.
// In reality, the warning persists even after I replace `val >> 3` with `val >> 63`.


