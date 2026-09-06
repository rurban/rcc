/* GCC Bug #81402 - unhelpful -Wparentheses suggestion for assignment from non-zero constant
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=81402
 */
/* { dg-do compile } */


void f (int i, int j)
{
  if (i = j) { }  /* { dg-warning "suggest parentheses around assignment used as truth value" } */
}

void g (int i)
{
  if (i = 1) { }   /* { dg-warning "suggest parentheses around assignment used as truth value" } */
}
// a.c: In function ‘f’:
// a.c:3:7: warning: suggest parentheses around assignment used as truth value [-Wparentheses]
//    if (i = j) { }  // assignment could be intended
//        ^
// a.c: In function ‘g’:
// a.c:8:7: warning: suggest parentheses around assignment used as truth value [-Wparentheses]
// if (i = 1) { }   // assignment almost certainly not intended
//        ^


