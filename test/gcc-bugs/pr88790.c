/* GCC Bug #88790 - No warning for misleading indentation
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88790
 */


void f(void)
{
  }
}
// ===
// (as Daniel (on cc:) found), but not even for
// ===
void f(void)
{
  }
// ===
// Is there a reason for that?


