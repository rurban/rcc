/* GCC Bug #99944 - poor format of array reference in -Wuninitialized
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99944
 */
/* { dg-do compile } */


int d;
int h(void);
void e1(void)
{
  int f[2];
  int g = 0;
  if (d)
//     g++;
  if (d == 1)
    f[g++] = 2;
  (void) (f[0] || (g && h()));
}
void e2(void)
{
  enum { a } f[2];
  int g = 0;
  if (d)
//     g++;
  if (d == 1)
    f[g++] = a;
  (void) (f[0] || (g && h()));
}
//    11 |   (void) (f[0] || (g && h()));
// file.c:21:3: error: ‘*(unsigned int *)(&f[0])’ may be used uninitialized [-Werror=maybe-uninitialized]
//    21 |   (void) (f[0] || (g && h()));

// The error for e1 is correct, but not the one for e2 (for e2, previous GCC versions were outputting ‘f’ instead of ‘*(unsigned int *)(&f[0])’, but this is about the same thing).

//    11 |   (void) (f[0] || (g && h()));
//     5 |   int f[2];
//    21 |   (void) (f[0] || (g && h()));
//    15 |   enum { a } f[2];


