/* Regression test: x86-64 SysV function returning a wide _BitInt
 * (size > 16, hidden-return-buffer convention) read its own incoming
 * scalar parameters from the wrong ABI registers.
 *
 * The SysV x86-64 ABI treats the hidden struct/complex/wide-_BitInt
 * return pointer as consuming RDI, shifting every real argument one
 * GP register right (arg0 in RSI, arg1 in RDX, ...). The CALLER side
 * (gen_funcall's has_hidden_retbuf/gp_reg_args) already accounted for
 * _BitInt correctly, but the CALLEE's own prologue had TWO separate,
 * independently-duplicated copies of the "does this return type shift
 * my incoming params by one register?" condition (codegen.c generates
 * every function body twice: a dry-run pass to size the stack frame,
 * then the real emission pass, each with its own local copy of this
 * check) -- neither included the TY_BITINT>16 case, only STRUCT/UNION/
 * COMPLEX. A function returning e.g. `_BitInt(216)` therefore read its
 * first real parameter from RDI (the hidden pointer) instead of RSI,
 * shifting every parameter down by one and losing the last one
 * entirely.
 *
 * Without the fix: `a` reads garbage (the retbuf pointer truncated to
 * int), `b` reads what should have been `a`, `c` reads what should
 * have been `b`, and the real `c` value never reaches the function body.
 */
#include <stdio.h>

typedef _BitInt(216) State;

static State make_mask(int a, int b, int c) {
    unsigned _BitInt(72) bit = 1;
    State msk;
    msk = bit << a;
    msk <<= 72;
    msk |= bit << b;
    msk <<= 72;
    msk |= bit << c;
    return msk;
}

int main(void) {
    State m = make_mask(5, 10, 15);
    unsigned long long lo = (unsigned long long)(m & (State)0xFFFFFFFFFFFFFFFFULL);
    State mid_shift = m >> 72;
    unsigned long long mid = (unsigned long long)(mid_shift & (State)0xFFFFFFFFFFFFFFFFULL);
    State hi_shift = m >> 144;
    unsigned long long hi = (unsigned long long)(hi_shift & (State)0xFFFFFFFFFFFFFFFFULL);

    int failed = 0;
    if (lo != (1ULL << 15)) { printf("FAIL: lo=%llx want %llx\n", lo, 1ULL << 15); failed = 1; }
    if (mid != (1ULL << 10)) { printf("FAIL: mid=%llx want %llx\n", mid, 1ULL << 10); failed = 1; }
    if (hi != (1ULL << 5)) { printf("FAIL: hi=%llx want %llx\n", hi, 1ULL << 5); failed = 1; }

    if (failed) {
        printf("FAILED\n");
        return 1;
    }
    printf("PASSED\n");
    return 0;
}
