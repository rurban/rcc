/* GCC Bug #92261 - syntax errors on __has_builtin (__has_builtin)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=92261
 */


int f (void)
{
  return __has_builtin (__has_builtin);
}
// z.c:3:38: error: missing '(' after "__has_builtin"
//     3 |   return __has_builtin (__has_builtin);
//     3 |   return __has_builtin (__has_builtin);
// z.c:3:23: error: expected ‘;’ at end of input
//     3 |   return __has_builtin (__has_builtin);
//       |                       ;
//     4 | }
//     4 | }


