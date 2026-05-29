// SPDX-License-Identifier: LGPL-2.1-or-later
// ARM64 (AArch64) instruction encoder — 32-bit fixed-width encoding.
// Emits into SecBuf via secbuf_emit32le() internally; takes SecBuf *s as first arg.
#ifndef ARM64_ENC_H
#define ARM64_ENC_H

#include <stdint.h>
#include <stdbool.h>
#include "obj.h"

// Register numbers (0-30 = x0-x30, 31 = xzr/sp depending on context)
#define ARM64_XZR 31
#define ARM64_SP  31

// ARM64 physical register enum
typedef enum {
    ARM64_S0 = 0,
    ARM64_D0 = 0,
    ARM64_X0 = 0,
    ARM64_X1,
    ARM64_X2,
    ARM64_X3,
    ARM64_X4,
    ARM64_X5,
    ARM64_X6,
    ARM64_X7,
    ARM64_X8,
    ARM64_X9,
    ARM64_X10,
    ARM64_X11,
    ARM64_X12,
    ARM64_X13,
    ARM64_X14,
    ARM64_X15,
    ARM64_X16,
    ARM64_X17,
    ARM64_X18,
    ARM64_X19,
    ARM64_X20,
    ARM64_X21,
    ARM64_X22,
    ARM64_X23,
    ARM64_X24,
    ARM64_X25,
    ARM64_X26,
    ARM64_X27,
    ARM64_X28,
    ARM64_X29 = 29, // FP / X29
    ARM64_X30 = 30, // LR
    ARM64_X31 = 31, // SP or XZR depending on context
} Arm64Reg;

// Condition codes (cond field in B.cond / CSEL etc.)
typedef enum {
    ARM64_EQ = 0,
    ARM64_NE = 1,
    ARM64_CS = 2,
    ARM64_CC = 3,
    ARM64_MI = 4,
    ARM64_PL = 5,
    ARM64_VS = 6,
    ARM64_VC = 7,
    ARM64_HI = 8,
    ARM64_LS = 9,
    ARM64_GE = 10,
    ARM64_LT = 11,
    ARM64_GT = 12,
    ARM64_LE = 13,
    ARM64_AL = 14,
    ARM64_NV = 15,
    // aliases
    ARM64_HS = 2,
    ARM64_LO = 3,
} Arm64Cond;

// Shift types
typedef enum { ARM64_LSL = 0,
               ARM64_LSR = 1,
               ARM64_ASR = 2,
               ARM64_ROR = 3 } Arm64Shift;

// Extend types (for load/store and ADD extended)
typedef enum {
    ARM64_UXTB = 0,
    ARM64_UXTH = 1,
    ARM64_UXTW = 2,
    ARM64_UXTX = 3,
    ARM64_SXTB = 4,
    ARM64_SXTH = 5,
    ARM64_SXTW = 6,
    ARM64_SXTX = 7,
} Arm64Ext;

// ---------------------------------------------------------------------------
// Data processing — immediate
// ---------------------------------------------------------------------------
void arm64_movz(SecBuf *s, int sf, Arm64Reg rd, uint16_t imm16, uint16_t shift16);
void arm64_movk(SecBuf *s, int sf, Arm64Reg rd, uint16_t imm16, uint16_t shift16);
void arm64_movn(SecBuf *s, int sf, Arm64Reg rd, uint16_t imm16, uint16_t shift16);
void arm64_add_imm(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, int32_t imm12, uint16_t shift2);
void arm64_adds_imm(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, int32_t imm12, uint32_t shift2);
void arm64_sub_imm(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, int32_t imm12, uint32_t shift2);
void arm64_subs_imm(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, int32_t imm12, uint32_t shift2);
void arm64_and_imm(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, uint64_t imm_enc);
void arm64_orr_imm(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, uint64_t imm_enc);
void arm64_eor_imm(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, uint64_t imm_enc);
void arm64_ands_imm(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, uint64_t imm_enc);

// ---------------------------------------------------------------------------
// Data processing — register
// ---------------------------------------------------------------------------
void arm64_add_reg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Shift sh, int imm6);
void arm64_adds_reg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Shift sh, int imm6);
void arm64_sub_reg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Shift sh, int imm6);
void arm64_subs_reg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Shift sh, int imm6);
void arm64_and_reg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Shift sh, int imm6);
void arm64_orr_reg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Shift sh, int imm6);
void arm64_eor_reg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Shift sh, int imm6);
void arm64_ands_reg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Shift sh, int imm6);
void arm64_bic_reg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Shift sh, int imm6);

// Extended register (supports SP register, e.g. sub sp, sp, x16)
void arm64_add_extreg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Ext option, int imm3);
void arm64_sub_extreg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Ext option, int imm3);
void arm64_mul(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm); // MADD xd,xn,xm,xzr
void arm64_smull(SecBuf *s, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm); // SMADDL
void arm64_umull(SecBuf *s, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm); // UMADDL
void arm64_smulh(SecBuf *s, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm);
void arm64_umulh(SecBuf *s, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm);
void arm64_sdiv(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm);
void arm64_udiv(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm);
void arm64_lsl_reg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm);
void arm64_lsr_reg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm);
void arm64_asr_reg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm);
void arm64_ror_reg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm);
void arm64_extr(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, int lsb);
void arm64_lsl_imm(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, int shift);
void arm64_lsr_imm(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, int shift);
void arm64_asr_imm(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, int shift);
void arm64_neg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rm);
void arm64_mvn(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rm, Arm64Shift sh, int imm6);
void arm64_clz(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn);
void arm64_cls(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn);
void arm64_rbit(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn);
void arm64_rev(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn);
void arm64_rev16(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn);
void arm64_rev32(SecBuf *s, Arm64Reg rd, Arm64Reg rn);
void arm64_sxtb(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn);
void arm64_sxth(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn);
void arm64_sxtw(SecBuf *s, Arm64Reg rd, Arm64Reg rn);
void arm64_uxtb(SecBuf *s, Arm64Reg rd, Arm64Reg rn);
void arm64_uxth(SecBuf *s, Arm64Reg rd, Arm64Reg rn);
void arm64_ubfx(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, int lsb, int width);
void arm64_sbfx(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, int lsb, int width);
void arm64_csel(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Cond cond);
void arm64_csinc(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Cond cond);
void arm64_csneg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm, Arm64Cond cond);
void arm64_cset(SecBuf *s, int sf, Arm64Reg rd, Arm64Cond cond);
void arm64_cneg(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Cond cond);
void arm64_adc(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm);
void arm64_sbc(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm);

// ---------------------------------------------------------------------------
// PC-relative addressing
// ---------------------------------------------------------------------------
// These encode 0 offset; caller adds reloc
void arm64_adrp(SecBuf *s, Arm64Reg rd, int32_t page_imm);
void arm64_adr(SecBuf *s, Arm64Reg rd, int32_t imm);

// ---------------------------------------------------------------------------
// Load/Store
// ---------------------------------------------------------------------------
void arm64_ldr_imm(SecBuf *s, int sf, Arm64Reg rt, Arm64Reg rn, int32_t imm9, bool pre);
void arm64_ldr_uoff(SecBuf *s, int sz, Arm64Reg rt, Arm64Reg rn, uint32_t uimm);
void arm64_ldrb_uoff(SecBuf *s, Arm64Reg rt, Arm64Reg rn, uint32_t uimm);
void arm64_ldrh_uoff(SecBuf *s, Arm64Reg rt, Arm64Reg rn, uint32_t uimm);
void arm64_ldrb_imm(SecBuf *s, Arm64Reg rt, Arm64Reg rn, int32_t imm9);
void arm64_ldrh_imm(SecBuf *s, Arm64Reg rt, Arm64Reg rn, int32_t imm9);
void arm64_ldrsb(SecBuf *s, int sf, Arm64Reg rt, Arm64Reg rn, int32_t imm9);
void arm64_ldrsh(SecBuf *s, int sf, Arm64Reg rt, Arm64Reg rn, int32_t imm9);
void arm64_ldrsw_imm(SecBuf *s, Arm64Reg rt, Arm64Reg rn, int32_t imm9);
void arm64_ldrsw_uoff(SecBuf *s, Arm64Reg rt, Arm64Reg rn, uint32_t uimm);
void arm64_str_imm(SecBuf *s, int sf, Arm64Reg rt, Arm64Reg rn, int32_t imm9, bool pre);
void arm64_str_uoff(SecBuf *s, int sz, Arm64Reg rt, Arm64Reg rn, uint32_t uimm);
void arm64_strb_uoff(SecBuf *s, Arm64Reg rt, Arm64Reg rn, uint32_t uimm);
void arm64_strh_uoff(SecBuf *s, Arm64Reg rt, Arm64Reg rn, uint32_t uimm);
void arm64_strb_imm(SecBuf *s, Arm64Reg rt, Arm64Reg rn, int32_t imm9);
void arm64_strh_imm(SecBuf *s, Arm64Reg rt, Arm64Reg rn, int32_t imm9);
// Pair: LDP/STP
void arm64_ldp(SecBuf *s, int sf, Arm64Reg rt1, Arm64Reg rt2, Arm64Reg rn, int32_t imm7, bool pre, bool post);
void arm64_stp(SecBuf *s, int sf, Arm64Reg rt1, Arm64Reg rt2, Arm64Reg rn, int32_t imm7, bool pre, bool post);
// Register offset
void arm64_ldr_reg(SecBuf *s, int sz, Arm64Reg rt, Arm64Reg rn, Arm64Reg rm, bool ext, int scale);
void arm64_str_reg(SecBuf *s, int sz, Arm64Reg rt, Arm64Reg rn, Arm64Reg rm, bool ext, int scale);
// Unscaled immediate (LDUR/STUR)
void arm64_ldur(SecBuf *s, int sf, Arm64Reg rt, Arm64Reg rn, int32_t imm9);
void arm64_ldurb(SecBuf *s, Arm64Reg rt, Arm64Reg rn, int32_t imm9);
void arm64_ldurh(SecBuf *s, Arm64Reg rt, Arm64Reg rn, int32_t imm9);
void arm64_stur(SecBuf *s, int sf, Arm64Reg rt, Arm64Reg rn, int32_t imm9);
void arm64_sturb(SecBuf *s, Arm64Reg rt, Arm64Reg rn, int32_t imm9);
void arm64_sturh(SecBuf *s, Arm64Reg rt, Arm64Reg rn, int32_t imm9);
// Load-exclusive / store-exclusive
void arm64_ldxr(SecBuf *s, int sf, Arm64Reg rt, Arm64Reg rn);
void arm64_ldxrb(SecBuf *s, Arm64Reg rt, Arm64Reg rn);
void arm64_ldxrh(SecBuf *s, Arm64Reg rt, Arm64Reg rn);
void arm64_stxr(SecBuf *s, int sf, Arm64Reg rs, Arm64Reg rt, Arm64Reg rn);
void arm64_stxrb(SecBuf *s, Arm64Reg rs, Arm64Reg rt, Arm64Reg rn);
void arm64_stxrh(SecBuf *s, Arm64Reg rs, Arm64Reg rt, Arm64Reg rn);
// Load/store acquire-release
void arm64_ldar(SecBuf *s, int sf, Arm64Reg rt, Arm64Reg rn);
void arm64_ldarb(SecBuf *s, Arm64Reg rt, Arm64Reg rn);
void arm64_ldarh(SecBuf *s, Arm64Reg rt, Arm64Reg rn);
void arm64_stlr(SecBuf *s, int sf, Arm64Reg rt, Arm64Reg rn);
void arm64_stlrb(SecBuf *s, Arm64Reg rt, Arm64Reg rn);
void arm64_stlrh(SecBuf *s, Arm64Reg rt, Arm64Reg rn);

// ---------------------------------------------------------------------------
// Branches
// ---------------------------------------------------------------------------
// imm26 / imm19 in instruction units (4 bytes each); caller scales
void arm64_b(SecBuf *s, int32_t imm26);
void arm64_bl(SecBuf *s, int32_t imm26);
void arm64_br(SecBuf *s, Arm64Reg rn);
void arm64_blr(SecBuf *s, Arm64Reg rn);
void arm64_ret(SecBuf *s, Arm64Reg rn); // rn=30 for normal return
void arm64_bcond(SecBuf *s, Arm64Cond cond, int32_t imm19);
void arm64_cbz(SecBuf *s, int sf, Arm64Reg rt, int32_t imm19);
void arm64_cbnz(SecBuf *s, int sf, Arm64Reg rt, int32_t imm19);
void arm64_tbz(SecBuf *s, Arm64Reg rt, int imm6, int32_t imm14);
void arm64_tbnz(SecBuf *s, Arm64Reg rt, int imm6, int32_t imm14);

// ---------------------------------------------------------------------------
// System / Misc
// ---------------------------------------------------------------------------
void arm64_nop(SecBuf *s);
void arm64_dmb(SecBuf *s, int opt); // opt=0xb=ish
void arm64_dsb(SecBuf *s, int opt);
void arm64_isb(SecBuf *s);
void arm64_mrs(SecBuf *s, Arm64Reg rt, uint32_t sys_reg);
void arm64_msr(SecBuf *s, uint32_t sys_reg, Arm64Reg rt);
void arm64_prfm_imm(SecBuf *s, int prfop, Arm64Reg rn, uint32_t uimm);

// ---------------------------------------------------------------------------
// FP / SIMD
// ---------------------------------------------------------------------------
void arm64_fmov_f2i(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn);
void arm64_fmov_i2f(SecBuf *s, int sf, Arm64Reg rd, Arm64Reg rn);
void arm64_fmov_imm(SecBuf *s, int ftype, Arm64Reg rd, uint8_t imm8);
void arm64_fadd(SecBuf *s, int ftype, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm);
void arm64_fsub(SecBuf *s, int ftype, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm);
void arm64_fmul(SecBuf *s, int ftype, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm);
void arm64_fdiv(SecBuf *s, int ftype, Arm64Reg rd, Arm64Reg rn, Arm64Reg rm);
void arm64_fneg(SecBuf *s, int ftype, Arm64Reg rd, Arm64Reg rn);
void arm64_fabs(SecBuf *s, int ftype, Arm64Reg rd, Arm64Reg rn);
void arm64_fcmp(SecBuf *s, int ftype, Arm64Reg rn, Arm64Reg rm);
void arm64_fcvt(SecBuf *s, int opc, int ftype, Arm64Reg rd, Arm64Reg rn); // ftype→opc conversion
void arm64_scvtf(SecBuf *s, int sf, int ftype, Arm64Reg rd, Arm64Reg rn);
void arm64_ucvtf(SecBuf *s, int sf, int ftype, Arm64Reg rd, Arm64Reg rn);
void arm64_fcvtzs(SecBuf *s, int sf, int ftype, Arm64Reg rd, Arm64Reg rn);
void arm64_fcvtzu(SecBuf *s, int sf, int ftype, Arm64Reg rd, Arm64Reg rn);
void arm64_ldr_fp(SecBuf *s, int opc, Arm64Reg rt, Arm64Reg rn, uint32_t uimm);
void arm64_str_fp(SecBuf *s, int opc, Arm64Reg rt, Arm64Reg rn, uint32_t uimm);
void arm64_stur_fp(SecBuf *s, int opc, Arm64Reg rt, Arm64Reg rn, int32_t imm9);
void arm64_ldp_fp(SecBuf *s, int opc, Arm64Reg rt1, Arm64Reg rt2, Arm64Reg rn, int32_t imm7, bool pre, bool post);
void arm64_stp_fp(SecBuf *s, int opc, Arm64Reg rt1, Arm64Reg rt2, Arm64Reg rn, int32_t imm7, bool pre, bool post);

// Helpers to build encode-immediate field from N/immr/imms bits
uint64_t arm64_encode_logic_imm(int sf, uint64_t val, int *N, int *immr, int *imms);

#endif // ARM64_ENC_H
