/* GCC Bug #98217 - Prefer a warning for when VLAs declared on stack
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=98217
 */


void f (void*, ...);

void func1(int n, int array[n]) /* ok, no warning */
{
  int array2[n];     /* bad, VLA on stack, warn! */
//   int (*array3)[n];  /* ok, no VLA on stack, so no warning */
  f (array, array2, array3);
}
//     5 |   int array2[n];     /* bad, VLA on stack, warn! */

// The warning triggers whenever a VLA is either a) known to be larger than the limit or b) not known to be less than or equal to it.  Please let us know if this isn't sufficient.
typedef __SIZE_TYPE__ size_t;

__attribute__ ((access (read_write, 1, 2)))
void my_memset (char *p, size_t n, char v)
{
  __builtin_memset (p, v, n);
}

char a[3];

void f (void)
{
  my_memset (a, 3 * sizeof a, 0);
}
//    13 |   my_memset (a, 3 * sizeof a, 0);
//    href="show_bug.cgi?id=98217">pr98217</a>-2.c:4:6: note: in a call to function ‘my_memset’ declared with attribute ‘access (read_write, 1, 2)’
//     4 | void my_memset (char p[], size_t n, char v)

// (Worth noting is that neither works reliably when the function is inlined and the association between the array and its bound is lost.)


