/* GCC Bug #105923 - unsupported return type ‘complex double’ for simd
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105923
 */


__attribute__ ((__simd__ ("notinbranch")))
double _Complex foo (double _Complex __x) __attribute__ ((__nothrow__ , __leaf__));
double _Complex foo (double _Complex __x) __attribute__ ((__nothrow__ , __leaf__));

int N = 3200;
double _Complex b[3200];
double _Complex a[3200];
// void
// bar (void)
{
  int i;

  for (i = 0; i < N; i += 1)
    b[i] = foo (a[i]);
}
// [hjl@gnu-tgl-3 tmp]$ gcc -S -Ofast c.c
// c.c:3:17: warning: unsupported return type ‘complex double’ for simd
//     3 | double _Complex foo (double _Complex __x) __attribute__ ((__nothrow__ , __leaf__));
//       |                 ^~~
// [hjl@gnu-tgl-3 tmp]$


