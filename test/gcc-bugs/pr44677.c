/* GCC Bug #44677 - Warn for variables incremented but not used (+=, ++)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=44677
 */
/* { dg-do compile } */


void f0 (int *p)
{
//   p = 0;           // -Wunused-but-set-parameter (expected)
}

void f1 (int *p)
{
  p += 1;          // missing warning
}

void f2 (int *p)
{
  p = p + 1;       // missing warning
}

void f3 (int *p)
{
//   ++p;             // missing warning
}
// <a class="bz_bug_link 
//           bz_status_RESOLVED  bz_closed"
//    title="RESOLVED DUPLICATE - missing -Wunused-but-set-parameter for compound assignment"
//    href="show_bug.cgi?id=95217">pr95217</a>.c:8:3: warning: Value stored to 'p' is never read
  p += 1;          // missing warning
//   ^    ~
// <a class="bz_bug_link 
//           bz_status_RESOLVED  bz_closed"
//    title="RESOLVED DUPLICATE - missing -Wunused-but-set-parameter for compound assignment"
//    href="show_bug.cgi?id=95217">pr95217</a>.c:13:3: warning: Value stored to 'p' is never read
  p = p + 1;       // missing warning
//   ^   ~~~~~
// 2 warnings generated.


