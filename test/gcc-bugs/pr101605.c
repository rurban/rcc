/* GCC Bug #101605 - bogus -Wvla-parameter in same bound expression with differently named parameters
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=101605
 */


void f (int m, int (*)[m]);
// void f (int n, int (*)[n]) { }   // bogus warning with -fsanitize=undefined

void g (int m, int (*)[m + 1]);
void g (int n, int (*)[n + 1]) { }   // bogus warning either way
// + gcc -S -Wall u.c
// u.c:5:16: warning: mismatch in bound 1 of argument 2 declared as ‘int (*)[n + 1]’ [-Wvla-parameter]
//     5 | void g (int n, int (*)[n + 1]) { }   // bogus warning either way
//       |                ^~~~~~~~~~~~~~
// u.c:4:16: note: previously declared as ‘int (*)[m + 1]’
//     4 | void g (int m, int (*)[m + 1]);
//       |                ^~~~~~~~~~~~~~
// + gcc -S -Wall -fsanitize=undefined u.c
// u.c:2:16: warning: mismatch in bound 1 of argument 2 declared as ‘int (*)[(long int)(n) - 1]’ [-Wvla-parameter]
//     2 | void f (int n, int (*)[n]) { }   // bogus warning with -fsanitize=undefined
//       |                ^~~~~~~~~~
// u.c:1:16: note: previously declared as ‘int (*)[(long int)(m) - 1]’
//     1 | void f (int m, int (*)[m]);
//       |                ^~~~~~~~~~
// u.c:5:16: warning: mismatch in bound 1 of argument 2 declared as ‘int (*)[(long int)(n + 1) - 1]’ [-Wvla-parameter]
//     5 | void g (int n, int (*)[n + 1]) { }   // bogus warning either way
//       |                ^~~~~~~~~~~~~~
// u.c:4:16: note: previously declared as ‘int (*)[(long int)(m + 1) - 1]’
//     4 | void g (int m, int (*)[m + 1]);
//       |                ^~~~~~~~~~~~~~


