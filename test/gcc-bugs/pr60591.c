/* GCC Bug #60591 - Report enum conversions as part of Wconversion
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=60591
 */


enum xpto
{
//   A = 0,
//   B = 1,
//   X = 512
};
extern void print (unsigned int);

unsigned char bar (enum xpto a)
{
   return a;
}
// We don't get currently a warning for this return conversion if we use --short-enums. With -O2 --short-enums, sizeof enum xpto == 2, but sizeof unsigned char == 1, therefore we should warn the user there's loss of precision.


