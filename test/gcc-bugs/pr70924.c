/* GCC Bug #70924 - Wrong position for "warning: missing braces around initializer [-Wmissing-braces]"
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70924
 */


struct { int w; struct { int x, y; } ss; } s = { 1, .ss = 2, 3 };
                                                            {    }
 int a[3][1] = { { 0 }, { 1 }, 2 };
                               { }


