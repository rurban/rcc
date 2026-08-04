/* GCC Bug #92086 - Provide way to avoid saving callee-saved registers in functions without callers
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=92086
 */
/* { dg-do compile } */


int f1 (int);

__attribute__ ((noreturn, nothrow))
// f2 (void)
{
  int x1 = f1 (1);
  int x2 = f1 (2);
  int x3 = f1 (3);
  int x4 = f1 (4);
  f1 (x1);
  f1 (x2);
  f1 (x3);
  f1 (x4);
  __builtin_unreachable ();
}

// yields this on x86-64 (with GCC 9):
// If it is not possible to unwind into the caller of f2 (say because it does not exist), there is no impact on debugging experience because the saved values are useless even for debugging.

// I've reported this bug against the C front end because we may need a new attribute for this.  (If noreturn+nothrown cannot be repurposed.)


