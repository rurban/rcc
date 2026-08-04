/* GCC Bug #23144 - invalid parameter forward declarations not diagnosed
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=23144
 */
/* { dg-do compile } */


int g1(int a;);
int g2(int a; __attribute__((unused)));
int g3(int;);
int g4(int, long;);


