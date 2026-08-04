/* GCC Bug #85425 - gcc 6.2.1 fails to catch error in function calling arguments
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=85425
 */


void ghhrobust_search (double ay, int type) { }

void f(int i, double d) { ghhrobust_search(i, d); }
// This must not produce an error, because it is 100% valid according to the C standard. An int can be converted to a double and a double can be converted to an int, so the code is valid.
// If you use -Wconversion you get a warning:
// a.c: In function ‘void f(int, double)’:
// a.c:3:48: warning: conversion to ‘int’ from ‘double’ may alter its value [-Wfloat-conversion]
 void f(int i, double d) { ghhrobust_search(i, d); }
//                                                 ^
// This seems to be what you're looking for. Indeed, compiling your attached code with -Wconversion gives a warning on the relevant line:
// fail_contour.c: In function ‘contour_trace’:
// fail_contour.c:203:56: warning: conversion to ‘int’ from ‘double’ may alter its value [-Wfloat-conversion]


