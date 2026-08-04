/* GCC Bug #84595 - Add __builtin_break() built-in for a breakpointing mechanism
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=84595
 */


#include <stdio.h>

int main()
{
 __builtin_trap();
// 	printf("hello world\n");
 return 0;
}
// Then
main:
//   ud2
// I propose, as a feature, either to provide a new command line option to control whether the code is optimized out or preserved, or to a add a new builtin function.


