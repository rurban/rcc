/* GCC Bug #117753 - ICE: tree check: expected class ‘type’, have ‘exceptional’ (error_mark) in build_int_cst, at tree.cc:1533
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117753
 */
/* { dg-do compile } */


int n;
void f1(int[n]);
float n();
void f1(int[n]);


