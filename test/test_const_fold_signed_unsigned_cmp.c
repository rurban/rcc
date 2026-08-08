// Regression: opt.c's ND_IF constant-condition folding calls
// eval_const_expr() on the condition; its ND_EQ/ND_NE/ND_LT/ND_LE cases
// compared operands as raw 64-bit `long long`s without first truncating
// each side to its own declared type's width. A negative-looking signed
// value that only fits within a NARROWER type than 64 bits (e.g. `(int)
// 0x80000000` == -2147483648, sign-extended to 0xFFFFFFFF80000000 as a
// 64-bit pattern) never compared equal to the bit-identical 32-bit
// unsigned value it represents (0x80000000), because the wider sign
// extension was never re-truncated to the comparison's actual (32-bit)
// common width before the unsigned reinterpretation.
//
// This only manifested at -O1/-O2 (opt.c's dead-branch elimination is the
// only caller that constant-folds an `if` condition at all) - at -O0 the
// comparison is generated as ordinary runtime code and was always
// correct, masking the bug until enum constants started actually being
// typed `unsigned`/`long`/etc. instead of always `int` (see
// test_enum_c23_wide_and_fixed.c).
enum { PU_A = 0x80000000 }; // unsigned (doesn't fit signed int)

int main(void) {
    // Compile-time-constant condition folded by opt.c's ND_IF handling
    // (both operands here are ND_NUM-derived compile-time constants).
    if (PU_A != (int)0x80000000)
        return 1;
    if ((int)0x80000000 != PU_A)
        return 2;
    if (!(PU_A == (int)0x80000000))
        return 3;
    if ((int)0x80000000 < PU_A) // equal values, strict-less must be false
        return 4;
    if (!((unsigned)0x80000000u <= PU_A))
        return 5;
    return 0;
}
