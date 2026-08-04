/* GCC Bug #31367 - Should not warn about use of deprecated type in deprecated struct
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=31367
 */
/* { dg-do compile } */


typedef __attribute__((deprecated)) int foo;
typedef __attribute__((deprecated)) struct bar {
  foo baz;
} bop;
// With current mainline this gives
// It should not give any warning.


