/* GCC Bug #52952 - Wformat location info is bad (wrong column number)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=52952
 */
/* { dg-do compile } */


extern int printf (__const char *__restrict __format, ...);
void f(void) {
//     printf("%.*d");
}


