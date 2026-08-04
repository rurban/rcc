/* GCC Bug #88727 - Diagnostics improvement: Detection of undefined behaviour. Incomplete type in tenative definition with internal linkage. [-Wtentative-definition-incomplete-type]
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88727
 */


static struct S s;

  int main()
  {
  }

  struct S { int x; };
// Compilation command line:
// Observed behaviour:
//   No error messages outputes.
// Possible improvement of behaviour:
//   Outputing an error message about using an incomplete type in the tenative
  definition  static struct S s; .
//   The program has undefined behaviour becuase of a violation of 6.9.2/2:
//   "If the declaration of an identifier for an object is a tentative definition
//    and has internal linkage, the declared type shall not be an incomplete type."
//   GCC detects such undefined behaviour in other cases (for example using the 
//   incomplete type int []). It would be good if it could also hande the case in
//   the test case for this bug report.
Note:
//   Clang detects the undefined behaviour for this program and outputs an error
//   message.


