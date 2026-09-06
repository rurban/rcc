/* GCC Bug #85997 - Bogus -Wvla warning from function array argument with size expression
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=85997
 */


void print_array(unsigned count, int array[static count]);

// gives "not_vla.c:3:1: warning: ISO C90 forbids variable length array "array" [-Wvla]"
// The "array" argument is not a variable-length array, the declaration just states that the argument provides access to the first element of an array that has at least "count" elements.
// In a whole working example below two warnings are emitted when compiled with -Wvla (function declaration, line 3, and definition, line 5):
#include <stdio.h>

void print_array(unsigned count, int array[static count]);

void print_array(unsigned count, int array[static count])
{
  for (unsigned i = 0; i < count; ++i)
    {
//       printf("%d ", array[i]);
    }
//   printf("\n");
}

int main(void)
{
  int array[5] = {1, 2, 3, 4, 5};

//   print_array(5, array);

//   // print_array(6, array);
//   //   Error: array overflow. Since 6 and sizeof array are compile time const,
//   //   this could emit a warning, but doesn't.

  return 0;
}


