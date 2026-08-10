/* Multiple x86-64 SysV ABI AND AAPCS64 (ARM64) ABI bugs in `long
 * double` (80-bit x87 extended on x86-64; IEEE binary128 on ARM64)
 * argument/return handling, found via ksh93's `arith.sh` (which calls
 * `atanl`/`cosl`/etc. internally through its own `.sh.math.*` dispatch
 * table) and confirmed against gcc torture's conversion.c and
 * 930622-2.c.
 *
 * rcc represents a `long double` VALUE internally as a plain 64-bit
 * double (a deliberate simplification -- see codegen.c's ND_ASSIGN
 * long-double store path), only widening to genuine extended
 * precision at ABI boundaries (arguments/return values crossing into
 * or out of a real, non-rcc-compiled function). Every one of those
 * boundary-crossing sites had an independent bug that silently skipped
 * the widen/narrow step and used the plain float/double convention
 * instead:
 *
 *  x86-64 SysV:
 *  1. gen_funcall's argument classification only forced a `long
 *     double` argument onto the stack (the ABI's MEMORY class, which
 *     long double *always* uses -- it never occupies an XMM register)
 *     when the argument was in a variadic call's unnamed tail; a
 *     plain, fully-prototyped call like `atanl(long double)` fell
 *     through to the ordinary float path and got an XMM register slot
 *     no caller-side code ever populated for it.
 *  2. gen_funcall's post-call return-value read always assumed
 *     XMM0, but a `long double`-returning function's result comes
 *     back in ST0 (x87), popped via `fstpl`.
 *  3. A function's own `return` statement had the same bug in
 *     reverse: returning a `long double` expression placed the value
 *     in XMM0 instead of widening it onto ST0 via `fldl`, and
 *     separately, returning a plain INTEGER expression as a `long
 *     double` (`long double f(long long n) { return n; }`) computed
 *     the int-to-double conversion correctly but also left it in
 *     XMM0.
 *  4. The function-prologue parameter loader (which reads incoming
 *     parameters from the ABI-specified location) has TWO
 *     independent copies of this exact classification -- one for
 *     Pass 1 (a dry run solely to discover stack layout, its own
 *     copy inert for correctness) and one for Pass 2 (the real
 *     instruction emission) -- and only the Pass-2 copy actually
 *     mattered; it had the identical "only variadic tail" bug,
 *     reading a `long double` parameter from an XMM register instead
 *     of `fldt`-ing it off the stack.
 *
 *  Any ONE of bugs 1/2/3 alone happened to self-cancel for a
 *  *single*-long-double-parameter round trip through rcc-compiled code
 *  calling rcc-compiled code (both sides made the same mistake
 *  consistently) -- what actually exposed bug 4 was a function with
 *  TWO long double parameters (`ldnear(long double x, long double y)`,
 *  lifted directly from gcc torture's conversion.c): the first
 *  parameter's value happened to still be sitting in XMM0 as an
 *  unrelated leftover from the caller's own prior computation, masking
 *  the bug, while the second genuinely read garbage.
 *
 *  AAPCS64 (ARM64), found while cross-checking this same test against
 *  the arm64 cross target -- ARM64's argument classification and
 *  prologue parameter loading already handled `long double` correctly
 *  (they route through a dedicated HFA/long-double path independent of
 *  the ordinary float classification), but return-value handling had
 *  the mirror-image gap of bugs 2/3 above:
 *  5. gen_funcall's post-call return-value read treated a `long
 *     double`-returning call exactly like a plain `double` one --
 *     `fmov x{r}, d0` -- but AAPCS64 returns `long double` as a
 *     genuine 128-bit binary128 quad in v0, not a value narrowed into
 *     d0's low 64 bits; reading only d0 silently reinterpreted
 *     binary128's mantissa/exponent layout as binary64 garbage. Fixed
 *     by narrowing the quad via libgcc's `__trunctfdf2` (the same
 *     routine ND_VA_ARG's long-double narrowing already uses) before
 *     treating the result as an ordinary double.
 *  6. A function's own `return` statement had the same bug in
 *     reverse for BOTH flonum-to-long-double and integer-to-long-
 *     double returns: the value was placed directly in d0 (a plain
 *     double) instead of being widened onto the full v0 quad via
 *     libgcc's `__extenddftf2` (the same routine the argument
 *     pre-pass in gen_funcall already uses) -- the caller's `fmov
 *     x{r}, d0` return-value read (bug 5, above, once fixed) would
 *     then narrow back down from an un-widened, garbage upper half.
 *
 * Windows/mingw x86-64 note: this whole file's `main()` is skipped on
 * _WIN32. Win64 uses a COMPLETELY DIFFERENT convention for `long
 * double` (16 bytes, non-power-of-2-fitting): both arguments AND
 * return values go by HIDDEN POINTER (the exact same "caller
 * allocates, passes/returns an address" mechanism already implemented
 * here for large structs) -- confirmed directly against real
 * mingw-w64-gcc's own generated code for both an argument-taking and a
 * return-value-producing function. rcc's Windows call/return
 * classification (`has_hidden_retbuf`, referenced throughout
 * gen_funcall/the prologue/ND_RETURN) currently only recognizes
 * struct/union/large-complex for this treatment, not `long double` --
 * a real, separate, root-caused gap (not yet fixed; see
 * test/third_party/TODO.md's "Investigated, not fixed" entry), out of
 * scope for this session. */
#include <math.h>
#include <assert.h>

static int close_to_ld(long double a, long double b) {
    long double d = a - b;
    if (d < 0) d = -d;
    return d < 1e-9L;
}

/* Bug 1+2: calling a real (non-rcc) long-double libm function with a
 * single, fully-prototyped (non-variadic) argument. */
static void test_direct_call(void) {
    assert(close_to_ld(fabsl(-2.5L), 2.5L));
    assert(close_to_ld(sqrtl(4.0L), 2.0L));
    assert(close_to_ld(atanl(1.0L), 0.7853981633974483096L));
    assert(close_to_ld(cosl(0.5L), 0.8775825618903727161L));
    assert(close_to_ld(sinl(0.5L), 0.4794255386042030003L));
}

/* Bug 1+2+3, chained: an rcc-compiled wrapper receives a long double
 * parameter, forwards it to a real libm function, and returns that
 * result -- three independent ABI-boundary crossings in one call. */
static long double my_fabsl(long double x) { return fabsl(x); }
static long double my_atanl(long double x) { return atanl(x); }

static void test_wrapper_forward(void) {
    assert(close_to_ld(my_fabsl(-2.5L), 2.5L));
    assert(close_to_ld(my_atanl(1.0L), 0.7853981633974483096L));
}

/* Bug 3 (integer-to-long-double return): converting an integer
 * expression to `long double` in a return statement, for every
 * integer source width/signedness (the actual conversion, before any
 * ABI widening, has its own separate signed/unsigned-64-bit-boundary
 * logic that must also stay correct). */
static long double s2ld(int s) { return s; }
static long double u2ld(unsigned u) { return u; }
static long double sll2ld(long long s) { return s; }
static long double ull2ld(unsigned long long u) { return u; }

static void test_int_to_longdouble_return(void) {
    assert(s2ld(-5) == -5.0L);
    assert(u2ld(0x80000000U) == 2147483648.0L); /* top bit set, unsigned */
    assert(sll2ld(-5LL) == -5.0L);
    assert(ull2ld(0x8000000000000000ULL) == 9223372036854775808.0L); /* top bit set, unsigned long long */
}

/* Bug 4: a function with TWO long double parameters -- the exact
 * shape (lifted from gcc torture's conversion.c) that exposed the
 * prologue's Pass-2 real-emission classification bug; a single
 * parameter alone doesn't reliably reproduce it (see the file
 * comment above). */
static int ldnear(long double x, long double y) {
    long double t = x - y;
    return t == 0 || x / t > 1e33L;
}

/* Three long double parameters: if an earlier one's stack slot
 * silently corrupted a later one's (an off-by-one-slot alignment
 * bug would do exactly this), only a chain of 3+ would reliably
 * surface it. */
static long double sum3ld(long double a, long double b, long double c) {
    return a + b + c;
}

static void test_two_longdouble_params(void) {
    unsigned int all_ones = ~0U;
    long double a = u2ld(all_ones);
    long double b = (long double)all_ones;
    assert(a == b);
    assert(ldnear(a, b));
    assert(close_to_ld(sum3ld(a, a, a), 3.0L * a));
}

int main(void) {
#ifdef _WIN32
    /* Win64's own long-double-by-hidden-pointer ABI isn't implemented
     * yet (see the file comment above) -- nothing to verify here. */
    return 0;
#else
    test_direct_call();
    test_wrapper_forward();
    test_int_to_longdouble_return();
    test_two_longdouble_params();
    return 0;
#endif
}
