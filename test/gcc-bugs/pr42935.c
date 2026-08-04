/* GCC Bug #42935 - warn if a binary operation is performed on a type but the result is then casted to a larger type
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=42935
 */


#include <stdio.h>
unsigned val1 = 0x10000000, val2 = 0x100;
int main(int argc, char **argv)
{
    unsigned long long val3 = val1 * val2;
//     printf ("val1 = 0x%X, val2 = 0x%X, val3 = 0x%llX, val1*val2 = 0x%llX\n",
//         val1, val2, val3, (unsigned long long)(val1*val2));
}
// Obviously the line printed is:
// val1 = 0x10000000, val2 = 0x100, val3 = 0x0, val1*val2 = 0x0
// (compiled with "gcc -w -Wall -O2 tmp.c" you get no warning)
// Got bitten yesterday by gcc-4.4.3, reproduced today on gcc version 4.1.2.


