/* GCC Bug #71852 - add warning for conditions that can never be true (missed -Wtautological-compare?)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71852
 */


int main(int argc, char**argv)
{
  unsigned int aa = argc;

  aa = aa & 0x04;

  if (aa == 0x00) {
//     // Ok.
  } else if (aa == 0x01) {
//     // Ok.
  } else if (aa == 0x0b) {
//     // Oops.
  }
}
// Here, two of the conditions can never be true.
// I think it would be nice if gcc could warn for this case.


