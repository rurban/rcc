/* GCC Bug #81543 - attribute may_alias on function and variable declarations silently accepted
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=81543
 */
/* { dg-do compile } */


int var __attribute__ ((may_alias));

void __attribute__ ((may_alias)) foo (void);
// $


