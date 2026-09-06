/* GCC Bug #94428 - Reintroduce -Wzero-length-array
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=94428
 */
/* { dg-do compile } */


char data[0];
//    20 |     char data[0];


