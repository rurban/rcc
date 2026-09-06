/* GCC Bug #88687 - redundant -Wbuiltin-declaration-mismatch after -Wimplicit-function-declaration
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88687
 */
/* { dg-do compile } */


int f (const char *s)
{
  return memcmp (s, "1234", 4);
}
// t.c: In function ‘f’:
// t.c:3:10: warning: implicit declaration of function ‘memcmp’ [-Wimplicit-function-declaration]
//     3 |   return memcmp (s, "1234", 4);
//       |          ^~~~~~
// t.c:3:29: warning: ‘memcmp’ argument 3 type is ‘int’ where ‘long unsigned int’ is expected in a call to built-in function declared without prototype [-Wbuiltin-declaration-mismatch]
//     3 |   return memcmp (s, "1234", 4);
//       |                             ^
// <built-in>: note: built-in ‘memcmp’ declared here


