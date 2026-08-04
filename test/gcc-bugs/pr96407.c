/* GCC Bug #96407 - LTO inlined functions don't inherit disabled warnings
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=96407
 */


#include <string.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstack-usage="

// test (int i)
{
  char str[i];
  memset (str, 0, sizeof str);
  return str[0]+i;
}

#pragma GCC diagnostic pop

#include <stdio.h>

extern int test (int i);

// main (int argc, char *argv[])
{
  int j = test (argc);
  printf ("%d\n", j);
  return 0;
}

//     6 | main (int argc, char *argv[])
// test() function.  So LTO seems to be causing the difference.


