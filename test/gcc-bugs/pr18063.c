/* GCC Bug #18063 - Gcc doesn't check overflowed size of structure
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=18063
 */


struct a {
    char x[0x7fffffff];
    char b[0x7fffffff];
    char c[3];
};

int main()
{
  __builtin_printf ("%zu\n", sizeof (struct a));
  _Static_assert (sizeof (struct a) > sizeof ((struct a*)0)->x, "");
}
// uu.c: In function ‘main’:
// uu.c:10:37: warning: expression in static assertion is not an integer constant expression [-Wpedantic]
   _Static_assert (sizeof (struct a) > sizeof ((struct a*)0)->x, "");
//                    ~~~~~~~~~~~~~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~
// uu.c:10:3: error: static assertion failed: ""
   _Static_assert (sizeof (struct a) > sizeof ((struct a*)0)->x, "");
//    ^~~~~~~~~~~~~~


