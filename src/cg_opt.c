// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Cheap machine-independent-ish codegen strength reductions that don't
// need SSA/dataflow analysis (see TODO's "-O2 passes ... no need for SSA
// analysis (yet)"). Currently: replacing a runtime idiv/div (x86) or
// sdiv/udiv (ARM64) by a compile-time-constant divisor with shifts and/or
// a magic-number multiply -- the classic Hacker's Delight / LLVM
// DivisionByConstantInfo technique gcc and clang both use at -O1+.
//
// The magic-number math (magicu32/64, magic32/64 below) is a literal port
// of LLVM's UnsignedDivisionByConstantInfo::get() and
// SignedDivisionByConstantInfo::get() (llvm/lib/Support/
// DivisionByConstantInfo.cpp, itself "Hacker's Delight", Henry S. Warren,
// Jr., Chapter 10), restricted to the plain (no pre-shift-widening) form.
// Verified standalone (independent of this file) against real `/`/`%`
// for every non-power-of-two 32-bit divisor in [3,2000] and every signed
// divisor in [-2000,2000], every 64-bit power of two, several large
// (>32-bit-magic) 64/32-bit divisors, and randomized dividends including
// INT32_MIN/INT64_MIN/-1/0/UINT*_MAX boundaries: 5.4M div+mod checks, 0
// failures. The power-of-two shift sequences (unsigned: plain shift;
// signed: arithmetic-shift-with-bias correction) were verified the same
// way for every shift amount 1..31 (32-bit) and 1..63 (64-bit), both
// divisor signs: 134k checks, 0 failures.

#include "rcc.h"
#include "asm.h"
#include "codegen_asm.h"

// ---------------------------------------------------------------------
// Magic-number computation (pure integer math, no codegen dependencies)
// ---------------------------------------------------------------------

typedef struct {
    uint32_t M;
    int a;
    int s;
    int preshift;
} MagicU32;
typedef struct {
    int32_t M;
    int s;
} MagicS32;
typedef struct {
    uint64_t M;
    int a;
    int s;
    int preshift;
} MagicU64;
typedef struct {
    int64_t M;
    int s;
} MagicS64;

static MagicU32 magicu32_inner(uint32_t d, unsigned leading_zeros) {
    MagicU32 r;
    r.a = 0;
    uint32_t allones = (leading_zeros >= 32) ? 0 : (0xFFFFFFFFu >> leading_zeros);
    uint32_t signedmin = 0x80000000u;
    uint32_t signedmax = 0x7FFFFFFFu;
    uint32_t nc = allones - (uint32_t)((allones + 1 - d) % d);
    unsigned p = 31;
    uint32_t q1 = signedmin / nc, r1 = signedmin % nc;
    uint32_t q2 = signedmax / d, r2 = signedmax % d;
    uint32_t delta;
    do {
        p = p + 1;
        if (r1 >= nc - r1) {
            q1 = q1 * 2 + 1;
            r1 = r1 * 2 - nc;
        } else {
            q1 = q1 * 2;
            r1 = r1 * 2;
        }
        if (r2 + 1 >= d - r2) {
            if (q2 >= signedmax) r.a = 1;
            q2 = q2 * 2 + 1;
            r2 = r2 * 2 + 1 - d;
        } else {
            if (q2 >= signedmin) r.a = 1;
            q2 = q2 * 2;
            r2 = r2 * 2 + 1;
        }
        delta = d - 1 - r2;
    } while (p < 64 && (q1 < delta || (q1 == delta && r1 == 0)));
    r.M = q2 + 1;
    r.s = (int)(p - 32);
    r.preshift = 0;
    return r;
}

// Unsigned magic-multiply constants for dividing a `bits`-wide unsigned
// value by d (d not 0, not 1, not a power of two -- those are handled
// separately by the caller). Runtime sequence (n = dividend):
//   if (preshift) n >>= preshift;             // logical
//   q = high_half(n * M);                      // mulhu
//   if (a) q = q + ((n - q) >> 1);              // add-back correction
//   q >>= s;                                    // logical
// (q is now the quotient). See magicu32_inner()'s "even divisor" note in
// magicu32() for why `a` can only be set when preshift is still 0.
static MagicU32 magicu32(uint32_t d) {
    MagicU32 r = magicu32_inner(d, 0);
    if (r.a && !(d & 1)) {
        // D is even: recurse on the odd part so the runtime sequence
        // never needs the add-back correction (matches LLVM's
        // AllowEvenDivisorOptimization path).
        int preshift = __builtin_ctz(d);
        MagicU32 r2 = magicu32_inner(d >> preshift, (unsigned)preshift);
        r2.preshift = preshift;
        return r2;
    }
    if (r.a) r.s -= 1;
    return r;
}

static MagicU64 magicu64_inner(uint64_t d, unsigned leading_zeros) {
    MagicU64 r;
    r.a = 0;
    uint64_t allones = (leading_zeros >= 64) ? 0 : (0xFFFFFFFFFFFFFFFFull >> leading_zeros);
    uint64_t signedmin = 0x8000000000000000ull;
    uint64_t signedmax = 0x7FFFFFFFFFFFFFFFull;
    uint64_t nc = allones - ((allones + 1 - d) % d);
    unsigned p = 63;
    uint64_t q1 = signedmin / nc, r1 = signedmin % nc;
    uint64_t q2 = signedmax / d, r2 = signedmax % d;
    uint64_t delta;
    do {
        p = p + 1;
        if (r1 >= nc - r1) {
            q1 = q1 * 2 + 1;
            r1 = r1 * 2 - nc;
        } else {
            q1 = q1 * 2;
            r1 = r1 * 2;
        }
        if (r2 + 1 >= d - r2) {
            if (q2 >= signedmax) r.a = 1;
            q2 = q2 * 2 + 1;
            r2 = r2 * 2 + 1 - d;
        } else {
            if (q2 >= signedmin) r.a = 1;
            q2 = q2 * 2;
            r2 = r2 * 2 + 1;
        }
        delta = d - 1 - r2;
    } while (p < 128 && (q1 < delta || (q1 == delta && r1 == 0)));
    r.M = q2 + 1;
    r.s = (int)(p - 64);
    r.preshift = 0;
    return r;
}

static MagicU64 magicu64(uint64_t d) {
    MagicU64 r = magicu64_inner(d, 0);
    if (r.a && !(d & 1)) {
        int preshift = __builtin_ctzll(d);
        MagicU64 r2 = magicu64_inner(d >> preshift, (unsigned)preshift);
        r2.preshift = preshift;
        return r2;
    }
    if (r.a) r.s -= 1;
    return r;
}

// Signed magic-multiply constants for dividing a `bits`-wide signed value
// by d (d not 0, not +-1, not a power of two -- handled separately).
// Runtime sequence (n = dividend, M/s from here, d the original divisor):
//   q = high_half_signed(n * M);                // mulhs
//   if (M > 0 && d < 0) q -= n;
//   if (M < 0 && d > 0) q += n;
//   q >>= s;                                      // arithmetic
//   q += (unsigned)q >> (bits-1);                 // round toward zero
static MagicS32 magic32(int32_t d) {
    MagicS32 mag;
    uint32_t ad = (uint32_t)(d < 0 ? -(int64_t)d : d);
    uint32_t t = 0x80000000u + ((uint32_t)d >> 31);
    uint32_t anc = t - 1 - t % ad;
    int p = 31;
    uint32_t q1 = 0x80000000u / anc, r1 = 0x80000000u - q1 * anc;
    uint32_t q2 = 0x80000000u / ad, r2 = 0x80000000u - q2 * ad;
    uint32_t delta;
    do {
        p++;
        q1 = 2 * q1;
        r1 = 2 * r1;
        if (r1 >= anc) {
            q1++;
            r1 -= anc;
        }
        q2 = 2 * q2;
        r2 = 2 * r2;
        if (r2 >= ad) {
            q2++;
            r2 -= ad;
        }
        delta = ad - r2;
    } while (q1 < delta || (q1 == delta && r1 == 0));
    mag.M = (int32_t)(q2 + 1);
    if (d < 0) mag.M = -mag.M;
    mag.s = p - 32;
    return mag;
}

static MagicS64 magic64(int64_t d) {
    MagicS64 mag;
    uint64_t ad = (uint64_t)(d < 0 ? -(__int128)d : d);
    uint64_t t = 0x8000000000000000ull + ((uint64_t)d >> 63);
    uint64_t anc = t - 1 - t % ad;
    int p = 63;
    uint64_t q1 = 0x8000000000000000ull / anc, r1 = 0x8000000000000000ull - q1 * anc;
    uint64_t q2 = 0x8000000000000000ull / ad, r2 = 0x8000000000000000ull - q2 * ad;
    uint64_t delta;
    do {
        p++;
        q1 = 2 * q1;
        r1 = 2 * r1;
        if (r1 >= anc) {
            q1++;
            r1 -= anc;
        }
        q2 = 2 * q2;
        r2 = 2 * r2;
        if (r2 >= ad) {
            q2++;
            r2 -= ad;
        }
        delta = ad - r2;
    } while (q1 < delta || (q1 == delta && r1 == 0));
    mag.M = (int64_t)(q2 + 1);
    if (d < 0) mag.M = -mag.M;
    mag.s = p - 64;
    return mag;
}

// ---------------------------------------------------------------------
// Codegen: widening high-multiply (n * m, keep only the top `sz` bytes)
// ---------------------------------------------------------------------

#ifndef ARCH_ARM64
// x86: one-operand MUL/IMUL, rdx:rax = rax * m. RAX/RDX are never part of
// the VReg pool (see codegen_asm.h's cg_x86_reg[] comment), so clobbering
// them here is always safe -- matches the plain idiv/div path this
// replaces, which uses the same two physical registers directly.
static VReg cgopt_mulhi(VReg n, VReg m, int sz, bool is_unsigned) {
    VReg q = alloc_reg();
    x86_mov_rr(cg_sec, sz, X86_RAX, REG(n));
    if (is_unsigned)
        asm_mul_1op(cg_sec, m, sz);
    else
        x86_imul_r(cg_sec, sz, REG(m));
    x86_mov_rr(cg_sec, sz, REG(q), X86_RDX);
    return q;
}
#else
// ARM64: UMULH/SMULH give the high 64 bits of a 64x64->128 product
// directly. There's no 32-bit "high multiply", so the 32-bit case widens
// with UMULL/SMULL (32x32->64) and then shifts the 64-bit result right by
// 32 to land the high half in the low 32 bits.
static VReg cgopt_mulhi(VReg n, VReg m, int sz, bool is_unsigned) {
    VReg q = alloc_reg();
    if (sz == 8) {
        if (is_unsigned) asm_umulh(cg_sec, q, n, m);
        else
            asm_smulh(cg_sec, q, n, m);
    } else {
        if (is_unsigned) asm_umull(cg_sec, q, n, m);
        else
            asm_smull(cg_sec, q, n, m);
        asm_shr_imm(cg_sec, q, 8, 32);
    }
    return q;
}
#endif

// asm_mov_imm's ARM64 branch expects `imm` pre-masked to exactly `size`
// bytes when size < 8 (every existing caller in this codebase already
// does this manually, e.g. codegen.c's ND_NUM case masks to `v & 0xffff`
// chunks itself) -- it builds the constant via MOVZ + up to 3 MOVKs
// bounded only by a byte-count shift limit, not by `size`, so a
// sign-extended 64-bit value at size==4 (upper 32 bits all 1s) drives it
// one MOVK iteration too far and emits an out-of-range (invalid, 32-bit
// destination, shift #32) encoding. Mask here so this file never relies
// on values happening to already fit.
static void cgopt_mov_imm(VReg r, int sz, int64_t imm) {
    if (sz == 4) imm = (int64_t)(uint32_t)(uint64_t)imm;
    asm_mov_imm(cg_sec, r, sz, imm);
}

// ---------------------------------------------------------------------
// Quotient computation: leaves n_reg untouched (the caller may still need
// the original dividend afterward to finish a modulo via n - q*d).
// ---------------------------------------------------------------------

static VReg cgopt_udiv_const_q(VReg n_reg, uint64_t d, int sz) {
    if ((d & (d - 1)) == 0) { // power of two
        int k = __builtin_ctzll(d);
        VReg q = alloc_reg();
        asm_mov_reg_reg(cg_sec, q, n_reg, sz);
        if (k) asm_shr_imm(cg_sec, q, sz, (uint8_t)k);
        return q;
    }
    MagicU32 mg32 = {0, 0, 0, 0};
    MagicU64 mg64 = {0, 0, 0, 0};
    if (sz == 8) mg64 = magicu64(d);
    else
        mg32 = magicu32((uint32_t)d);
    int preshift = (sz == 8) ? mg64.preshift : mg32.preshift;
    uint64_t M = (sz == 8) ? mg64.M : (uint64_t)mg32.M;
    int add_back = (sz == 8) ? mg64.a : mg32.a;
    int postshift = (sz == 8) ? mg64.s : mg32.s;

    VReg nreg = alloc_reg();
    asm_mov_reg_reg(cg_sec, nreg, n_reg, sz);
    if (preshift) asm_shr_imm(cg_sec, nreg, sz, (uint8_t)preshift);
    VReg mreg = alloc_reg();
    cgopt_mov_imm(mreg, sz, (int64_t)M);
    VReg q = cgopt_mulhi(nreg, mreg, sz, true);
    free_reg(mreg);
    if (add_back) {
        VReg t = alloc_reg();
        asm_mov_reg_reg(cg_sec, t, nreg, sz);
        asm_sub_reg_reg(cg_sec, t, q, sz);
        asm_shr_imm(cg_sec, t, sz, 1);
        asm_add_reg_reg(cg_sec, q, t, sz);
        free_reg(t);
    }
    free_reg(nreg);
    if (postshift) asm_shr_imm(cg_sec, q, sz, (uint8_t)postshift);
    return q;
}

static VReg cgopt_sdiv_const_q(VReg n_reg, int64_t d, int sz) {
    uint64_t ad = (uint64_t)(d < 0 ? -d : d);
    int width = sz * 8;
    if ((ad & (ad - 1)) == 0) { // |d| is a power of two
        int k = __builtin_ctzll(ad);
        VReg q = alloc_reg();
        asm_mov_reg_reg(cg_sec, q, n_reg, sz);
        if (k) {
            VReg t = alloc_reg();
            asm_mov_reg_reg(cg_sec, t, q, sz);
            asm_sar_imm(cg_sec, t, sz, (uint8_t)(width - 1)); // t = n>>(width-1), arithmetic
            asm_shr_imm(cg_sec, t, sz, (uint8_t)(width - k)); // t = (unsigned)t >> (width-k)
            asm_add_reg_reg(cg_sec, q, t, sz); // q = n + t
            asm_sar_imm(cg_sec, q, sz, (uint8_t)k); // q >>= k, arithmetic
            free_reg(t);
        }
        if (d < 0) asm_neg(cg_sec, q, sz);
        return q;
    }
    int64_t M;
    int postshift;
    if (sz == 8) {
        MagicS64 mg = magic64(d);
        M = mg.M;
        postshift = mg.s;
    } else {
        MagicS32 mg = magic32((int32_t)d);
        M = mg.M;
        postshift = mg.s;
    }
    VReg mreg = alloc_reg();
    cgopt_mov_imm(mreg, sz, M);
    VReg q = cgopt_mulhi(n_reg, mreg, sz, false);
    free_reg(mreg);
    if (M > 0 && d < 0) asm_sub_reg_reg(cg_sec, q, n_reg, sz);
    if (M < 0 && d > 0) asm_add_reg_reg(cg_sec, q, n_reg, sz);
    if (postshift) asm_sar_imm(cg_sec, q, sz, (uint8_t)postshift);
    VReg sbit = alloc_reg();
    asm_mov_reg_reg(cg_sec, sbit, q, sz);
    asm_shr_imm(cg_sec, sbit, sz, (uint8_t)(width - 1)); // 0 or 1, the sign of q
    asm_add_reg_reg(cg_sec, q, sbit, sz);
    free_reg(sbit);
    return q;
}

// ---------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------

VReg cgopt_div_mod_const(VReg r_lhs, int64_t d, int sz, bool is_unsigned, bool is_mod) {
    // Trivial divisors: skip the general machinery (magic() requires
    // |d| >= 2, and it's not worth a shift sequence either).
    if (is_unsigned) {
        uint64_t ud = (uint64_t)d;
        if (sz == 4) ud &= 0xFFFFFFFFull;
        if (ud == 1) {
            if (is_mod) asm_xor_reg_reg(cg_sec, r_lhs, r_lhs, sz);
            return r_lhs;
        }
    } else {
        if (d == 1) {
            if (is_mod) asm_xor_reg_reg(cg_sec, r_lhs, r_lhs, sz);
            return r_lhs;
        }
        if (d == -1) {
            // n/-1 and n%-1: INT_MIN/-1 is UB in C, so folding it to
            // -INT_MIN (== INT_MIN, matching plain negation) resp. 0 is a
            // legal "any answer" choice -- the same one gcc/clang make.
            if (is_mod) asm_xor_reg_reg(cg_sec, r_lhs, r_lhs, sz);
            else
                asm_neg(cg_sec, r_lhs, sz);
            return r_lhs;
        }
    }

    VReg q = is_unsigned ? cgopt_udiv_const_q(r_lhs, (uint64_t)d, sz)
                         : cgopt_sdiv_const_q(r_lhs, d, sz);
    if (!is_mod) {
        asm_mov_reg_reg(cg_sec, r_lhs, q, sz);
        free_reg(q);
        return r_lhs;
    }
    // r_lhs (still the original dividend n) -= q*d
    VReg dreg = alloc_reg();
    cgopt_mov_imm(dreg, sz, d);
    asm_mul_reg_reg(cg_sec, q, dreg, sz);
    free_reg(dreg);
    asm_sub_reg_reg(cg_sec, r_lhs, q, sz);
    free_reg(q);
    return r_lhs;
}
