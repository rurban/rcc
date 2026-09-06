/* GCC Bug #95157 - Missing -Wtautological-compare warning
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=95157
 */


// NOTE: reporter's original testcase used "volatile short insn;" but that
// was pointed out (comment #1) to make the tautological-compare warning
// technically incorrect for volatile accesses; comment #2 asked for the
// testcase to be considered without volatile, which is the actual
// still-missing-warning case (comment #3: confirmed).
short insn;

int main()
{
  if (insn < 0 && insn > 3) {
      return 1;
  } else {
      return 0;
  }
}

