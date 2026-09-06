/* GCC Bug #79010 - -Wlarger-than ineffective for VLAs, alloca, malloc
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79010
 */
/* { dg-do compile } */


#define N 123456

void sink (void*);

char a[N];

void farray (void)
{
  char a[N];
  sink (a);
}

void funnamed_array (void)
{
  sink ((char[N]){ 0 });
}

void fvla (void)
{
  int n = N;
  char a[n];
  sink (a);
}

void falloca (void)
{
  void *a = __builtin_alloca (N);
  sink (a);
}

void fmalloc (void)
{
  void *a = __builtin_malloc (N);
  sink (a);
}

