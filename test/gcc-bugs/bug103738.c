/* GCC Bug #103738 - No warning when setting deprecated fields using designated initializers
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=103738
 */
/* { dg-do compile } */


struct foo foo;
//     foo.bar = 5;
//     8 |     foo.bar = 5;
//     2 |     int bar __attribute((deprecated));


