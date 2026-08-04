/* GCC Bug #77817 - -Wimplicit-fallthrough: cpp directive renders FALLTHRU comment ineffective
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=77817
 */


> void foo(int i) {
>   switch (i) {
>   case 1: {
>   }


