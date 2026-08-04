/* GCC Bug #105875 - Toggling an atomic_bool is inefficient
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105875
 */
/* { dg-do compile } */
/* { dg-options "-O3 -std=c11" } */

#include <stdatomic.h>

atomic_bool b;
atomic_char c;
_Bool b2;

void f1(void) {
    b ^= 1;   /* misses-optimization: expands to cmpxchg loop instead of
                 lock xorb $1, b(%rip) */
}

void f2(void) {
    c ^= 1;   /* lock xorb $1, c(%rip) - fine */
}

void f3(void) {
    b2 ^= 1;  /* xorb $1, b2(%rip) - fine */
}

/* Using __atomic_xor_fetch_1 directly produces the ideal code:
   __atomic_xor_fetch_1 (&b, 1, 5);  =>  lock xorb $1, b(%rip) */