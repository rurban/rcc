/* GCC Bug #79269 - Calculate size of struct with flexible array at compile time
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79269
 */
/* { dg-do compile } */


char a[] = { sizeof a, 2, 3, 4 };

struct {
    char a;
    char b[];
} test = { 10, { 0, 1, 2, 3 } };

unsigned size (void)
{
  return __builtin_object_size (&test, 0);
}


