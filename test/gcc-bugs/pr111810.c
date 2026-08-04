/* GCC Bug #111810 - rtlFE: nested inline RTL function cause ICE
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=111810
 */
/* { dg-do compile } */
/* { dg-options "-std=gnu89" } */

/* Comment 1's reduced testcase: a nested inline __RTL function used to
 * segfault gcc during error recovery (finish_function).  Modern gcc
 * errors out cleanly.  __RTL is gcc's internal RTL front-end. */
void j() {inline void __RTL test(void) {}}