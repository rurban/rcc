/* GCC Bug #107166 - "useless type name in empty declaration" diagnostic may refer to wrong location
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=107166
 */
/* { dg-do compile } */


static int;
_Alignas(int) char;
long long;
//     1 | static int;
//     2 | _Alignas(int) char;
//     3 | long long;

// These could be changed to actually refer to the location of the type specifier(s), or they could be changed to different diagnostics that already exist for empty declarations without a type, such as "useless storage class specifier in empty declaration".


