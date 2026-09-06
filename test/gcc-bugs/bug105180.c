/* GCC Bug #105180 - K&R style definition does not evaluate array size
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105180
 */
/* { dg-do run } */

/* Comment 6's self-contained tester: in a K&R-style function definition,
 * the side effects of a parameter's array-size expression (char c[static
 * func()]) must be evaluated on each call.  gcc does not evaluate them
 * at all for K&R definitions (regression since 4.7); this aborts with
 * gcc and should pass with a correct compiler. */
int global = 0;
int func(void)
{
  global++;
  return global;
}
void crime(s, c)
    char *s;
    char c[static func()];
{
}

int main(void)
{
    crime("1", "1");
    if (global != 1)
      __builtin_abort();
    return 0;
}
