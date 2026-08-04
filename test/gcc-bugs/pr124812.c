/* GCC Bug #124812 - ICE with invalid __builtin_strlen definition
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124812
 */
/* { dg-do compile } */
/* { dg-options "-O3" } */

#include <stdlib.h>
#include <string.h>

long long __builtin_strlen(const char* str)
{
    long long result = 0;
    while (*str) {
            ++result;
            ++str;
    }
    return result;
}

// At -O3, loop distribution (ldist) recognizes this hand-written loop as
// the strlen idiom and rewrites it into a call to the "strlen" builtin,
// which is looked up by name and so resolves to this redefined
// __builtin_strlen (long long return type instead of the builtin's real
// size_t/long unsigned int).  That produces a GIMPLE call whose LHS type
// doesn't match the callee's declared return type, which
// verify_gimple_in_cfg() rejects with an ICE:
//   error: invalid conversion in gimple call
//   internal compiler error: verify_gimple failed
// Per comment 3 (Richard Biener): this is a front-end issue for not
// rejecting the invalid __builtin_strlen redefinition outright; renaming it
// to a non-builtin name like "strlen" avoids the ICE.  Per comment 4
// (Jakub Jelinek): rejecting *all* __builtin_* redefinitions isn't safe
// either, since glibc/libatomic legitimately redeclare some of them.


