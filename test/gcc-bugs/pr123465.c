/* GCC Bug #123465 - ICE in handle_access_attribute after redeclaration error
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=123465
 */
/* { dg-do compile } */


void a(void *);
int a;
// void
__attribute__((access(read_only, 1))) a(void *);
// ```


