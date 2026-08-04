/* GCC Bug #81402 - unhelpful -Wparentheses suggestion for assignment from non-zero constant
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=81402
 */


{
//   if (i = j) { }  // assignment could be intended
}

void g (int i)
{
  if (i = 1) { }   // assignment almost certainly not intended
}
// a.c: In function ‘f’:
// a.c:3:7: warning: suggest parentheses around assignment used as truth value [-Wparentheses]
//    if (i = j) { }  // assignment could be intended
//        ^
// a.c: In function ‘g’:
// a.c:8:7: warning: suggest parentheses around assignment used as truth value [-Wparentheses]
   if (i = 1) { }   // assignment almost certainly not intended
//        ^


