/* GCC Bug #79269 - Calculate size of struct with flexible array at compile time
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79269
 */


char a[] = { sizeof a, 2, 3, 4 };

struct {
    char a;
    char b[];
} test = { 10, { 0, 1, 2, 3 } };

unsigned size (void)
{
  return __builtin_object_size (&test, 0);
}
// ;; Function size (size, funcdef_no=0, decl_uid=1799, cgraph_uid=0, symbol_order=1)

// size ()
{
  unsigned int D.1802;
  unsigned int _1;

//   <bb 2> [0.00%]:
  _1 = 5;

// <L0> [0.00%]:
  return _1;

}


