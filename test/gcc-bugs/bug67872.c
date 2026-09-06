/* GCC Bug #67872 - missing -Warray-bounds warning, bogus -Wmaybe-uninitialized
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67872
 */
/* { dg-do compile } */


struct A {
    int a[3];
} a;

int foo (void)
{
    return __builtin_offsetof (struct A, a[4]);
}

