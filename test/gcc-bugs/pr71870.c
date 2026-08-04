/* GCC Bug #71870 - wrong location of "%n$" directive in -Wformat
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71870
 */


char d [4];

void f (void)
{
  __builtin_sprintf (d, "%r");

  __builtin_sprintf (d, "%2$i%1$i", 1, 234);
}
// xyz.c: In function ‘f’:
// xyz.c:5:27: warning: unknown conversion type character ‘r’ in format [-Wformat=]
   __builtin_sprintf (d, "%r");
//                            ^
// xyz.c:7:3: warning: ISO C does not support %n$ operand number formats [-Wformat]
   __builtin_sprintf (d, "%2$i%1$i", 1, 234);
//    ^~~~~~~~~~~~~~~~~


