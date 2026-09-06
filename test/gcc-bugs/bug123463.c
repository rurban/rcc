/* GCC Bug #123463 - ICE in comptypes with some GCC like code after errors
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=123463
 */
/* { dg-do compile } */


enum mm {L};
extern const enum mm m[1];
void f(unsigned egno) {
//     f(m);
}
// ```


