/* GCC Bug #85290 - Defining identifiers to themselves in system headers prevents diagnostics from being emitted.
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=85290
 */
/* { dg-do compile } */


int test(void)
{
//  test();
# 8 "bla.c" 3
// pid_t 
# 8 "bla.c"
      pid = 0;

//  test();

 return pid;
}
// essentialy marking the remaining translation unit as system header.


