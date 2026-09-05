/* GCC Bug #123429 - GCC trunk segfaults while parsing a file-scope __builtin_c23_va_start
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=123429
 */
/* { dg-do compile } */


typedef __builtin_va_list __gnuc_va_list;
typedef __gnuc_va_list va_list;
__gnuc_va_list t;
int const_1 = __builtin_c23_va_start(t, t); /* { dg-error "unsupported global initializer" } gcc still ICEs (segfault) on this
                                               file-scope use - the bug */


