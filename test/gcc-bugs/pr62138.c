/* GCC Bug #62138 - Poor error recovery when parsing for-loops
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=62138
 */


void foo(void)
{
  for (int k, k < 20; k++);
}
// manuel@gcc10:~$ ~/test1/213518M/build/gcc/cc1 parseerr.c -std=c99
// parseerr.c: In function ‘foo’:
// parseerr.c:3:17: error: expected ‘=’, ‘,’, ‘;’, ‘asm’ or ‘__attribute__’ before ‘<’ token
   for (int k, k < 20; k++);
//                  ^
// parseerr.c:3:26: error: expected ‘;’ before ‘)’ token
   for (int k, k < 20; k++);
//                           ^
// The first error doesn't make any sense.


