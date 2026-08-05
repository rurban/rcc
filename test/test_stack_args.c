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
//
// It was eventually a off-by-one wrong staging check: stack_scratch >= NUM_REGS

#include <assert.h>

// --- test 1: ternary with bitwise-op branches as a deep stack argument ---

__attribute__((noinline))
static long sink10(long a1, long a2, long a3, long a4, long a5, long a6,
                   long a7, long a8, long a9, long v10, long a11, long a12) {
    (void)a1;(void)a2;(void)a3;(void)a4;(void)a5;(void)a6;
    (void)a7;(void)a8;(void)a9;(void)a11;(void)a12;
    return v10;
}

static int test_ternary_stack(void) {
    long mincount = 0;
    long f = 0x2C00;
    long X = 0x400;

    long r_and = sink10(1,2,3,4,5,6,7,8,9, (mincount==0 ? (f & ~X) : f), 11,12);
    assert(r_and == (f & ~X));

    long r_or  = sink10(1,2,3,4,5,6,7,8,9, (mincount==0 ? (f | ~X) : f), 11,12);
    assert(r_or == (f | ~X));
    long r_xor = sink10(1,2,3,4,5,6,7,8,9, (mincount==0 ? (f ^ ~X) : f), 11,12);
    assert(r_xor == (f ^ ~X));
    long r_sub = sink10(1,2,3,4,5,6,7,8,9, (mincount==0 ? (f - ~X) : f), 11,12);
    assert(r_sub == (f - ~X));

    mincount = 1;
    long r_false = sink10(1,2,3,4,5,6,7,8,9, (mincount==0 ? (f & ~X) : f), 11,12);
    assert(r_false == f);

    return 0;
}

// --- test 2: last register arg and first stack arg share a sub-expression ---
//
// The last register-passed argument (r9 on x86-64, 6th GP arg reg) shares a
// repeated pointer-arithmetic sub-expression with the immediately-following
// stack argument.  Evaluating the stack arg under register pressure corrupts
// the already-computed register arg unless staging is triggered.  Before the
// >= NUM_REGS fix, when live_now==0 the count was exactly 6 reg args + 2
// scratch == 8 == NUM_REGS, *not* > NUM_REGS, so staging was skipped and the
// register value was silently lost.
//
// On aarch64 NUM_REGS==12 and there are 8 GP arg regs; bump the count to
// match the same boundary.
#ifdef __aarch64__
__attribute__((noinline))
static unsigned long sink_r9(unsigned long a1, unsigned long a2, unsigned long a3,
                              unsigned long a4, unsigned long a5, unsigned long a6,
                              unsigned long a7, unsigned long a8,
                              unsigned long a9, unsigned long a10)
{
    (void)a1;(void)a2;(void)a3;(void)a4;(void)a5;(void)a6;(void)a7;(void)a8;
    return a10 == a9 ? a9 : 0;
}
#else
__attribute__((noinline))
static unsigned long sink_r9(unsigned long a1, unsigned long a2, unsigned long a3,
                              unsigned long a4, unsigned long a5,
                              unsigned long a6, unsigned long a7)
{
    (void)a1;(void)a2;(void)a3;(void)a4;(void)a5;
    return a7 == a6 ? a6 : 0;
}
#endif

static int test_regarg_stack_shared_subexpr(void)
{
    static const unsigned char buf[64] = {0};
    const unsigned char *tI = buf, *tC = buf, *eI = buf + 23;
    const unsigned char *xC = buf + 23;

    // Both the last register arg and first stack arg evaluate
    // xI(xC)=tI+(xC-tC) INLINE as ternaries sharing the same
    // sub-expression.  Pre-computing into locals before the call
    // side-steps the register-pressure path that triggers the
    // corruption — the args must be inline ternaries.
#ifdef __aarch64__
    unsigned long r = sink_r9(1, 2, 3, 4, 5, 6, 7, 8,
        ((xC > eI) ? (unsigned long)0 : (unsigned long)(tI + (xC - tC))),
        ((xC > eI) ? (unsigned long)eI : (unsigned long)(tI + (xC - tC))));
#else
    unsigned long r = sink_r9(1, 2, 3, 4, 5,
        ((xC > eI) ? (unsigned long)0 : (unsigned long)(tI + (xC - tC))),
        ((xC > eI) ? (unsigned long)eI : (unsigned long)(tI + (xC - tC))));
#endif
    unsigned long expected = (unsigned long)(tI + (xC - tC));
    assert(r == expected);
    return 0;
}

int main(void)
{
    int f0 = test_ternary_stack();
    int f1 = test_regarg_stack_shared_subexpr();
    return f0 | f1;
}
