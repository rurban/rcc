/* GCC Bug #45821 - Missed -Wreturn-local-addr when local variable address comes from within a statement expression
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=45821
 */


#include <stdio.h>
int * aaa() {return ({int a = 2;        &a;});} // no warning unless -O2
int * bbb() {         int b = 3; return &b;}    //    warning

int main(void)
 {int *a = aaa(), *b = bbb();
//   fprintf(stderr, "AAAA %d\n", *a);
//   fprintf(stderr, "BBBB %d\n", *b);
  return 0;
 }
// AAAA 3
// Segmentation fault (core dumped)

// Please provide the warning for line 2 (aaa) regardless of the level of optimization?


