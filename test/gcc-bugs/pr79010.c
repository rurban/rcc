/* GCC Bug #79010 - -Wlarger-than ineffective for VLAs, alloca, malloc
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79010
 */
/* { dg-do compile } */


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
// b.c:3:6: warning: size of ‘a’ is 123456 bytes [-Wlarger-than=]
 char a[N];
//       ^
// b.c: In function ‘farray’:
// b.c:7:8: warning: size of ‘a’ is 123456 bytes [-Wlarger-than=]
   char a[N];
//         ^
// b.c: In function ‘funnamed_array’:
// b.c:13:18: warning: size of ‘({anonymous})’ is 123456 bytes [-Wlarger-than=]
   sink ((char[N]){ 0 });
//                   ^
// b.c:13:18: warning: size of ‘({anonymous})’ is 123456 bytes [-Wlarger-than=]


