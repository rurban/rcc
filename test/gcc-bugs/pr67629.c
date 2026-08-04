/* GCC Bug #67629 - bogus -Wreturn-type in a function with tautological if-else
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67629
 */


int foo (_Bool a) {
    if (a) return 0;
    else if (!a) return 1;
}
// u.c: In function ‘foo’:
// u.c:4:1: warning: control reaches end of non-void function [-Wreturn-type]
 }
//  ^


