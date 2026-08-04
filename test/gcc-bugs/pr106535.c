/* GCC Bug #106535 - GCC doesn't reject non-constant initializer if -pedantic is specified but does so in any other circumstances
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=106535
 */


int f = (0, 0);
//     1 | int f = (0, 0);
//     1 | int f = (0, 0);


