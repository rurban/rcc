/* GCC Bug #45464 - Warning missing using -Wall when comparing unsigned int and sum of unsigned chars.
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=45464
 */


int main()
{
  unsigned char a=0;
  unsigned int b =0;
  bool test1 =( b < a  + a);//no warning, why
  bool test2 =( b < a + a + a);//warning
  if (wtf1 && wtf2) return 1;
}


