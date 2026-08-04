/* GCC Bug #63710 - Incorrect column number for -Wconversion
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=63710
 */


unsigned long f1(unsigned long ul, char l) {
  unsigned long r1 = ul + l;
  unsigned long r2 = l + ul;
  return r1 + r2;
}
// $: 
// $: gcc-trunk -c -Wconversion s.c
// s.c: In function ‘f1’:
// s.c:2:25: warning: conversion to ‘long unsigned int’ from ‘char’ may change the sign of the result [-Wsign-conversion]
   unsigned long r1 = ul + l;
//                          ^
// s.c:3:24: warning: conversion to ‘long unsigned int’ from ‘char’ may change the sign of the result [-Wsign-conversion]
   unsigned long r2 = l + ul;
//                         ^
// $: 
// $: clang-trunk -c -Wconversion s.c
// s.c:2:27: warning: implicit conversion changes signedness: 'char' to
//       'unsigned long' [-Wsign-conversion]
  unsigned long r1 = ul + l;
//                         ~ ^
// s.c:3:22: warning: implicit conversion changes signedness: 'char' to
//       'unsigned long' [-Wsign-conversion]
  unsigned long r2 = l + ul;
//                      ^ ~
// 2 warnings generated.


