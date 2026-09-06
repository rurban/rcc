/* GCC Bug #117865 - "error: too many arguments to function" does not show the function type nor which argument is too many (especially w/ C23)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117865
 */
/* { dg-do compile } */


void baz(int (*bar)(int)) {
//     bar(1, 1);
}
// ```
// C front-end:
// ```
// <source>: In function 'baz':
// <source>:3:5: error: too many arguments to function 'bar'
//     3 |     bar(1, 1);
//       |     ^~~
// ```
// C++ front-end:
// ```
// <source>: In function 'void baz(int (*)(int))':
// <source>:3:8: error: too many arguments to function
//     3 |     bar(1, 1);
//       |     ~~~^~~~~~
// ```


