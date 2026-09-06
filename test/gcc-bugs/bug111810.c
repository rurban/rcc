/* GCC Bug #111810 - rtlFE: nested inline RTL function cause ICE
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=111810
 */
/* { dg-do run } */
/* { dg-options "-std=gnu89" } */

/* Comment 1's reduced testcase: a nested inline __RTL function used to
 * segfault gcc during error recovery (finish_function).  Modern gcc
 * errors out cleanly.  __RTL is gcc's internal RTL front-end. */
#if !defined(__GNUC__) || defined(__RCC__)
#define __RTL
#endif
/* Beyond merely parsing, verify the nested inline function is a real,
 * working closure: it must return a value and that value must reach
 * the caller correctly, not just be silently accepted/discarded. */
int j(void) {inline int __RTL test(int x) { return x * 2 + 1; } return test(20);}
int main(void) { return j() == 41 ? 0 : 1; }