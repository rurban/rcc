// SPDX-License-Identifier: LGPL-2.1-or-later
// AArch64 instruction encoder.  All instructions are 32-bit fixed-width.
// Reference: Arm Architecture Reference Manual for A-profile architecture.
#include "rcc.h"
#ifdef ARCH_ARM64
#include "arm64_enc.h"
#include "obj.h"
#include <assert.h>
#include <stdint.h>

// Bit-field helpers
#define BIT(n)         (1u << (n))
#define BITS(hi, lo, v)  (((uint32_t)(v) & ((1u<<((hi)-(lo)+1))-1u)) << (lo))
#define SF(n)          BITS(31,31,(n))
#define OPC(hi, lo, v)   BITS(hi,lo,v)

// ---------------------------------------------------------------------------
// Logical immediate encoding helper
// Based on ARM reference: an immediate is a replicated bitmask.
// Returns 0 if not encodable, else packs N:immr:imms into bits 22:10.
// ---------------------------------------------------------------------------
static bool try_encode_logic_imm(int sf, uint64_t val, int *N_out, int *immr_out, int *imms_out) {
    // For 32-bit ops, only the lower 32 bits matter per ARM64 W-register semantics
    if (!sf) val &= 0xFFFFFFFFull;
    if (val == 0 || val == ~0ull) return false;
    int max_e = sf ? 64 : 32;
    int len;
    uint64_t mask;
    // Try each rotation length (2,4,8,16,32,64)
    for (len = 1; len <= 6; len++) {
        int e = 1 << len; // element size
        if (e > 64) break;
        mask = (e == 64) ? (uint64_t)-1 : ((uint64_t)1 << e) - 1;
        uint64_t elem = val & mask;
        // Check if val is a repetition of elem across the register width
        bool ok = true;
        int reg_bits = sf ? 64 : 32;
        for (int s = e; s < reg_bits; s += e) {
            if (((val >> s) & mask) != elem) {
                ok = false;
                break;
            }
        }
        if (!ok) continue;
        // elem must be a contiguous run of 1s (possibly rotated)
        if (elem == 0 || (elem & mask) == mask) continue;
        // Find the rotation
        // Count trailing zeros of ~elem to find where 0-run starts
        int ctz_inv = 0;
        uint64_t t = (~elem) & mask;
        while (t & 1) {
            ctz_inv++;
            t >>= 1;
        }
        // Rotation amount = (e - ctz_inv) % e  -- rotate in correct direction
        int immr = (e - ctz_inv) % e;
        // Rotate elem right by ctz_inv so the 1-run starts at bit 0
        uint64_t rotated = ((elem >> ctz_inv) | (elem << (e - ctz_inv))) & mask;
        int cto = 0; // count trailing ones of rotated element
        t = rotated;
        while (t & 1) {
            cto++;
            t >>= 1;
        }
        int imms = cto - 1;
        int N = (e == 64) ? 1 : 0;
        if (!sf && e == 64) return false;
        *N_out = N;
        *immr_out = immr;
        *imms_out = imms | ((e == 64) ? 0 : ((~(e - 1) << 1) & 0x3f));
        return true;
    }
    return false;
}

// Encode a logic immediate; returns 0 if not representable.
uint64_t arm64_encode_logic_imm(int sf, uint64_t val, int *N, int *immr, int *imms) {
    if (!try_encode_logic_imm(sf, val, N, immr, imms)) return 0;
    return 1;
}

static uint32_t logic_imm_field(int sf, uint64_t val) {
    int N = 0, immr = 0, imms = 0;
    (void)try_encode_logic_imm(sf, val, &N, &immr, &imms);
    return BITS(22, 22, N) | BITS(21, 16, immr) | BITS(15, 10, imms);
}

// ---------------------------------------------------------------------------
// Data processing — immediate
// ---------------------------------------------------------------------------

void arm64_movz(SecBuf *s, int sf, Arm64Reg rd, uint16_t imm16, uint16_t shift16) {
    assert(shift16 == 0 || shift16 == 16 || shift16 == 32 || shift16 == 48);
    secbuf_emit32le(s, SF(sf) | 0xd2800000u | BITS(22, 21, shift16 / 16) | BITS(20, 5, imm16) | BITS(4, 0, rd));
}

void arm64_movk(SecBuf *s, int sf, Arm64Reg rd, uint16_t imm16, uint16_t shift16) {
    secbuf_emit32le(s, SF(sf) | 0xf2800000u | BITS(22, 21, shift16 / 16) | BITS(20, 5, imm16) | BITS(4, 0, rd));
}

void arm64_movn(SecBuf *s, int sf, Arm64Reg rd, uint16_t imm16, uint16_t shift16) {
    secbuf_emit32le(s, SF(sf) | 0x92800000u | BITS(22, 21, shift16 / 16) | BITS(20, 5, imm16) | BITS(4, 0, rd));
}

void arm64_add_imm(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, int32_t imm12, uint16_t sh) {
    assert(imm12 >= 0 && imm12 < 4096);
    secbuf_emit32le(s, SF(sf) | 0x11000000u | BITS(23, 22, sh) | BITS(21, 10, imm12) | BITS(9, 5, rn) | BITS(4, 0, rd));
}

void arm64_adds_imm(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, int32_t imm12, uint32_t sh) {
    secbuf_emit32le(s, SF(sf) | 0x31000000u | BITS(23, 22, sh) | BITS(21, 10, imm12) | BITS(9, 5, rn) | BITS(4, 0, rd));
}

void arm64_sub_imm(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, int32_t imm12, uint32_t sh) {
    assert(imm12 >= 0 && imm12 < 4096);
    secbuf_emit32le(s, SF(sf) | 0x51000000u | BITS(23, 22, sh) | BITS(21, 10, imm12) | BITS(9, 5, rn) | BITS(4, 0, rd));
}

void arm64_subs_imm(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, int32_t imm12, uint32_t sh) {
    secbuf_emit32le(s, SF(sf) | 0x71000000u | BITS(23, 22, sh) | BITS(21, 10, imm12) | BITS(9, 5, rn) | BITS(4, 0, rd));
}

void arm64_and_imm(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, uint64_t val) {
    secbuf_emit32le(s, SF(sf) | 0x12000000u | logic_imm_field(sf, val) | BITS(9, 5, rn) | BITS(4, 0, rd));
}
void arm64_orr_imm(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, uint64_t val) {
    secbuf_emit32le(s, SF(sf) | 0x32000000u | logic_imm_field(sf, val) | BITS(9, 5, rn) | BITS(4, 0, rd));
}
void arm64_eor_imm(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, uint64_t val) {
    secbuf_emit32le(s, SF(sf) | 0x52000000u | logic_imm_field(sf, val) | BITS(9, 5, rn) | BITS(4, 0, rd));
}
void arm64_ands_imm(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, uint64_t val) {
    secbuf_emit32le(s, SF(sf) | 0x72000000u | logic_imm_field(sf, val) | BITS(9, 5, rn) | BITS(4, 0, rd));
}

// ---------------------------------------------------------------------------
// Data processing — register (shifted register)
// ---------------------------------------------------------------------------
static uint32_t dp_reg(int sf, uint32_t opc, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm,
                       Arm64Shift sh, int imm6) {
    return SF(sf) | opc | BITS(23, 22, sh) | BITS(20, 16, (unsigned)rm) | BITS(15, 10, imm6) | BITS(9, 5, (unsigned)rn) | BITS(4, 0, (unsigned)rd);
}

void arm64_add_reg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Shift sh, int i6) {
    secbuf_emit32le(s, dp_reg(sf, 0x0b000000u, rd, rn, rm, sh, i6));
}
void arm64_adds_reg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Shift sh, int i6) {
    secbuf_emit32le(s, dp_reg(sf, 0x2b000000u, rd, rn, rm, sh, i6));
}
void arm64_sub_reg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Shift sh, int i6) {
    secbuf_emit32le(s, dp_reg(sf, 0x4b000000u, rd, rn, rm, sh, i6));
}
void arm64_subs_reg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Shift sh, int i6) {
    secbuf_emit32le(s, dp_reg(sf, 0x6b000000u, rd, rn, rm, sh, i6));
}
void arm64_and_reg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Shift sh, int i6) {
    secbuf_emit32le(s, dp_reg(sf, 0x0a000000u, rd, rn, rm, sh, i6));
}
void arm64_orr_reg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Shift sh, int i6) {
    secbuf_emit32le(s, dp_reg(sf, 0x2a000000u, rd, rn, rm, sh, i6));
}
void arm64_eor_reg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Shift sh, int i6) {
    secbuf_emit32le(s, dp_reg(sf, 0x4a000000u, rd, rn, rm, sh, i6));
}
void arm64_ands_reg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Shift sh, int i6) {
    secbuf_emit32le(s, dp_reg(sf, 0x6a000000u, rd, rn, rm, sh, i6));
}
void arm64_bic_reg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Shift sh, int i6) {
    secbuf_emit32le(s, dp_reg(sf, 0x0a200000u, rd, rn, rm, sh, i6));
}

// Extended register encoding (supports SP register)
// ADD/SUB (extended register): sf | op<<30 | 0<<29 | 0x0B<<24 | 1<<21 | Rm | option<<13 | 0 | 0 | Rn | Rd
// option encoding: UXTB=0, UXTH=1, UXTW=2, UXTX=3
static uint32_t extreg_enc(int sf, int is_sub, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm,
                           Arm64Ext option, int imm3) {
    (void)imm3;
    uint32_t op_bit = is_sub ? (1u << 30) : 0;
    return SF(sf) | op_bit | (0x0Bu << 24) | (1u << 21) |
        BITS(20, 16, rm) | BITS(15, 13, (uint32_t)option) |
        BITS(9, 5, rn) | BITS(4, 0, rd);
}

void arm64_add_extreg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Ext option, int imm3) {
    secbuf_emit32le(s, extreg_enc(sf, 0, rd, rn, rm, option, imm3));
}

void arm64_sub_extreg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Ext option, int imm3) {
    secbuf_emit32le(s, extreg_enc(sf, 1, rd, rn, rm, option, imm3));
}

// 3-register
static uint32_t dp3(int sf, uint32_t op, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Reg ra) {
    return SF(sf) | op | BITS(20, 16, rm) | BITS(14, 10, ra) | BITS(9, 5, rn) | BITS(4, 0, rd);
}

void arm64_mul(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm) {
    secbuf_emit32le(s, dp3(sf, 0x1b000000u, rd, rn, rm, ARM64_XZR));
}
void arm64_smull(SecBuf *s, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm) {
    secbuf_emit32le(s, 0x9b200000u | BITS(20, 16, rm) | BITS(9, 5, rn) | BITS(4, 0, rd));
}
void arm64_umull(SecBuf *s, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm) {
    secbuf_emit32le(s, 0x9ba00000u | BITS(20, 16, rm) | BITS(9, 5, rn) | BITS(4, 0, rd));
}
void arm64_smulh(SecBuf *s, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm) {
    secbuf_emit32le(s, 0x9b407c00u | BITS(20, 16, rm) | BITS(9, 5, rn) | BITS(4, 0, rd));
}
void arm64_umulh(SecBuf *s, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm) {
    secbuf_emit32le(s, 0x9bc07c00u | BITS(20, 16, rm) | BITS(9, 5, rn) | BITS(4, 0, rd));
}
void arm64_sdiv(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm) {
    secbuf_emit32le(s, SF(sf) | 0x1ac00c00u | BITS(20, 16, rm) | BITS(9, 5, rn) | BITS(4, 0, rd));
}
void arm64_udiv(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm) {
    secbuf_emit32le(s, SF(sf) | 0x1ac00800u | BITS(20, 16, rm) | BITS(9, 5, rn) | BITS(4, 0, rd));
}

// Shifts
static uint32_t dp2(int sf, uint32_t op, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm) {
    return SF(sf) | op | BITS(20, 16, rm) | BITS(9, 5, rn) | BITS(4, 0, rd);
}
void arm64_lsl_reg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm) { secbuf_emit32le(s, dp2(sf, 0x1ac02000u, rd, rn, rm)); }
void arm64_lsr_reg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm) { secbuf_emit32le(s, dp2(sf, 0x1ac02400u, rd, rn, rm)); }
void arm64_asr_reg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm) { secbuf_emit32le(s, dp2(sf, 0x1ac02800u, rd, rn, rm)); }
void arm64_ror_reg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm) { secbuf_emit32le(s, dp2(sf, 0x1ac02c00u, rd, rn, rm)); }
// EXTR: extract field. ROR immediate is EXTR rd, rn, rn, #shift.
// Encoding: 1 00 11111 0 0 sf Rm imm6 Rn Rd
void arm64_extr(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, int lsb) {
    uint32_t opc = sf ? 0x93c00000u : 0x13800000u;
    uint32_t imm = (uint32_t)(lsb & (sf ? 63 : 31));
    secbuf_emit32le(s, opc | BITS(20, 16, rm) | BITS(15, 10, imm) | BITS(9, 5, rn) | BITS(4, 0, rd));
}

// Immediate shifts use UBFM/SBFM with appropriate fields
// LSL #shift: UBFM rd, rn, #(-shift mod bitwidth), #(bitwidth-1-shift)
void arm64_lsl_imm(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, int shift) {
    int w = sf ? 64 : 32;
    int immr = (-shift) & (w - 1);
    int imms = w - 1 - shift;
    uint32_t opc = sf ? 0xd3400000u : 0x53000000u;
    secbuf_emit32le(s, opc | BITS(21, 16, immr) | BITS(15, 10, imms) | BITS(9, 5, rn) | BITS(4, 0, rd));
}
void arm64_lsr_imm(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, int shift) {
    int w = sf ? 64 : 32;
    int imms = w - 1;
    uint32_t opc = sf ? 0xd3400000u : 0x53000000u;
    secbuf_emit32le(s, opc | BITS(21, 16, shift) | BITS(15, 10, imms) | BITS(9, 5, rn) | BITS(4, 0, rd));
}
void arm64_asr_imm(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, int shift) {
    int w = sf ? 64 : 32;
    int imms = w - 1;
    uint32_t opc = sf ? 0x93400000u : 0x13000000u;
    secbuf_emit32le(s, opc | BITS(21, 16, shift) | BITS(15, 10, imms) | BITS(9, 5, rn) | BITS(4, 0, rd));
}

// Unary operations
void arm64_neg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rm) {
    arm64_sub_reg(s, sf, rd, ARM64_XZR, rm, ARM64_LSL, 0);
}
void arm64_mvn(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rm, Arm64Shift sh, int imm6) {
    secbuf_emit32le(s, dp_reg(sf, 0x2a200000u, rd, ARM64_XZR, rm, sh, imm6));
}

// Bit counting
static uint32_t dp1(int sf, uint32_t op, Arm64Reg rd, Arm64Reg rn) {
    return SF(sf) | op | BITS(9, 5, rn) | BITS(4, 0, rd);
}
void arm64_clz(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn) { secbuf_emit32le(s, dp1(sf, 0x5ac01000u, rd, rn)); }
void arm64_cls(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn) { secbuf_emit32le(s, dp1(sf, 0x5ac01400u, rd, rn)); }
void arm64_rbit(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn) { secbuf_emit32le(s, dp1(sf, 0x5ac00000u, rd, rn)); }
void arm64_rev(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn) {
    uint32_t opc = sf ? 0x5ac00c00u : 0x5ac00800u;
    secbuf_emit32le(s, dp1(sf, opc, rd, rn));
}
void arm64_rev16(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn) { secbuf_emit32le(s, dp1(sf, 0x5ac00400u, rd, rn)); }
void arm64_rev32(SecBuf *s, Arm64Reg rd, Arm64Reg rn) { secbuf_emit32le(s, 0xdac00800u | BITS(9, 5, rn) | BITS(4, 0, rd)); }

// Sign/zero extend: use SBFM/UBFM
void arm64_sxtb(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn) { secbuf_emit32le(s, SF(sf) | (sf ? 0x93401c00u : 0x13001c00u) | BITS(9, 5, rn) | BITS(4, 0, rd)); }
void arm64_sxth(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn) { secbuf_emit32le(s, SF(sf) | (sf ? 0x93403c00u : 0x13003c00u) | BITS(9, 5, rn) | BITS(4, 0, rd)); }
void arm64_sxtw(SecBuf *s, Arm64Reg rd, Arm64Reg rn) { secbuf_emit32le(s, 0x93407c00u | BITS(9, 5, rn) | BITS(4, 0, rd)); }
void arm64_uxtb(SecBuf *s, Arm64Reg rd, Arm64Reg rn) { secbuf_emit32le(s, 0x53001c00u | BITS(9, 5, rn) | BITS(4, 0, rd)); }
void arm64_uxth(SecBuf *s, Arm64Reg rd, Arm64Reg rn) { secbuf_emit32le(s, 0x53003c00u | BITS(9, 5, rn) | BITS(4, 0, rd)); }

// Bitfield extract
void arm64_ubfx(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, int lsb, int width) {
    int imms = lsb + width - 1;
    secbuf_emit32le(s, SF(sf) | (sf ? 0xd3400000u : 0x53000000u) | BITS(21, 16, lsb) | BITS(15, 10, imms) | BITS(9, 5, rn) | BITS(4, 0, rd));
}
void arm64_sbfx(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, int lsb, int width) {
    int imms = lsb + width - 1;
    secbuf_emit32le(s, SF(sf) | (sf ? 0x93400000u : 0x13000000u) | BITS(21, 16, lsb) | BITS(15, 10, imms) | BITS(9, 5, rn) | BITS(4, 0, rd));
}

// Conditional select
void arm64_csel(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Cond c) {
    secbuf_emit32le(s, SF(sf) | 0x1a800000u | BITS(20, 16, rm) | BITS(15, 12, c) | BITS(9, 5, rn) | BITS(4, 0, rd));
}
void arm64_csinc(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Cond c) {
    secbuf_emit32le(s, SF(sf) | 0x1a800400u | BITS(20, 16, rm) | BITS(15, 12, c) | BITS(9, 5, rn) | BITS(4, 0, rd));
}
void arm64_csneg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Cond c) {
    secbuf_emit32le(s, SF(sf) | 0x5a800400u | BITS(20, 16, rm) | BITS(15, 12, c) | BITS(9, 5, rn) | BITS(4, 0, rd));
}
void arm64_cset(SecBuf *s, int sf, Arm64Reg rd, Arm64Cond c) {
    // CSET = CSINC rd, xzr, xzr, invert(c)
    arm64_csinc(s, sf, rd, ARM64_XZR, ARM64_XZR, c ^ 1);
}
void arm64_cneg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Cond c) {
    arm64_csneg(s, sf, rd, rn, rn, c ^ 1);
}
void arm64_adc(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm) {
    secbuf_emit32le(s, SF(sf) | 0x1a000000u | BITS(20, 16, rm) | BITS(9, 5, rn) | BITS(4, 0, rd));
}
void arm64_sbc(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm) {
    secbuf_emit32le(s, SF(sf) | 0x5a000000u | BITS(20, 16, rm) | BITS(9, 5, rn) | BITS(4, 0, rd));
}

// ---------------------------------------------------------------------------
// PC-relative addressing
// ---------------------------------------------------------------------------
void arm64_adrp(SecBuf *s, Arm64Reg rd, int32_t page_imm) {
    uint32_t immlo = (uint32_t)(page_imm & 3);
    uint32_t immhi = (uint32_t)((page_imm >> 2) & 0x7ffff);
    secbuf_emit32le(s, 0x90000000u | BITS(30, 29, immlo) | BITS(23, 5, immhi) | BITS(4, 0, rd));
}
void arm64_adr(SecBuf *s, Arm64Reg rd, int32_t imm) {
    uint32_t immlo = (uint32_t)(imm & 3);
    uint32_t immhi = (uint32_t)((imm >> 2) & 0x7ffff);
    secbuf_emit32le(s, 0x10000000u | BITS(30, 29, immlo) | BITS(23, 5, immhi) | BITS(4, 0, rd));
}

// ---------------------------------------------------------------------------
// Load/Store
// ---------------------------------------------------------------------------
// Unsigned offset load: size=0→byte,1→half,2→word,3→dword + sf for x-regs
void arm64_ldr_uoff(SecBuf *s, int sz, Arm64Reg rt, Arm64Reg rn, uint32_t uimm) {
    // sz: 0=8bit,1=16bit,2=32bit,3=64bit
    uint32_t opc = (sz == 3) ? 0xf9400000u : (sz == 2) ? 0xb9400000u
        : (sz == 1)                                    ? 0x79400000u
                                                       : 0x39400000u;
    secbuf_emit32le(s, opc | BITS(21, 10, uimm) | BITS(9, 5, rn) | BITS(4, 0, rt));
}
void arm64_ldrb_uoff(SecBuf *s, Arm64Reg rt, Arm64Reg rn, uint32_t uimm) { arm64_ldr_uoff(s, 0, rt, rn, uimm); }
void arm64_ldrh_uoff(SecBuf *s, Arm64Reg rt, Arm64Reg rn, uint32_t uimm) { arm64_ldr_uoff(s, 1, rt, rn, uimm); }

void arm64_str_uoff(SecBuf *s, int sz, Arm64Reg rt, Arm64Reg rn, uint32_t uimm) {
    uint32_t opc = (sz == 3) ? 0xf9000000u : (sz == 2) ? 0xb9000000u
        : (sz == 1)                                    ? 0x79000000u
                                                       : 0x39000000u;
    secbuf_emit32le(s, opc | BITS(21, 10, uimm) | BITS(9, 5, rn) | BITS(4, 0, rt));
}
void arm64_strb_uoff(SecBuf *s, Arm64Reg rt, Arm64Reg rn, uint32_t uimm) { arm64_str_uoff(s, 0, rt, rn, uimm); }
void arm64_strh_uoff(SecBuf *s, Arm64Reg rt, Arm64Reg rn, uint32_t uimm) { arm64_str_uoff(s, 1, rt, rn, uimm); }

// Pre/post-index (9-bit signed immediate)
static uint32_t ldr_imm9(uint32_t opc, Arm64Reg rt, Arm64Reg rn, int32_t imm9, bool pre) {
    uint32_t idx = pre ? 0xc00u : 0x400u;
    return opc | idx | BITS(20, 12, (uint32_t)imm9 & 0x1ff) | BITS(9, 5, rn) | BITS(4, 0, rt);
}
void arm64_ldr_imm(SecBuf *s, int sf, Arm64Reg rt, Arm64Reg rn, int32_t imm9, bool pre) {
    secbuf_emit32le(s, ldr_imm9(sf ? 0xf8400000u : 0xb8400000u, rt, rn, imm9, pre));
}
void arm64_ldrb_imm(SecBuf *s, Arm64Reg rt, Arm64Reg rn, int32_t imm9) {
    secbuf_emit32le(s, ldr_imm9(0x38400000u, rt, rn, imm9, false));
}
void arm64_ldrh_imm(SecBuf *s, Arm64Reg rt, Arm64Reg rn, int32_t imm9) {
    secbuf_emit32le(s, ldr_imm9(0x78400000u, rt, rn, imm9, false));
}
void arm64_ldrsb(SecBuf *s, int sf, Arm64Reg rt, Arm64Reg rn, int32_t imm9) {
    uint32_t opc = sf ? 0x38800000u : 0x38c00000u;
    secbuf_emit32le(s, ldr_imm9(opc, rt, rn, imm9, false));
}
void arm64_ldrsh(SecBuf *s, int sf, Arm64Reg rt, Arm64Reg rn, int32_t imm9) {
    uint32_t opc = sf ? 0x78800000u : 0x78c00000u;
    secbuf_emit32le(s, ldr_imm9(opc, rt, rn, imm9, false));
}
void arm64_ldrsw_imm(SecBuf *s, Arm64Reg rt, Arm64Reg rn, int32_t imm9) {
    secbuf_emit32le(s, ldr_imm9(0xb8800000u, rt, rn, imm9, false));
}
void arm64_ldrsw_uoff(SecBuf *s, Arm64Reg rt, Arm64Reg rn, uint32_t uimm) {
    secbuf_emit32le(s, 0xb9800000u | BITS(21, 10, uimm) | BITS(9, 5, rn) | BITS(4, 0, rt));
}
void arm64_str_imm(SecBuf *s, int sf, Arm64Reg rt, Arm64Reg rn, int32_t imm9, bool pre) {
    secbuf_emit32le(s, ldr_imm9(sf ? 0xf8000000u : 0xb8000000u, rt, rn, imm9, pre));
}
void arm64_strb_imm(SecBuf *s, Arm64Reg rt, Arm64Reg rn, int32_t imm9) {
    secbuf_emit32le(s, ldr_imm9(0x38000000u, rt, rn, imm9, false));
}
void arm64_strh_imm(SecBuf *s, Arm64Reg rt, Arm64Reg rn, int32_t imm9) {
    secbuf_emit32le(s, ldr_imm9(0x78000000u, rt, rn, imm9, false));
}

// Unscaled (LDUR/STUR)
void arm64_ldur(SecBuf *s, int sf, Arm64Reg rt, Arm64Reg rn, int32_t imm9) {
    secbuf_emit32le(s, (sf ? 0xf8400000u : 0xb8400000u) | BITS(20, 12, (uint32_t)imm9 & 0x1ff) | BITS(9, 5, rn) | BITS(4, 0, rt));
}
void arm64_ldurb(SecBuf *s, Arm64Reg rt, Arm64Reg rn, int32_t imm9) {
    secbuf_emit32le(s, 0x38400000u | BITS(20, 12, (uint32_t)imm9 & 0x1ff) | BITS(9, 5, rn) | BITS(4, 0, rt));
}
void arm64_ldurh(SecBuf *s, Arm64Reg rt, Arm64Reg rn, int32_t imm9) {
    secbuf_emit32le(s, 0x78400000u | BITS(20, 12, (uint32_t)imm9 & 0x1ff) | BITS(9, 5, rn) | BITS(4, 0, rt));
}
void arm64_stur(SecBuf *s, int sf, Arm64Reg rt, Arm64Reg rn, int32_t imm9) {
    secbuf_emit32le(s, (sf ? 0xf8000000u : 0xb8000000u) | BITS(20, 12, (uint32_t)imm9 & 0x1ff) | BITS(9, 5, rn) | BITS(4, 0, rt));
}
void arm64_sturb(SecBuf *s, Arm64Reg rt, Arm64Reg rn, int32_t imm9) {
    secbuf_emit32le(s, 0x38000000u | BITS(20, 12, (uint32_t)imm9 & 0x1ff) | BITS(9, 5, rn) | BITS(4, 0, rt));
}
void arm64_sturh(SecBuf *s, Arm64Reg rt, Arm64Reg rn, int32_t imm9) {
    secbuf_emit32le(s, 0x78000000u | BITS(20, 12, (uint32_t)imm9 & 0x1ff) | BITS(9, 5, rn) | BITS(4, 0, rt));
}

// Register offset
void arm64_ldr_reg(SecBuf *s, int sz, Arm64Reg rt, Arm64Reg rn, Arm64Reg rm, bool ext, int scale) {
    uint32_t opc = (sz == 3) ? 0xf8600800u : (sz == 2) ? 0xb8600800u
        : (sz == 1)                                    ? 0x78600800u
                                                       : 0x38600800u;
    secbuf_emit32le(s, opc | BITS(20, 16, rm) | BITS(15, 13, ext ? 6 : 3) | BITS(12, 12, scale) | BITS(9, 5, rn) | BITS(4, 0, rt));
}
void arm64_str_reg(SecBuf *s, int sz, Arm64Reg rt, Arm64Reg rn, Arm64Reg rm, bool ext, int scale) {
    uint32_t opc = (sz == 3) ? 0xf8200800u : (sz == 2) ? 0xb8200800u
        : (sz == 1)                                    ? 0x78200800u
                                                       : 0x38200800u;
    secbuf_emit32le(s, opc | BITS(20, 16, rm) | BITS(15, 13, ext ? 6 : 3) | BITS(12, 12, scale) | BITS(9, 5, rn) | BITS(4, 0, rt));
}

// Load/store pair
static uint32_t ldp_stp(uint32_t base, Arm64Reg rt1, Arm64Reg rt2, Arm64Reg rn, int32_t imm7,
                        bool pre, bool post) {
    uint32_t mode = pre ? 0x01800000u : post ? 0x00800000u
                                             : 0x01000000u;
    return base | mode | BITS(21, 15, (uint32_t)imm7 & 0x7f) | BITS(14, 10, rt2) | BITS(9, 5, rn) | BITS(4, 0, rt1);
}
void arm64_ldp(SecBuf *s, int sf, Arm64Reg rt1, Arm64Reg rt2, Arm64Reg rn, int32_t imm7, bool pre, bool post) {
    secbuf_emit32le(s, ldp_stp(sf ? 0xa8400000u : 0x28400000u, rt1, rt2, rn, imm7, pre, post));
}
void arm64_stp(SecBuf *s, int sf, Arm64Reg rt1, Arm64Reg rt2, Arm64Reg rn, int32_t imm7, bool pre, bool post) {
    secbuf_emit32le(s, ldp_stp(sf ? 0xa8000000u : 0x28000000u, rt1, rt2, rn, imm7, pre, post));
}

// Exclusive
void arm64_ldxr(SecBuf *s, int sf, Arm64Reg rt, Arm64Reg rn) {
    secbuf_emit32le(s, (sf ? 0xc85f7c00u : 0x885f7c00u) | BITS(9, 5, rn) | BITS(4, 0, rt));
}
void arm64_ldxrb(SecBuf *s, Arm64Reg rt, Arm64Reg rn) { secbuf_emit32le(s, 0x085f7c00u | BITS(9, 5, rn) | BITS(4, 0, rt)); }
void arm64_ldxrh(SecBuf *s, Arm64Reg rt, Arm64Reg rn) { secbuf_emit32le(s, 0x485f7c00u | BITS(9, 5, rn) | BITS(4, 0, rt)); }
void arm64_stxr(SecBuf *s, int sf, Arm64Reg rs, Arm64Reg rt, Arm64Reg rn) {
    secbuf_emit32le(s, (sf ? 0xc8007c00u : 0x88007c00u) | BITS(20, 16, rs) | BITS(9, 5, rn) | BITS(4, 0, rt));
}
void arm64_stxrb(SecBuf *s, Arm64Reg rs, Arm64Reg rt, Arm64Reg rn) {
    secbuf_emit32le(s, 0x08007c00u | BITS(20, 16, rs) | BITS(9, 5, rn) | BITS(4, 0, rt));
}
void arm64_stxrh(SecBuf *s, Arm64Reg rs, Arm64Reg rt, Arm64Reg rn) {
    secbuf_emit32le(s, 0x48007c00u | BITS(20, 16, rs) | BITS(9, 5, rn) | BITS(4, 0, rt));
}
void arm64_ldar(SecBuf *s, int sf, Arm64Reg rt, Arm64Reg rn) {
    secbuf_emit32le(s, (sf ? 0xc8dffc00u : 0x88dffc00u) | BITS(9, 5, rn) | BITS(4, 0, rt));
}
void arm64_ldarb(SecBuf *s, Arm64Reg rt, Arm64Reg rn) { secbuf_emit32le(s, 0x08dffc00u | BITS(9, 5, rn) | BITS(4, 0, rt)); }
void arm64_ldarh(SecBuf *s, Arm64Reg rt, Arm64Reg rn) { secbuf_emit32le(s, 0x48dffc00u | BITS(9, 5, rn) | BITS(4, 0, rt)); }
void arm64_stlr(SecBuf *s, int sf, Arm64Reg rt, Arm64Reg rn) {
    secbuf_emit32le(s, (sf ? 0xc89ffc00u : 0x889ffc00u) | BITS(9, 5, rn) | BITS(4, 0, rt));
}
void arm64_stlrb(SecBuf *s, Arm64Reg rt, Arm64Reg rn) { secbuf_emit32le(s, 0x089ffc00u | BITS(9, 5, rn) | BITS(4, 0, rt)); }
void arm64_stlrh(SecBuf *s, Arm64Reg rt, Arm64Reg rn) { secbuf_emit32le(s, 0x489ffc00u | BITS(9, 5, rn) | BITS(4, 0, rt)); }

// ---------------------------------------------------------------------------
// Branches
// ---------------------------------------------------------------------------
void arm64_b(SecBuf *s, int32_t imm26) {
    secbuf_emit32le(s, 0x14000000u | ((uint32_t)imm26 & 0x3ffffffu));
}
void arm64_bl(SecBuf *s, int32_t imm26) {
    secbuf_emit32le(s, 0x94000000u | ((uint32_t)imm26 & 0x3ffffffu));
}
void arm64_br(SecBuf *s, Arm64Reg rn) { secbuf_emit32le(s, 0xd61f0000u | BITS(9, 5, rn)); }
void arm64_blr(SecBuf *s, Arm64Reg rn) { secbuf_emit32le(s, 0xd63f0000u | BITS(9, 5, rn)); }
void arm64_ret(SecBuf *s, Arm64Reg rn) { secbuf_emit32le(s, 0xd65f0000u | BITS(9, 5, rn)); }
void arm64_bcond(SecBuf *s, Arm64Cond cond, int32_t imm19) {
    secbuf_emit32le(s, 0x54000000u | BITS(23, 5, (uint32_t)imm19 & 0x7ffff) | BITS(3, 0, cond));
}
void arm64_cbz(SecBuf *s, int sf, Arm64Reg rt, int32_t imm19) {
    secbuf_emit32le(s, SF(sf) | 0x34000000u | BITS(23, 5, (uint32_t)imm19 & 0x7ffff) | BITS(4, 0, rt));
}
void arm64_cbnz(SecBuf *s, int sf, Arm64Reg rt, int32_t imm19) {
    secbuf_emit32le(s, SF(sf) | 0x35000000u | BITS(23, 5, (uint32_t)imm19 & 0x7ffff) | BITS(4, 0, rt));
}
void arm64_tbz(SecBuf *s, Arm64Reg rt, int imm6, int32_t imm14) {
    uint32_t b5 = (uint32_t)(imm6 >> 5) & 1;
    uint32_t b40 = (uint32_t)imm6 & 0x1f;
    secbuf_emit32le(s, 0x36000000u | (b5 << 31) | BITS(23, 19, b40) | BITS(18, 5, (uint32_t)imm14 & 0x3fff) | BITS(4, 0, rt));
}
void arm64_tbnz(SecBuf *s, Arm64Reg rt, int imm6, int32_t imm14) {
    uint32_t b5 = (uint32_t)(imm6 >> 5) & 1;
    uint32_t b40 = (uint32_t)imm6 & 0x1f;
    secbuf_emit32le(s, 0x37000000u | (b5 << 31) | BITS(23, 19, b40) | BITS(18, 5, (uint32_t)imm14 & 0x3fff) | BITS(4, 0, rt));
}

// ---------------------------------------------------------------------------
// System / Misc
// ---------------------------------------------------------------------------
void arm64_nop(SecBuf *s) { secbuf_emit32le(s, 0xd503201fu); }
void arm64_dmb(SecBuf *s, int opt) { secbuf_emit32le(s, 0xd50330bfu | BITS(11, 8, opt)); }
void arm64_dsb(SecBuf *s, int opt) { secbuf_emit32le(s, 0xd503309fu | BITS(11, 8, opt)); }
void arm64_isb(SecBuf *s) { secbuf_emit32le(s, 0xd5033fdfu); }
// MRS: move system register to general register.
// sys_reg is the packed op0:op1:CRn:CRm:op2 field.
// Encoding: 1101 0101 0011 1 oooo oooo oooo oooo + Rt (bits 4:0)
// Note: full sys_reg is 15 bits (op0:2, op1:3, CRn:4, CRm:4, op2:3) packed as:
//   op0<<14 | op1<<11 | CRn<<7 | CRm<<3 | op2
void arm64_mrs(SecBuf *s, Arm64Reg rt, uint32_t sys_reg) {
    // MRS: D53B_<sys_reg>_<Rt>
    // op0 goes into bits 20:19, rest into bits 18:5 then shifted
    uint32_t op0 = (sys_reg >> 14) & 3;
    uint32_t rest = sys_reg & 0x3FFF;
    secbuf_emit32le(s, 0xd5300000u | (op0 << 19) | (rest << 5) | (rt & 31));
}
// MSR: move general register to system register.
void arm64_msr(SecBuf *s, uint32_t sys_reg, Arm64Reg rt) {
    uint32_t op0 = (sys_reg >> 14) & 3;
    uint32_t rest = sys_reg & 0x3FFF;
    secbuf_emit32le(s, 0xd5100000u | (op0 << 19) | (rest << 5) | (rt & 31));
}
void arm64_prfm_imm(SecBuf *s, int prfop, Arm64Reg rn, uint32_t uimm) {
    secbuf_emit32le(s, 0xf9800000u | BITS(21, 10, uimm) | BITS(9, 5, rn) | BITS(4, 0, prfop));
}

// ---------------------------------------------------------------------------
// FP / SIMD
// ---------------------------------------------------------------------------
// ftype: 0=single, 1=double (most common for rcc)
void arm64_fmov_f2i(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn) {
    uint32_t ftype = sf ? 1u : 0u;
    secbuf_emit32le(s, SF(sf) | 0x1e260000u | BITS(23, 22, ftype) | BITS(9, 5, rn) | BITS(4, 0, rd));
}
void arm64_fmov_i2f(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn) {
    uint32_t ftype = sf ? 1u : 0u;
    secbuf_emit32le(s, SF(sf) | 0x1e270000u | BITS(23, 22, ftype) | BITS(9, 5, rn) | BITS(4, 0, rd));
}
void arm64_fmov_imm(SecBuf *s, int ftype, Arm64Reg rd, uint8_t imm8) {
    secbuf_emit32le(s, 0x1e201000u | BITS(23, 22, ftype) | BITS(20, 13, imm8) | BITS(4, 0, rd));
}
static uint32_t fp3(int ftype, uint32_t op, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm) {
    return 0x1e200000u | BITS(23, 22, ftype) | op | BITS(20, 16, rm) | BITS(9, 5, rn) | BITS(4, 0, rd);
}
void arm64_fadd(SecBuf *s, int ft, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm) { secbuf_emit32le(s, fp3(ft, 0x002800u, rd, rn, rm)); }
void arm64_fsub(SecBuf *s, int ft, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm) { secbuf_emit32le(s, fp3(ft, 0x003800u, rd, rn, rm)); }
void arm64_fmul(SecBuf *s, int ft, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm) { secbuf_emit32le(s, fp3(ft, 0x000800u, rd, rn, rm)); }
void arm64_fdiv(SecBuf *s, int ft, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm) { secbuf_emit32le(s, fp3(ft, 0x001800u, rd, rn, rm)); }
void arm64_fneg(SecBuf *s, int ft, Arm64Reg rd, Arm64Reg rn) {
    secbuf_emit32le(s, 0x1e214000u | BITS(23, 22, ft) | BITS(9, 5, rn) | BITS(4, 0, rd));
}
void arm64_fabs(SecBuf *s, int ft, Arm64Reg rd, Arm64Reg rn) {
    secbuf_emit32le(s, 0x1e20c000u | BITS(23, 22, ft) | BITS(9, 5, rn) | BITS(4, 0, rd));
}
void arm64_fcmp(SecBuf *s, int ft, Arm64Reg rn, Arm64Reg rm) {
    secbuf_emit32le(s, 0x1e202000u | BITS(23, 22, ft) | BITS(20, 16, rm) | BITS(9, 5, rn));
}
void arm64_fcvt(SecBuf *s, int opc, int ftype, Arm64Reg rd, Arm64Reg rn) {
    secbuf_emit32le(s, 0x1e224000u | BITS(23, 22, ftype) | BITS(16, 15, opc) | BITS(9, 5, rn) | BITS(4, 0, rd));
}
void arm64_scvtf(SecBuf *s, int sf, int ftype, Arm64Reg rd, Arm64Reg rn) {
    secbuf_emit32le(s, SF(sf) | 0x1e220000u | BITS(23, 22, ftype) | BITS(9, 5, rn) | BITS(4, 0, rd));
}
void arm64_ucvtf(SecBuf *s, int sf, int ftype, Arm64Reg rd, Arm64Reg rn) {
    secbuf_emit32le(s, SF(sf) | 0x1e230000u | BITS(23, 22, ftype) | BITS(9, 5, rn) | BITS(4, 0, rd));
}
void arm64_fcvtzs(SecBuf *s, int sf, int ftype, Arm64Reg rd, Arm64Reg rn) {
    secbuf_emit32le(s, SF(sf) | 0x1e380000u | BITS(23, 22, ftype) | BITS(9, 5, rn) | BITS(4, 0, rd));
}
void arm64_fcvtzu(SecBuf *s, int sf, int ftype, Arm64Reg rd, Arm64Reg rn) {
    secbuf_emit32le(s, SF(sf) | 0x1e390000u | BITS(23, 22, ftype) | BITS(9, 5, rn) | BITS(4, 0, rd));
}

// FP load/store unsigned offset: sz=2→S(32bit), sz=3→D(64bit), sz=4→Q(128bit)
// LDR S: 0xBD400000, LDR D: 0xFD400000, LDR Q: 0x3DC00000
// uimm is byte offset; auto-scaled to element size
void arm64_ldr_fp(SecBuf *s, int sz, Arm64Reg rt, Arm64Reg rn, uint32_t uimm) {
    switch (sz) {
    case 2: secbuf_emit32le(s, 0xBD400000u | BITS(21, 10, uimm / 4) | BITS(9, 5, rn) | BITS(4, 0, rt)); break;
    case 3: secbuf_emit32le(s, 0xFD400000u | BITS(21, 10, uimm / 8) | BITS(9, 5, rn) | BITS(4, 0, rt)); break;
    default: secbuf_emit32le(s, 0x3DC00000u | BITS(21, 10, uimm / 16) | BITS(9, 5, rn) | BITS(4, 0, rt)); break;
    }
}
void arm64_str_fp(SecBuf *s, int sz, Arm64Reg rt, Arm64Reg rn, uint32_t uimm) {
    switch (sz) {
    case 2: secbuf_emit32le(s, 0xBD000000u | BITS(21, 10, uimm / 4) | BITS(9, 5, rn) | BITS(4, 0, rt)); break;
    case 3: secbuf_emit32le(s, 0xFD000000u | BITS(21, 10, uimm / 8) | BITS(9, 5, rn) | BITS(4, 0, rt)); break;
    default: secbuf_emit32le(s, 0x3D800000u | BITS(21, 10, uimm / 16) | BITS(9, 5, rn) | BITS(4, 0, rt)); break;
    }
}
// Unscaled FP/SIMD store (negative offsets). sz: 2=S, 3=D, default=Q
void arm64_stur_fp(SecBuf *s, int sz, Arm64Reg rt, Arm64Reg rn, int32_t imm9) {
    uint32_t imm = (uint32_t)imm9 & 0x1ff;
    switch (sz) {
    case 2: secbuf_emit32le(s, 0xBC000000u | BITS(20, 12, imm) | BITS(9, 5, rn) | BITS(4, 0, rt)); break;
    case 3: secbuf_emit32le(s, 0xFC000000u | BITS(20, 12, imm) | BITS(9, 5, rn) | BITS(4, 0, rt)); break;
    default: secbuf_emit32le(s, 0x3C800000u | BITS(20, 12, imm) | BITS(9, 5, rn) | BITS(4, 0, rt)); break;
    }
}
void arm64_ldp_fp(SecBuf *s, int opc, Arm64Reg rt1, Arm64Reg rt2, Arm64Reg rn, int32_t imm7, bool pre, bool post) {
    uint32_t base = 0x2c400000u | BITS(31, 30, opc);
    secbuf_emit32le(s, ldp_stp(base, rt1, rt2, rn, imm7, pre, post));
}
void arm64_stp_fp(SecBuf *s, int opc, Arm64Reg rt1, Arm64Reg rt2, Arm64Reg rn, int32_t imm7, bool pre, bool post) {
    uint32_t base = 0x2c000000u | BITS(31, 30, opc);
    secbuf_emit32le(s, ldp_stp(base, rt1, rt2, rn, imm7, pre, post));
}
#endif /* ARCH_ARM64 */
