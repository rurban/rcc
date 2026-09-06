/* GCC Bug #60069 - Different warning messages for -Wconversion with different optimization levels
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=60069
 */
/* { dg-do compile } */


void fn1() {
  unsigned short *l_432;
  const long l_448 = 70758;
//   *l_432 = +l_448;
}
// $: gcc-trunk -O0 -c -Wconversion s.c
// s.c: In function ‘fn1’:
// s.c:4:12: warning: conversion to ‘short unsigned int’ from ‘long int’ may alter its value [-Wconversion]
//    *l_432 = +l_448;
//             ^
// $: gcc-trunk -O1 -c -Wconversion s.c
// s.c: In function ‘fn1’:
// s.c:4:12: warning: large integer implicitly truncated to unsigned type [-Woverflow]
//    *l_432 = +l_448;
//             ^
// $: gcc-trunk --version
// gcc-trunk (GCC) 4.9.0 20140204 (experimental)
// Copyright (C) 2014 Free Software Foundation, Inc.
// This is free software; see the source for copying conditions.  There is NO
// warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.


