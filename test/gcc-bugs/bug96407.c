/* GCC Bug #96407 - LTO inlined functions don't inherit disabled warnings
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=96407
 * NOTE: the actual bug only manifests across two translation units built
 * with -flto (test.c defining test() with the pragma, main.c defining
 * main() calling it); the report explicitly notes that combining them into
 * a single file does *not* reproduce the warning/error, so this cannot be
 * a faithful single-TU reproducer. This file reconstructs the original
 * two-file content merged into one TU for documentation purposes only.
 */


#include <string.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstack-usage="

int
test (int i)
{
  char str[i];
  memset (str, 0, sizeof str);
  return str[0]+i;
}

#pragma GCC diagnostic pop

#include <stdio.h>

extern int test (int i);

int
main (int argc, char *argv[])
{
  int j = test (argc);
  printf ("%d\n", j);
  return 0;
}

//     6 | main (int argc, char *argv[])
// test() function.  So LTO seems to be causing the difference.


