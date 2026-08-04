/* GCC Bug #105180 - K&R style definition does not evaluate array size
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105180
 */


int global = 0;
int func(void)
{
//   global++;
  return global;
}
void crime(s, c)
    char *s;
    char c[static func()];
{
}

int main(void)
{
//     crime("1", "1");
    if (global != 1)
      __builtin_abort();
    return 0;
}


