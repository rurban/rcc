/* GCC Bug #108660 - Wrong location for first statement of for loop (-Wunused-value)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=108660
 */


void t1(int t)
{
  for (t; t < 10; t++)
    ;
}
// ```
// This should warn about a statement with no effect and GCC does but the location marker is incorrect. The C front-end underlines the for keyword rather than the expression `t`.
// ```
// <source>: In function 't1':
// <source>:3:3: warning: statement with no effect [-Wunused-value]
//     3 |   for (t; t < 10; t++)
//       |   ^~~
// ```
// Note the C++ front-end gets it correct:
// ```
// <source>: In function 'void t1(int)':
// <source>:3:8: warning: statement has no effect [-Wunused-value]
//     3 |   for (t; t < 10; t++)
//       |        ^
// ```


