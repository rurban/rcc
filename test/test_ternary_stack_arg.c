// Regression: a ternary expression used as a stack-passed function argument
// (7th+ arg on x86-64 SysV) was miscompiled.  When one branch (e.g. `f & ~X`)
// spilled its lhs and the rhs reused the same physical register, the binary
// op became a self-operation (`and %rsi,%rsi`) and the spilled lhs was
// discarded, so the whole ternary yielded the wrong branch value.  Only `add`
// had spill-aware handling; and/or/xor/sub/imul/cmp did not.
//
// Seen in rcc-compiled perl5 regcomp_study.c:Perl_study_chunk, where the
// recursion's flags argument `(mincount==0 ? (f & ~SCF_DO_SUBSTR) : f)` (a
// stack arg) passed the wrong value, making optional quantifiers (a?) behave
// as required and corrupting regex substring optimization.
#include <assert.h>

__attribute__((noinline))
static long sink10(long a1, long a2, long a3, long a4, long a5, long a6,
                   long a7, long a8, long a9, long v10, long a11, long a12) {
    (void)a1;(void)a2;(void)a3;(void)a4;(void)a5;(void)a6;
    (void)a7;(void)a8;(void)a9;(void)a11;(void)a12;
    return v10;
}

int main(void) {
    long mincount = 0;
    long f = 0x2C00;
    long X = 0x400;

    // ternary (with a bitwise-AND branch) as the 10th (stack) argument
    long r_and = sink10(1,2,3,4,5,6,7,8,9, (mincount==0 ? (f & ~X) : f), 11,12);
    assert(r_and == (f & ~X)); /* 0x2800 */

    // exercise the other non-add spill paths too
    long r_or  = sink10(1,2,3,4,5,6,7,8,9, (mincount==0 ? (f | ~X) : f), 11,12);
    assert(r_or == (f | ~X));
    long r_xor = sink10(1,2,3,4,5,6,7,8,9, (mincount==0 ? (f ^ ~X) : f), 11,12);
    assert(r_xor == (f ^ ~X));
    long r_sub = sink10(1,2,3,4,5,6,7,8,9, (mincount==0 ? (f - ~X) : f), 11,12);
    assert(r_sub == (f - ~X));

    // false branch must still work
    mincount = 1;
    long r_false = sink10(1,2,3,4,5,6,7,8,9, (mincount==0 ? (f & ~X) : f), 11,12);
    assert(r_false == f);

    return 0;
}
