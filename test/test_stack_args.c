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

// --- test 3: arm64 variadic call with NGRN/NSRN exhausted independently ---
//
// AAPCS64 tracks GP (NGRN) and FP (NSRN) argument-register consumption with
// INDEPENDENT counters. A call whose named parameters exhaust one register
// class without exhausting the other leaves a real register slot open in
// the exhausted-looking class for the first variadic argument of that
// class: 7 named ints fill x0-x6 (NGRN=7, x7 still free) while 9 named
// doubles fill v0-v7 and spill the 9th to the stack (NSRN=8, unrelated to
// NGRN) -- the first variadic int must still go in x7.
//
// Getting this right requires gen_funcall to hold MANY argument values
// (19 total here) live simultaneously before placing them, which forced
// alloc_reg() to spill an earlier, still-needed argument's register to
// make room for a later one. alloc_reg()'s spill mechanism assumes
// strictly nested (LIFO) register lifetimes, an invariant this wide a
// call's simultaneously-live argument set violates: a later free_reg() on
// the stolen register silently restored whatever OTHER value the spill
// was protecting, discarding the argument's actual value without it ever
// going through its own spill/restore cycle. The first variadic int
// silently read back as 0 instead of 8. Found via GCC torture tests
// va-arg-7.c / va-arg-15.c.
#include <stdarg.h>

static int result_a, result_b, result_c;

__attribute__((noinline)) static void
variadic_sink(int i1, int i2, int i3, int i4, int i5, int i6, int i7,
              double f1, double f2, double f3, double f4, double f5,
              double f6, double f7, double f8, double f9, ...) {
    (void)i1;(void)i2;(void)i3;(void)i4;(void)i5;(void)i6;(void)i7;
    (void)f1;(void)f2;(void)f3;(void)f4;(void)f5;(void)f6;(void)f7;(void)f8;(void)f9;
    va_list ap;
    va_start(ap, f9);
    result_a = va_arg(ap, int);
    result_b = va_arg(ap, int);
    result_c = va_arg(ap, int);
    va_end(ap);
}

static int test_variadic_ngrn_nsrn_independent(void) {
    variadic_sink(1, 2, 3, 4, 5, 6, 7,
                  1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0,
                  8, 9, 10);
    assert(result_a == 8);
    assert(result_b == 9);
    assert(result_c == 10);
    return 0;
}

// --- test 4: many ordinary low-arity calls must NOT trigger staging ---
//
// Regression guard for the fix to test 3: an EARLIER attempt staged every
// register-destined call argument unconditionally (no register-pressure
// gate at all), which broke ordinary multi-statement functions making
// several modest, unrelated calls -- e.g. a tinycc torture test with three
// sequential control-flow blocks (if/switch/while), each guarding a
// 4-argument printf call, segfaulted when compiled as a single function
// (main_1() called from main()), even though no individual call came
// anywhere near register exhaustion. Root cause not fully isolated;
// gating staging behind real register pressure (test 3's fix) sidesteps
// it by keeping ordinary calls like these on the untouched path.
static int probe_calls;
__attribute__((noinline)) static int probe(int a, int b, int c, int d, int e) {
    return a + b + c + d + e;
}
static int test_many_low_pressure_calls(void) {
    int sum = 0;
    for (int i = 0; i < 9; i++) {
        sum += probe(i, i + 1, i + 2, i + 3, i + 4);
        probe_calls++;
    }
    assert(probe_calls == 9);
    assert(sum == 9 * (0 + 1 + 2 + 3 + 4) + 5 * (0 + 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8));
    return 0;
}
int main(void)
{
    int f0 = test_ternary_stack();
    int f1 = test_regarg_stack_shared_subexpr();
    int f2 = test_variadic_ngrn_nsrn_independent();
    int f3 = test_many_low_pressure_calls();
    return f0 | f1 | f2 | f3;
}
