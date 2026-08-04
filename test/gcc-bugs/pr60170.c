/* GCC Bug #60170 - No -Wtype-limits warning with -O1
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=60170
 */


unsigned short *g;
int fn1() {
  unsigned char ***const l = 0;
  return -4L == (*g = l == 0);
}
// $: gcc-trunk -Wtype-limits -c s.c
// s.c: In function ‘fn1’:
// s.c:4:14: warning: comparison is always false due to limited range of data type [-Wtype-limits]
   return -4L == (*g = l == 0);
//               ^
// $: gcc-trunk -Wtype-limits -c -O1 s.c
// $: gcc-trunk --version
// gcc-trunk (GCC) 4.9.0 20140210 (experimental)
// Copyright (C) 2014 Free Software Foundation, Inc.
// This is free software; see the source for copying conditions.  There is NO
// warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.


