/* GCC Bug #99587 - warning: ‘retain’ attribute ignored while __has_attribute(retain) is 1
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99587
 */
/* { dg-do compile } */


__attribute__((used, retain)) int a;
//     1 | __attribute__((used, retain)) int a;
// xgcc (GCC) 11.0.1 20210313 (experimental)
// __has_attribute(retain) should return 0 in this case.


