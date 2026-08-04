/* GCC Bug #98217 - Prefer a warning for when VLAs declared on stack
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=98217
 */
/* { dg-do compile } */


void f (void*, ...);

void func1(int n, int array[n]) /* ok, no warning */
{
  int array2[n];     /* bad, VLA on stack, warn! */
  int (*array3)[n];  /* ok, no VLA on stack, so no warning */
  f (array, array2, array3);
}
// $ gcc -O2 -S -Wvla-larger-than=0 pr98217.c
// pr98217.c: In function 'func1':
// pr98217.c:5:7: warning: argument to variable-length array may be too large [-Wvla-larger-than=]
//     5 |   int array2[n];     /* bad, VLA on stack, warn! */
//       |       ^~~~~~
//
// The warning triggers whenever a VLA is either a) known to be larger than
// the limit or b) not known to be less than or equal to it.

// For better portability, the solution recommended for single-dimensional
// VLAs over the forward parameter declaration is attribute access.  This
// was originally a second, separate file "pr98217-2.c" in the report;
// merged here (function renamed f -> f2 to avoid clashing with f above)
// to keep this a single standalone translation unit.
typedef __SIZE_TYPE__ size_t;

__attribute__ ((access (read_write, 1, 2)))
void my_memset (char *p, size_t n, char v)
{
  __builtin_memset (p, v, n);
}

char a[3];

void f2 (void)
{
  my_memset (a, 3 * sizeof a, 0);
}
// $ gcc -S pr98217-2.c
// pr98217-2.c: In function 'f2':
// pr98217-2.c:13:3: warning: 'my_memset' accessing 9 bytes in a region of size 3 [-Wstringop-overflow=]
//    13 |   my_memset (a, 3 * sizeof a, 0);
//       |   ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// pr98217-2.c:4:6: note: in a call to function 'my_memset' declared with attribute 'access (read_write, 1, 2)'
//     4 | void my_memset (char p[], size_t n, char v)
//       |      ^~~~~~~~~
//
// (Worth noting is that neither works reliably when the function is inlined
// and the association between the array and its bound is lost.)
