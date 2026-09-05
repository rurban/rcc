/* GCC Bug #70186 - RFE: better handling of misspelled attributes
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70186
 */
/* { dg-do compile } */


struct S *foo __attribute__ ((visbility("hidden")));

