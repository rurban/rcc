/* GCC Bug #86695 - Calls to builtins do not use visibility information
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86695
 */


// gcc -O2 -fpic -m32
#pragma GCC visibility push(hidden)
void *memcpy(void *, const void *, __SIZE_TYPE__);
void *malloc(__SIZE_TYPE__);
#pragma GCC visibility pop

struct s {
  char c[1024*1024];
};

void f1(struct s a[2])
{
  a[1] = a[0];
}

void f2(struct s a[2])
{
//   memcpy(a+1, a, sizeof *a);
}

void *g1()
{
  return __builtin_malloc(42);
}

void *g2()
{
  return malloc(42);
}


