/* GCC Bug #101537 - -Wconversion false positive in ternary and |=
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=101537
 */
/* { dg-do compile } */


int foo() {
    int aaa = 1;
    unsigned char bbb = 0;
    bbb |= aaa ? 1 : 0;
    return bbb;
}
// Gives this warning:
// <source>: In function 'foo':
// <source>:4:12: warning: conversion from 'int' to 'unsigned char' may change value [-Wconversion]
//     4 |     bbb |= aaa ? 1 : 0;
//       |            ^~~
// Compiler returned: 0
// It happens both in C and C++ modes.


