/* GCC Bug #83840 - missing -Wmemset-elt-size with address of array element
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=83840
 */
/* { dg-do compile } */


int a[2];

void f (void)
{
  __builtin_memset (a, 0, 2);   // -Wmemset-elt-size (good)
}

void g (void)
{
  __builtin_memset (&a, 0, 2);   // -Wmemset-elt-size (good)
}

void h (void)
{
  __builtin_memset (&a[0], 0, 2);   // missing -Wmemset-elt-size
}

// z.c: In function ‘void f()’:
// z.c:5:28: warning: ‘memset’ used with length equal to number of elements without multiplication by element size [-Wmemset-elt-size]
//   __builtin_memset (a, 0, 2);   // -Wmemset-elt-size (good)
//                             ^
// z.c: In function ‘void g()’:
// z.c:10:29: warning: ‘memset’ used with length equal to number of elements without multiplication by element size [-Wmemset-elt-size]
//   __builtin_memset (&a, 0, 2);   // -Wmemset-elt-size (good)
//                              ^


