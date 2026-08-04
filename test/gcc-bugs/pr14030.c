/* GCC Bug #14030 - missing parameter count check ?
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=14030
 */


void f( one, two)
int one;
char two;
{
}

void g(int i)
{
// 	f( 1);
// 	f( 1, 'e', i);
}
// [dcb@localhost src]$ gcc -c bad.c
// [dcb@localhost src]$
// [dcb@localhost src]$ gcc -c -g -O2 -Wall -ansi -pedantic bad.c
// [dcb@localhost src]$
// Surprising.  Here is Intel compiler on the same code. Note the lack
// of extra flags.
// [dcb@localhost src]$ icc -c bad.c
// bad.c(13): warning #165: too few arguments in function call
//         f( 1);
//             ^
// bad.c(14): warning #140: too many arguments in function call
//         f( 1, 'e', i);
//                    ^
// Any chance of getting gcc to find the faults in the code, without
// changing the original code from K & R to ISO ?


