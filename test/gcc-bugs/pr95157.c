/* GCC Bug #95157 - Missing -Wtautological-compare warning
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=95157
 */


volatile short insn;

int main()
{
  if (insn < 0 && insn > 3) {
      return 1;
  } else {
      return 0;
  }
}
  if (insn < 0 && insn > 3) {
  if (insn < 0 && insn > 3) {


