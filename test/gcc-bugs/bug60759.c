/* GCC Bug #60759 - improve -Wlogical-op
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=60759
 */


int foo(int x);

/* These should generate a warning (constant used with logical op,
   confusion with Perl/Python semantics or a bitwise op typo): */
void warn_cases(void)
{
  int a = foo(1) || foo(2);
  int one = 2 && 3;
}

/* These should not generate a warning: */
void no_warn_cases(void)
{
  while (foo(1) || foo(2));
  int zero = (1 != 2) && (3 == 4);
}

int foo(int x) { return x; }

static int x = 2 || 3;
int main(void) { (void)x; return 0; }


