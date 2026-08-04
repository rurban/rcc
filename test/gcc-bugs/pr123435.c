/* GCC Bug #123435 - [15 regression] ICE extern enum with qualifiers and int on the same decl
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=123435
 */
/* { dg-do compile } */


enum E { E1 = -1, E2 = 0, E3 = 1 };
const volatile enum E i2;
extern int i2;
// ```


