/* GCC Bug #70954 - -Wmisleading-indentation false positive on code from GNU "ed" (featuring a label)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70954
 */
/* { dg-do compile } */


void test (const char ** const ibufpp)
{
  int c = *(*ibufpp)++;
  switch (c)
    {
    case '#': while( *(*ibufpp)++ != '\n' ) ;
              break;
    }
}
// test.c: In function ‘test’:
// test.c:6:15: warning: this ‘while’ clause does not guard... [-Wmisleading-indentation]
     case '#': while( *(*ibufpp)++ != '\n' ) ;
//                ^~~~~
// test.c:7:15: note: ...this statement, but the latter is misleadingly indented as if it is guarded by the ‘while’
               break;
//                ^~~~~
// I agree that this is a false positive.
// With trunk <a href="https://gcc.gnu.org/viewcvs/gcc?view=revision&revision=235889">r235889</a> (and presumably the gcc-6-branch), we currently exit from
//   should_warn_for_misleading_indentation
here:
// 495		  /* Otherwise, they are visually aligned: issue a warning.  */
// 496		  return true;


