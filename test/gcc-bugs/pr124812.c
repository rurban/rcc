/* GCC Bug #124812 - ICE with invalid __builtin_strlen definition
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124812
 */
/* { dg-do compile } */


#include <stdlib.h>
#include <string.h>

long long __builtin_strlen(const char* str)
{
    long long result = 0;
    while (*str) {
//             ++result;
//             ++str;
    }
    return result;
}
//    - Compiler version: gcc (GCC) 16.0.1 20260407 (experimental)
//     4 | long long __builtin_strlen(const char* str)
long unsigned int
long long int

# VUSE <.MEM_5(D)>
_2 = __builtin_strlen (_8);
// 0x577106b internal_error(char const*, ...)
// 0x235cb82 verify_gimple_in_cfg(function*, bool, bool)
// Please submit a full bug report, with preprocessed source (by using -freport-bug).


