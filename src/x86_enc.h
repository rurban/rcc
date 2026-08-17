// SPDX-License-Identifier: LGPL-2.1-or-later
// x86-64 instruction encoder — variable-length encoding (AT&T syntax register names).
// Emits bytes directly into a SecBuf.
#ifndef X86_ENC_H
#define X86_ENC_H

#include "obj.h"
#include <stdint.h>
#include <stdbool.h>

// Register encoding (matches AT&T register order used by rcc)
// 64-bit registers
typedef enum {
    X86_RAX = 0,
    X86_RCX = 1,
    X86_RDX = 2,
    X86_RBX = 3,
    X86_RSP = 4,
    X86_RBP = 5,
    X86_RSI = 6,
    X86_RDI = 7,
    X86_R8 = 8,
    X86_R9 = 9,
    X86_R10 = 10,
    X86_R11 = 11,
    X86_R12 = 12,
    X86_R13 = 13,
    X86_R14 = 14,
    X86_R15 = 15,
    X86_RIP = 16,
    // Legacy 8-bit "high byte" pseudo-registers (AH/CH/DH/BH): only ever
    // valid as an 8-bit operand with NO REX prefix present (a REX prefix
    // repurposes indices 4-7 as SPL/BPL/SIL/DIL instead). Given distinct
    // values here (not reusing 4-7) so the encoder can tell "the user
    // wrote %spl" from "the user wrote %ah" -- x86_enc.c's modrm()/
    // maybe_rex()/x86_mov_{rr,rm,mr}() special-case this range: encode
    // the real 4-7 ModRM field but never let it force/contribute to a
    // REX byte.
    X86_AH = 17,
    X86_CH = 18,
    X86_DH = 19,
    X86_BH = 20,
    X86_NOREG = -1,
} X86Reg;

// XMM registers
typedef enum {
    X86_XMM0 = 0,
    X86_XMM1 = 1,
    X86_XMM2 = 2,
    X86_XMM3 = 3,
    X86_XMM4 = 4,
    X86_XMM5 = 5,
    X86_XMM6 = 6,
    X86_XMM7 = 7,
    X86_XMM8 = 8,
    X86_XMM9 = 9,
    X86_XMM10 = 10,
    X86_XMM11 = 11,
    X86_XMM12 = 12,
    X86_XMM13 = 13,
    X86_XMM14 = 14,
    X86_XMM15 = 15,
} X86XmmReg;

// Condition codes (for Jcc / SETcc / CMOVcc)
typedef enum {
    X86_O = 0,
    X86_NO = 1,
    X86_B = 2,
    X86_AE = 3,
    X86_E = 4,
    X86_NE = 5,
    X86_BE = 6,
    X86_A = 7,
    X86_S = 8,
    X86_NS = 9,
    X86_P = 10,
    X86_NP = 11,
    X86_L = 12,
    X86_GE = 13,
    X86_LE = 14,
    X86_G = 15,
    // aliases
    X86_C = 2,
    X86_NC = 3,
    X86_Z = 4,
    X86_NZ = 5,
    X86_NAE = 2,
    X86_NBE = 7,
    X86_NGE = 12,
    X86_NLE = 15,
} X86Cond;

// Memory operand: disp(base, index, scale)  index=-1 → no index
typedef struct {
    X86Reg base;
    X86Reg index;
    int scale; // 1,2,4,8
    int64_t disp;
} X86Mem;

static inline X86Mem x86_mem(X86Reg base, int64_t disp) {
    return (X86Mem){base, X86_NOREG, 1, disp};
}
static inline X86Mem x86_mem_idx(X86Reg base, X86Reg idx, int scale, int64_t disp) {
    return (X86Mem){base, idx, scale, disp};
}

// ---------------------------------------------------------------------------
// Integer instructions (size = 1/2/4/8 bytes)
// ---------------------------------------------------------------------------
void x86_mov_rr(SecBuf *s, int size, X86Reg dst, X86Reg src);
void x86_mov_ri(SecBuf *s, int size, X86Reg dst, int64_t imm);
void x86_mov_rm(SecBuf *s, int size, X86Reg dst, X86Mem srcm);
void x86_mov_mr(SecBuf *s, int size, X86Mem dstm, X86Reg src);
void x86_mov_mi(SecBuf *s, int size, X86Mem dstm, int32_t imm);
void x86_or_mi(SecBuf *s, int size, X86Mem dstm, int32_t imm);
void x86_cmp_mi(SecBuf *s, int size, X86Mem dstm, int32_t imm);
void x86_add_mi(SecBuf *s, int size, X86Mem dstm, int32_t imm);
void x86_sub_mi(SecBuf *s, int size, X86Mem dstm, int32_t imm);
void x86_and_mi(SecBuf *s, int size, X86Mem dstm, int32_t imm);
void x86_xor_mi(SecBuf *s, int size, X86Mem dstm, int32_t imm);
void x86_movabs(SecBuf *s, X86Reg dst, uint64_t imm64); // 64-bit immediate
void x86_movsx(SecBuf *s, int dst_sz, int src_sz, X86Reg dst, X86Reg src);
void x86_movzx(SecBuf *s, int dst_sz, int src_sz, X86Reg dst, X86Reg src);
void x86_movsx_rm(SecBuf *s, int dst_sz, int src_sz, X86Reg dst, X86Mem srcm);
void x86_movzx_rm(SecBuf *s, int dst_sz, int src_sz, X86Reg dst, X86Mem srcm);

void x86_lea(SecBuf *s, int size, X86Reg dst, X86Mem src);

// Arithmetic
void x86_add_rr(SecBuf *s, int size, X86Reg dst, X86Reg src);
void x86_add_ri(SecBuf *s, int size, X86Reg dst, int32_t imm);
void x86_add_rm(SecBuf *s, int size, X86Reg dst, X86Mem srcm);
void x86_add_mr(SecBuf *s, int size, X86Mem dstm, X86Reg src);
void x86_sub_rr(SecBuf *s, int size, X86Reg dst, X86Reg src);
void x86_sub_ri(SecBuf *s, int size, X86Reg dst, int32_t imm);
void x86_sub_rm(SecBuf *s, int size, X86Reg dst, X86Mem srcm);
void x86_sub_mr(SecBuf *s, int size, X86Mem dstm, X86Reg src);
void x86_imul_rr(SecBuf *s, int size, X86Reg dst, X86Reg src);
void x86_imul_rri(SecBuf *s, int size, X86Reg dst, X86Reg src, int32_t imm);
void x86_imul_r(SecBuf *s, int size, X86Reg src); // RDX:RAX = RAX*src
void x86_idiv_r(SecBuf *s, int size, X86Reg src);
void x86_div_r(SecBuf *s, int size, X86Reg src);
void x86_neg_r(SecBuf *s, int size, X86Reg r);
void x86_not_r(SecBuf *s, int size, X86Reg r);
void x86_inc_r(SecBuf *s, int size, X86Reg r);
void x86_dec_r(SecBuf *s, int size, X86Reg r);
void x86_inc_m(SecBuf *s, int size, X86Mem m);
void x86_dec_m(SecBuf *s, int size, X86Mem m);
void x86_neg_m(SecBuf *s, int size, X86Mem m);
void x86_not_m(SecBuf *s, int size, X86Mem m);
void x86_cdq(SecBuf *s); // sign-extend EAX to EDX:EAX (AT&T alias: cltd)
void x86_cqo(SecBuf *s); // sign-extend RAX to RDX:RAX (AT&T alias: cqto)
void x86_cbw(SecBuf *s); // sign-extend AL to AX (AT&T alias: cbtw)
void x86_cwde(SecBuf *s); // sign-extend AX to EAX (AT&T alias: cwtl)
void x86_cdqe(SecBuf *s); // sign-extend EAX to RAX (AT&T alias: cltq)
void x86_cwd(SecBuf *s); // sign-extend AX to DX:AX (AT&T alias: cwtd)

// Logical
void x86_and_rr(SecBuf *s, int size, X86Reg dst, X86Reg src);
void x86_and_ri(SecBuf *s, int size, X86Reg dst, int32_t imm);
void x86_and_rm(SecBuf *s, int size, X86Reg dst, X86Mem srcm);
void x86_and_mr(SecBuf *s, int size, X86Mem dstm, X86Reg src);
void x86_or_rr(SecBuf *s, int size, X86Reg dst, X86Reg src);
void x86_or_ri(SecBuf *s, int size, X86Reg dst, int32_t imm);
void x86_or_rm(SecBuf *s, int size, X86Reg dst, X86Mem srcm);
void x86_or_mr(SecBuf *s, int size, X86Mem dstm, X86Reg src);
void x86_xor_rr(SecBuf *s, int size, X86Reg dst, X86Reg src);
void x86_xor_ri(SecBuf *s, int size, X86Reg dst, int32_t imm);
void x86_xor_rm(SecBuf *s, int size, X86Reg dst, X86Mem srcm);
void x86_xor_mr(SecBuf *s, int size, X86Mem dstm, X86Reg src);

// Shifts
void x86_shl_ri(SecBuf *s, int size, X86Reg r, uint8_t imm);
void x86_shr_ri(SecBuf *s, int size, X86Reg r, uint8_t imm);
void x86_sar_ri(SecBuf *s, int size, X86Reg r, uint8_t imm);
void x86_shl_rcl(SecBuf *s, int size, X86Reg r); // shift by CL
void x86_shr_rcl(SecBuf *s, int size, X86Reg r);
void x86_sar_rcl(SecBuf *s, int size, X86Reg r);
void x86_ror_ri(SecBuf *s, int size, X86Reg r, uint8_t imm);
void x86_rol_ri(SecBuf *s, int size, X86Reg r, uint8_t imm);
// BMI2 register-count shifts (VEX.NDS.LZ.pp.0F38.W? F7 /r): unlike the
// legacy shl/shr/sar-by-CL forms, the shift count is an explicit third
// operand (any GP register, not just CL) and neither source register nor
// FLAGS are clobbered. AT&T operand order is "shrx %count, %src, %dst".
void x86_shlx_rr(SecBuf *s, int size, X86Reg dst, X86Reg src, X86Reg count);
void x86_shrx_rr(SecBuf *s, int size, X86Reg dst, X86Reg src, X86Reg count);
void x86_sarx_rr(SecBuf *s, int size, X86Reg dst, X86Reg src, X86Reg count);
// RCL/RCR (rotate-through-carry): opcode group 2, /r == 2/3. Real GAS
// accepts both "rcr $imm, reg" (or the bare-register form implying
// $1, e.g. GMP's own mpn/x86_64/*.asm) and "rcr %cl, reg".
void x86_rcl_ri(SecBuf *s, int size, X86Reg r, uint8_t imm);
void x86_rcr_ri(SecBuf *s, int size, X86Reg r, uint8_t imm);
void x86_rcl_rcl(SecBuf *s, int size, X86Reg r); // rotate by CL
void x86_rcr_rcl(SecBuf *s, int size, X86Reg r);
void x86_rol_rcl(SecBuf *s, int size, X86Reg r);
void x86_ror_rcl(SecBuf *s, int size, X86Reg r);

// Compare / test
void x86_cmp_rr(SecBuf *s, int size, X86Reg a, X86Reg b);
void x86_cmp_ri(SecBuf *s, int size, X86Reg a, int32_t imm);
void x86_cmp_rm(SecBuf *s, int size, X86Reg a, X86Mem bm);
void x86_cmp_mr(SecBuf *s, int size, X86Mem am, X86Reg b);
void x86_test_rr(SecBuf *s, int size, X86Reg a, X86Reg b);
void x86_test_ri(SecBuf *s, int size, X86Reg a, int32_t imm);

// Set/conditional move
void x86_setcc(SecBuf *s, X86Cond cc, X86Reg dst);
void x86_cmovcc(SecBuf *s, int size, X86Cond cc, X86Reg dst, X86Reg src);

// Bit operations
void x86_bsf(SecBuf *s, int size, X86Reg dst, X86Reg src);
void x86_bsr(SecBuf *s, int size, X86Reg dst, X86Reg src);
void x86_bsf_rm(SecBuf *s, int size, X86Reg dst, X86Mem src);
void x86_bsr_rm(SecBuf *s, int size, X86Reg dst, X86Mem src);
void x86_popcnt(SecBuf *s, int size, X86Reg dst, X86Reg src);
void x86_lzcnt(SecBuf *s, int size, X86Reg dst, X86Reg src);
void x86_tzcnt(SecBuf *s, int size, X86Reg dst, X86Reg src);
void x86_adcx_rr(SecBuf *s, int size, X86Reg dst, X86Reg src);
void x86_adox_rr(SecBuf *s, int size, X86Reg dst, X86Reg src);
void x86_adcx_rm(SecBuf *s, int size, X86Reg dst, X86Mem src);
void x86_adox_rm(SecBuf *s, int size, X86Reg dst, X86Mem src);
void x86_bswap(SecBuf *s, int size, X86Reg r); // size=4 or 8

// BT/BTS/BTR/BTC r/m, r (memory or register bit-index destination)
void x86_bt_mr(SecBuf *s, int size, X86Mem dst, X86Reg src);
void x86_bts_mr(SecBuf *s, int size, X86Mem dst, X86Reg src);
void x86_btr_mr(SecBuf *s, int size, X86Mem dst, X86Reg src);
void x86_btc_mr(SecBuf *s, int size, X86Mem dst, X86Reg src);
void x86_bt_rr(SecBuf *s, int size, X86Reg dst, X86Reg src);
void x86_bts_rr(SecBuf *s, int size, X86Reg dst, X86Reg src);
void x86_btr_rr(SecBuf *s, int size, X86Reg dst, X86Reg src);
void x86_btc_rr(SecBuf *s, int size, X86Reg dst, X86Reg src);

// XADD r/m, r: adds src to dst, stores dst's original value into src
void x86_xadd_mr(SecBuf *s, int size, X86Mem dst, X86Reg src);
void x86_xadd_rr(SecBuf *s, int size, X86Reg dst, X86Reg src);

// CMPXCHG r/m, r: compares r/m with %al/%eax/%rax; if equal, r/m = src
void x86_cmpxchg_mr(SecBuf *s, int size, X86Mem dst, X86Reg src);
void x86_cmpxchg_rr(SecBuf *s, int size, X86Reg dst, X86Reg src);

// Stack
void x86_push(SecBuf *s, X86Reg r);
void x86_push_m(SecBuf *s, X86Mem m);
void x86_pop(SecBuf *s, X86Reg r);
void x86_push_imm(SecBuf *s, int32_t imm);

// Control flow
void x86_call_rel32(SecBuf *s, int32_t rel32); // fills 0; caller adds reloc
void x86_call_r(SecBuf *s, X86Reg r);
void x86_call_m(SecBuf *s, X86Mem m);
void x86_jmp_rel32(SecBuf *s, int32_t rel32);
void x86_jmp_rel8(SecBuf *s, int8_t rel8);
void x86_jmp_r(SecBuf *s, X86Reg r);
void x86_jmp_m(SecBuf *s, X86Mem m);
void x86_jcc_rel32(SecBuf *s, X86Cond cc, int32_t rel32);
void x86_jcc_rel8(SecBuf *s, X86Cond cc, int8_t rel8);
void x86_ret(SecBuf *s);
void x86_leave(SecBuf *s);
void x86_nop(SecBuf *s);
void x86_int(SecBuf *s, uint8_t imm8);

// Misc
void x86_xchg_rr(SecBuf *s, int size, X86Reg a, X86Reg b);
void x86_lock_prefix(SecBuf *s);
void x86_rep_prefix(SecBuf *s);
void x86_repne_prefix(SecBuf *s);
void x86_seg_prefix(SecBuf *s, uint8_t byte);
void x86_cld(SecBuf *s);
void x86_stosb(SecBuf *s);
void x86_movsb(SecBuf *s);
void x86_cmpsb(SecBuf *s);
void x86_scasb(SecBuf *s);
void x86_movs(SecBuf *s, int size);
void x86_stos(SecBuf *s, int size);
void x86_cmps(SecBuf *s, int size);
void x86_scas(SecBuf *s, int size);
void x86_lods(SecBuf *s, int size);
void x86_mfence(SecBuf *s);
void x86_lfence(SecBuf *s);
void x86_sfence(SecBuf *s);
void x86_cpuid(SecBuf *s);
void x86_ud2(SecBuf *s);
void x86_rdtsc(SecBuf *s);
void x86_rdtscp(SecBuf *s);
void x86_clac(SecBuf *s);
void x86_stac(SecBuf *s);
void x86_iretq(SecBuf *s);
void x86_lahf(SecBuf *s);
void x86_sahf(SecBuf *s);
void x86_clc(SecBuf *s);
void x86_stc(SecBuf *s);
void x86_std(SecBuf *s);
void x86_endbr32(SecBuf *s);
void x86_endbr64(SecBuf *s);
void x86_int3(SecBuf *s);
void x86_int1(SecBuf *s);
void x86_syscall(SecBuf *s);
void x86_sysenter(SecBuf *s);
void x86_sysexit(SecBuf *s);
void x86_sysret(SecBuf *s);
void x86_sysretq(SecBuf *s);
void x86_rdrand(SecBuf *s, int size, X86Reg r);
void x86_rdseed(SecBuf *s, int size, X86Reg r);
void x86_crc32(SecBuf *s, int dst_size, int src_size, X86Reg dst, X86Reg src);
void x86_invpcid(SecBuf *s, X86Reg type_reg, X86Mem desc);
void x86_rdfsbase(SecBuf *s, int size, X86Reg r);
void x86_rdgsbase(SecBuf *s, int size, X86Reg r);
void x86_wrfsbase(SecBuf *s, int size, X86Reg r);
void x86_wrgsbase(SecBuf *s, int size, X86Reg r);
void x86_mul_r(SecBuf *s, int size, X86Reg src);
void x86_mul_m(SecBuf *s, int size, X86Mem src);
void x86_adc_rr(SecBuf *s, int size, X86Reg dst, X86Reg src);
void x86_adc_ri(SecBuf *s, int size, X86Reg dst, int32_t imm);
void x86_adc_rm(SecBuf *s, int size, X86Reg dst, X86Mem src);
void x86_adc_mr(SecBuf *s, int size, X86Mem dst, X86Reg src);
void x86_adc_mi(SecBuf *s, int size, X86Mem dst, int32_t imm);
void x86_sbb_rr(SecBuf *s, int size, X86Reg dst, X86Reg src);
void x86_sbb_ri(SecBuf *s, int size, X86Reg dst, int32_t imm);
void x86_sbb_rm(SecBuf *s, int size, X86Reg dst, X86Mem src);
void x86_sbb_mr(SecBuf *s, int size, X86Mem dst, X86Reg src);
void x86_sbb_mi(SecBuf *s, int size, X86Mem dst, int32_t imm);
void x86_rdmsr(SecBuf *s);
void x86_wrmsr(SecBuf *s);
void x86_cmpxchg8b_m(SecBuf *s, X86Mem m);
void x86_cmpxchg16b_m(SecBuf *s, X86Mem m);
void x86_wbinvd(SecBuf *s);
void x86_sti(SecBuf *s);
void x86_cli(SecBuf *s);
void x86_hlt(SecBuf *s);
void x86_pushfq(SecBuf *s);
void x86_popfq(SecBuf *s);
// Port I/O: "outb %al, %dx" / "outb %al, $imm8", "inb %dx, %al" / "inb $imm8, %al"
void x86_outb_dx(SecBuf *s);
void x86_outw_dx(SecBuf *s);
void x86_outl_dx(SecBuf *s);
void x86_outb_imm(SecBuf *s, uint8_t imm8);
void x86_outw_imm(SecBuf *s, uint8_t imm8);
void x86_outl_imm(SecBuf *s, uint8_t imm8);
void x86_inb_dx(SecBuf *s);
void x86_inw_dx(SecBuf *s);
void x86_inl_dx(SecBuf *s);
void x86_inb_imm(SecBuf *s, uint8_t imm8);
void x86_inw_imm(SecBuf *s, uint8_t imm8);
void x86_inl_imm(SecBuf *s, uint8_t imm8);
// String port I/O: "insb"/"insw"/"insl", "outsb"/"outsw"/"outsl" (implicit
// %dx port, (%rsi)/(%rdi) memory operand — no explicit operands in GAS)
void x86_insb(SecBuf *s);
void x86_insw(SecBuf *s);
void x86_insl(SecBuf *s);
void x86_outsb(SecBuf *s);
void x86_outsw(SecBuf *s);
void x86_outsl(SecBuf *s);
void x86_vmcall(SecBuf *s);
void x86_vmmcall(SecBuf *s);
void x86_prefetcht0(SecBuf *s, X86Mem m);
void x86_prefetchnta(SecBuf *s, X86Mem m);
void x86_prefetchw(SecBuf *s, X86Mem m);
void x86_clflush(SecBuf *s, X86Mem m);
void x86_clflushopt(SecBuf *s, X86Mem m);
void x86_clwb(SecBuf *s, X86Mem m);
void x86_lgdt(SecBuf *s, X86Mem m);
void x86_lidt(SecBuf *s, X86Mem m);
void x86_sgdt(SecBuf *s, X86Mem m);
void x86_sidt(SecBuf *s, X86Mem m);
void x86_invlpg(SecBuf *s, X86Mem m);
void x86_lldt_r(SecBuf *s, X86Reg r);
void x86_lldt_m(SecBuf *s, X86Mem m);
void x86_ltr_r(SecBuf *s, X86Reg r);
void x86_ltr_m(SecBuf *s, X86Mem m);
void x86_str_r(SecBuf *s, X86Reg r);
void x86_str_m(SecBuf *s, X86Mem m);
void x86_pause(SecBuf *s);
void x86_swapgs(SecBuf *s);
void x86_rdpmc(SecBuf *s);
void x86_rdpkru(SecBuf *s);
void x86_wrpkru(SecBuf *s);
void x86_verw_m(SecBuf *s, X86Mem m);
void x86_rdpid(SecBuf *s, X86Reg dst);
void x86_lsl_rr(SecBuf *s, X86Reg src, X86Reg dst);
void x86_lsl_rm(SecBuf *s, X86Mem src, X86Reg dst);
void x86_cmc(SecBuf *s);
void x86_clts(SecBuf *s);
void x86_invd(SecBuf *s);
void x86_wbnoinvd(SecBuf *s);
void x86_wait(SecBuf *s);
void x86_xgetbv(SecBuf *s);
void x86_xsetbv(SecBuf *s);
void x86_serialize(SecBuf *s);
void x86_verr_m(SecBuf *s, X86Mem m);
void x86_lar_rr(SecBuf *s, X86Reg src, X86Reg dst);
void x86_lar_rm(SecBuf *s, X86Mem src, X86Reg dst);
void x86_smsw_r(SecBuf *s, X86Reg r);
void x86_smsw_m(SecBuf *s, X86Mem m);
void x86_lmsw_r(SecBuf *s, X86Reg r);
void x86_lmsw_m(SecBuf *s, X86Mem m);
void x86_sldt_r(SecBuf *s, X86Reg r);
void x86_sldt_m(SecBuf *s, X86Mem m);
void x86_ud0(SecBuf *s, X86Reg dst, X86Reg src);
void x86_ud1(SecBuf *s, X86Reg dst, X86Reg src);
void x86_ud1_m(SecBuf *s, bool addr32, X86Reg reg, X86Mem m);
// FXSAVE/FXRSTOR/XSAVE-family (0F AE and 0F C7 sub-opcodes): all take a
// single memory operand — the "64" name variants are the same opcode with
// REX.W forced, not a different opcode.
void x86_fxsave(SecBuf *s, int w, X86Mem m);
void x86_fxrstor(SecBuf *s, int w, X86Mem m);
void x86_xsave(SecBuf *s, int w, X86Mem m);
void x86_xrstor(SecBuf *s, int w, X86Mem m);
void x86_xsaveopt(SecBuf *s, int w, X86Mem m);
void x86_xsavec(SecBuf *s, int w, X86Mem m);
void x86_xsaves(SecBuf *s, int w, X86Mem m);
void x86_xrstors(SecBuf *s, int w, X86Mem m);

// x87 FPU control/status (init/exception-handling subset only — the
// arithmetic instruction family is deferred, the kernel avoids x87 math
// outside very specific paths). Each "fXXX" waiting form is the "fnXXX"
// non-waiting form with an implicit leading FWAIT (0x9B).
void x86_fninit(SecBuf *s);
void x86_finit(SecBuf *s);
void x86_fnclex(SecBuf *s);
void x86_fclex(SecBuf *s);
void x86_fnop(SecBuf *s);
void x86_fldcw_m(SecBuf *s, X86Mem m);
void x86_fnstcw_m(SecBuf *s, X86Mem m);
void x86_fstcw_m(SecBuf *s, X86Mem m);
void x86_fnstenv_m(SecBuf *s, X86Mem m); // store FPU environment (14/28 bytes)
void x86_fstenv_m(SecBuf *s, X86Mem m);
void x86_fnstsw_m(SecBuf *s, X86Mem m);
void x86_fnstsw_ax(SecBuf *s);
void x86_fstsw_m(SecBuf *s, X86Mem m);
void x86_fstsw_ax(SecBuf *s);

// Port I/O, far return, stack frame, misc system
void x86_retf(SecBuf *s);
void x86_retf_imm(SecBuf *s, uint16_t imm16);
void x86_enter(SecBuf *s, uint16_t frame_size, uint8_t nesting);
void x86_prefetcht1(SecBuf *s, X86Mem m);
void x86_prefetcht2(SecBuf *s, X86Mem m);
void x86_prefetchwt1(SecBuf *s, X86Mem m);
void x86_rsm(SecBuf *s);
void x86_xtest(SecBuf *s);
void x86_xend(SecBuf *s);
void x86_clzero(SecBuf *s);
void x86_cldemote_m(SecBuf *s, X86Mem m);
void x86_xabort(SecBuf *s, uint8_t imm8);
void x86_xbegin_rel32(SecBuf *s, int32_t rel32); // fills 0; caller adds fixup

// SSE / FP
void x86_movsd_rr(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_movsd_rm(SecBuf *s, X86XmmReg dst, X86Mem srcm);
void x86_movsd_mr(SecBuf *s, X86Mem dstm, X86XmmReg src);
void x86_movss_rr(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_movss_rm(SecBuf *s, X86XmmReg dst, X86Mem src);
void x86_movss_mr(SecBuf *s, X86Mem dstm, X86XmmReg src);
void x86_addsd(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_subsd(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_mulsd(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_divsd(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_addss(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_subss(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_mulss(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_divss(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_movq_r_xmm(SecBuf *s, X86XmmReg d, X86Reg src);
void x86_movq_xmm_r(SecBuf *s, X86Reg d, X86XmmReg src);
void x86_ucomisd(SecBuf *s, X86XmmReg a, X86XmmReg b);
void x86_ucomiss(SecBuf *s, X86XmmReg a, X86XmmReg b);
void x86_comisd(SecBuf *s, X86XmmReg a, X86XmmReg b);
void x86_cvtsi2sd(SecBuf *s, int src_size, X86XmmReg dst, X86Reg src);
void x86_cvtsi2ss(SecBuf *s, int src_size, X86XmmReg dst, X86Reg src);
void x86_cvttsd2si(SecBuf *s, int dst_size, X86Reg dst, X86XmmReg src);
void x86_cvttss2si(SecBuf *s, int dst_size, X86Reg dst, X86XmmReg src);
void x86_cvtsd2ss(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_cvtss2sd(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_xorpd(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_xorps(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_movaps(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_movaps_rm(SecBuf *s, X86XmmReg dst, X86Mem src);
void x86_movaps_mr(SecBuf *s, X86Mem dstm, X86XmmReg src);
void x86_movdqu_mr(SecBuf *s, X86Mem dstm, X86XmmReg src);
void x86_pxor(SecBuf *s, X86XmmReg dst, X86XmmReg src);

// Packed SSE (128-bit vector) for __attribute__((vector_size(16)))
void x86_movups_rm(SecBuf *s, X86XmmReg dst, X86Mem src);
void x86_movups_mr(SecBuf *s, X86Mem dstm, X86XmmReg src);
void x86_addps(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_subps(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_mulps(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_divps(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_minps(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_maxps(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_andps(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_andnps(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_orps(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_sqrtps(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_sqrtss(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_rsqrtps(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_sqrtpd(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_sqrtsd(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_rcpps(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_unpcklps(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_unpckhps(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_movhlps(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_movlhps(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_cmpps(SecBuf *s, X86XmmReg dst, X86XmmReg src, uint8_t imm);
void x86_shufps(SecBuf *s, X86XmmReg dst, X86XmmReg src, uint8_t imm);
void x86_shufpd(SecBuf *s, X86XmmReg dst, X86XmmReg src, uint8_t imm);
void x86_movsldup(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_movshdup(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_movmskps(SecBuf *s, X86Reg dst, X86XmmReg src);
void x86_movmskpd(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pmovmskb(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_movntdqa_rm(SecBuf *s, X86Mem m, X86XmmReg d);

// Low-level byte emission helpers (shared with codegen.c's intrinsic dispatch).
void emit1(SecBuf *s, uint8_t b);
void emit2(SecBuf *s, uint8_t a, uint8_t b);
void emit3(SecBuf *s, uint8_t a, uint8_t b, uint8_t c);
uint8_t modrxmm(int mod, X86XmmReg reg, X86XmmReg rm);
void maybe_rex(SecBuf *s, int W, int R, int X, int B);
void sse_rr_66(SecBuf *s, uint8_t op, X86XmmReg d, X86XmmReg sr);
void sse_rr_f3(SecBuf *s, uint8_t op, X86XmmReg d, X86XmmReg sr);
void sse_rr_f2(SecBuf *s, uint8_t op, X86XmmReg d, X86XmmReg sr);
void sse_rr_np(SecBuf *s, uint8_t op, X86XmmReg d, X86XmmReg sr);
void x86_addpd(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_subpd(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_mulpd(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_divpd(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_andpd(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_orpd(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_paddd(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_psubd(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_paddq(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_psubq(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_paddw(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_psubw(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_paddb(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_psubb(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_pand(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_por(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_pcmpeqd(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_pcmpgtd(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_paddd_rm(SecBuf *s, X86XmmReg dst, X86Mem src);
void x86_psubd_rm(SecBuf *s, X86XmmReg dst, X86Mem src);
void x86_paddq_rm(SecBuf *s, X86XmmReg dst, X86Mem src);
void x86_psubq_rm(SecBuf *s, X86XmmReg dst, X86Mem src);
void x86_paddw_rm(SecBuf *s, X86XmmReg dst, X86Mem src);
void x86_psubw_rm(SecBuf *s, X86XmmReg dst, X86Mem src);
void x86_paddb_rm(SecBuf *s, X86XmmReg dst, X86Mem src);
void x86_psubb_rm(SecBuf *s, X86XmmReg dst, X86Mem src);
void x86_pand_rm(SecBuf *s, X86XmmReg dst, X86Mem src);
void x86_por_rm(SecBuf *s, X86XmmReg dst, X86Mem src);
void x86_pcmpeqd_rm(SecBuf *s, X86XmmReg dst, X86Mem src);
void x86_pcmpgtd_rm(SecBuf *s, X86XmmReg dst, X86Mem src);
// PSHUFB xmm, xmm (SSSE3): byte-lane shuffle/permute, dst = shuffle(dst, src)
void x86_pshufb(SecBuf *s, X86XmmReg dst, X86XmmReg src);
// PSHUFD xmm, xmm, imm8 (SSE2, 66 0F 70 /r ib): dword-lane shuffle,
// dst = shuffle(src, imm) -- unlike PSHUFB's dst-in-place form, this
// reads only from src (dst is write-only), matching real GAS's own
// "pshufd $imm, src, dst" AT&T operand order.
void x86_pshufd(SecBuf *s, X86XmmReg dst, X86XmmReg src, uint8_t imm);
// "Group 14" shift-by-immediate (SSE2, 66 0F 73 /ext ib): the whole
// xmm register shifts by imm8, zero-filling; single-operand form (dst
// is both source and destination). PSLLDQ/PSRLDQ shift by *bytes*
// (whole-register, lane-agnostic); PSLLQ/PSRLQ shift each 64-bit lane
// independently.
void x86_pslldq(SecBuf *s, X86XmmReg dst, uint8_t imm);
void x86_psrldq(SecBuf *s, X86XmmReg dst, uint8_t imm);
void x86_psllq(SecBuf *s, X86XmmReg dst, uint8_t imm);
void x86_psrlq(SecBuf *s, X86XmmReg dst, uint8_t imm);
void x86_pslld(SecBuf *s, X86XmmReg dst, uint8_t imm);
void x86_psrld(SecBuf *s, X86XmmReg dst, uint8_t imm);
// AES-NI (66 0F 38 xx /r): one round of AES encryption/decryption, or
// the inverse-mix-columns step used when expanding a decryption key
// schedule from an encryption one.
void x86_aesenc(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_aesenclast(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_aesdec(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_aesdeclast(SecBuf *s, X86XmmReg dst, X86XmmReg src);
void x86_aesimc(SecBuf *s, X86XmmReg dst, X86XmmReg src);
// AESKEYGENASSIST xmm, xmm, imm8 (66 0F 3A DF /r ib): one round of the
// AES key schedule (round constant in imm8).
void x86_aeskeygenassist(SecBuf *s, X86XmmReg dst, X86XmmReg src, uint8_t imm);
// PINSRW xmm, r32/m16, imm8 (66 0F C4 /r ib): insert a 16-bit word
// into one of the xmm register's 8 word lanes (imm8 selects which).
void x86_pinsrw_rm(SecBuf *s, X86XmmReg dst, X86Mem srcm, uint8_t imm);
// Intel SHA extensions.
void x86_sha1msg1(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_sha1msg2(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_sha1nexte(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_sha1rnds4(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm);
// PEXTRD r/m32, xmm, imm8 (66 0F 3A 16 /r ib): extract a 32-bit dword
// lane (imm8 selects which of 4) from an xmm register to a GP register
// or memory.
void x86_pextrd_r(SecBuf *s, X86Reg d, X86XmmReg sr, uint8_t imm);
void x86_pextrd_m(SecBuf *s, X86Mem m, X86XmmReg sr, uint8_t imm);
// SHA256RNDS2/MSG1/MSG2 (implicit XMM0 3rd operand for RNDS2).
void x86_sha256rnds2(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_sha256msg1(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_sha256msg2(SecBuf *s, X86XmmReg d, X86XmmReg sr);
// PINSRD xmm, r/m32, imm8 (66 0F 3A 22 /r ib): insert a 32-bit dword
// lane (imm8 selects which of 4) from a GP register or memory.
void x86_pinsrd_r(SecBuf *s, X86XmmReg d, X86Reg sr, uint8_t imm);
void x86_pinsrd_m(SecBuf *s, X86XmmReg d, X86Mem m, uint8_t imm);

void x86_minpd(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_maxpd(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_unpcklpd(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_unpckhpd(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_cvttps2pi(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_cvtps2pi(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_cvtpi2ps(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_cvtps2pd(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_cvtdq2ps(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_comiss(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pcmpeqb(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pcmpeqw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pcmpgtb(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pcmpgtw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_paddsb(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_paddsw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_paddusb(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_paddusw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_psubsb(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_psubsw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_psubusb(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_psubusw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pmullw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pmulhw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pmulhuw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pmuludq(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pmaddwd(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pavgb(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pavgw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_psadbw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pmaxub(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pmaxsw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pminub(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pminsw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_punpcklbw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_punpcklwd(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_punpckldq(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_punpcklqdq(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_punpckhbw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_punpckhwd(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_punpckhdq(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_punpckhqdq(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_packsswb(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_packssdw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_packuswb(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_addsubpd(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_haddpd(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_hsubpd(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_cvtpd2ps(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_cvtps2dq(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_cvttpd2dq(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_cvtpi2pd(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_cvtpd2pi(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_cvttpd2pi(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_maxss(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_maxsd(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_minss(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_minsd(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_rcpss(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_rsqrtss(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_cvtsd2si(SecBuf *s, int dstsz, X86Reg d, X86XmmReg sr);
void x86_cvtss2si(SecBuf *s, int dstsz, X86Reg d, X86XmmReg sr);
void x86_psllq_r(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_psrlq_r(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_psrlw_r(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_psrld_r(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_psraw_r(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_psrad_r(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_psllw_r(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pslld_r(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_maskmovdqu(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_cvtdq2pd(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_cvtpd2dq(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_addsubps(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_haddps(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_hsubps(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pshuflw(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm);
void x86_pshufhw(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm);
void x86_pshufw(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm);
void x86_cmpss(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm);
void x86_cmpsd(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm);
void x86_cmppd(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm);
void x86_phaddw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_phaddd(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_phaddsw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pmaddubsw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_phsubw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_phsubd(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_phsubsw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_psignb(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_psignw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_psignd(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pmulhrsw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pabsb(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pabsw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pabsd(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pblendvb(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_blendvps(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_blendvpd(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_ptest(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pmovsxbw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pmovsxbd(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pmovsxbq(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pmovsxwd(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pmovsxwq(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pmovsxdq(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pmuldq(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pcmpeqq(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_movntdqa(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_packusdw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pmovzxbw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pmovzxbd(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pmovzxbq(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pmovzxwd(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pmovzxwq(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pmovzxdq(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pcmpgtq(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pminsb(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pminsd(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pminuw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pminud(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pmaxsb(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pmaxsd(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pmaxuw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pmaxud(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_pmulld(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_phminposuw(SecBuf *s, X86XmmReg d, X86XmmReg sr);
void x86_roundps(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm);
void x86_roundpd(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm);
void x86_roundss(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm);
void x86_roundsd(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm);
void x86_blendps(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm);
void x86_blendpd(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm);
void x86_pblendw(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm);
void x86_palignr(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm);
void x86_insertps(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm);
void x86_dpps(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm);
void x86_dppd(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm);
void x86_mpsadbw(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm);
void x86_pclmulqdq(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm);
void x86_pcmpestrm(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm);
void x86_pcmpestri(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm);
void x86_pcmpistrm(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm);
void x86_pcmpistri(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm);
void x86_movdqa_rm(SecBuf *s, X86Mem srcm, X86XmmReg dst);
void x86_movdqa_mr(SecBuf *s, X86Mem dstm, X86XmmReg src);
void x86_movdqu_rm(SecBuf *s, X86Mem srcm, X86XmmReg dst);
void x86_movq_rm(SecBuf *s, X86Mem srcm, X86XmmReg dst);
void x86_movq_mr(SecBuf *s, X86Mem dstm, X86XmmReg src);
void x86_movd_r_xmm(SecBuf *s, X86XmmReg dst, X86Reg src);
void x86_movd_xmm_r(SecBuf *s, X86Reg dst, X86XmmReg src);
void x86_pextrw(SecBuf *s, X86Reg dst, X86XmmReg src, uint8_t imm);
void x86_movntps_m(SecBuf *s, X86Mem dstm, X86XmmReg src);
void x86_movntpd_m(SecBuf *s, X86Mem dstm, X86XmmReg src);
void x86_movntdq_m(SecBuf *s, X86Mem dstm, X86XmmReg src);
void x86_movntq_m(SecBuf *s, X86Mem dstm, X86XmmReg src);
void x86_movnti_m(SecBuf *s, X86Mem dstm, X86Reg src, int size);
void x86_lddqu_rm(SecBuf *s, X86Mem srcm, X86XmmReg dst);
void x86_ldmxcsr_m(SecBuf *s, X86Mem m);
void x86_stmxcsr_m(SecBuf *s, X86Mem m);
void x86_emms(SecBuf *s);
void x86_monitor(SecBuf *s);
void x86_mwait(SecBuf *s);
void x86_femms(SecBuf *s);
void x86_crc32qi(SecBuf *s, X86Reg dst, X86Reg src);
void x86_crc32si(SecBuf *s, X86Reg dst, X86Reg src, int size);

// x87 (legacy, for long double)
void x86_fldl_m(SecBuf *s, X86Mem srcm);
void x86_fstpt_m(SecBuf *s, X86Mem dstm);
void x86_fldt_m(SecBuf *s, X86Mem srcm);
void x86_fstpl_m(SecBuf *s, X86Mem dstm);


// ===== AVX/AVX2 (VEX.256) =====
void x86_vaddpd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vaddps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vaddsd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vaddss(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vaddsubpd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vaddsubps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vandnpd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vandnps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vandpd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vandps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vbroadcastsd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vbroadcastss(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vcmppd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm);
void x86_vcmpps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm);
void x86_vcmpsd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm);
void x86_vcmpss(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm);
void x86_vdivpd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vdivps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vdivsd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vdivss(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vextractf128(SecBuf *s, X86XmmReg d, X86XmmReg rm, uint8_t imm);
void x86_vextracti128(SecBuf *s, X86XmmReg d, X86XmmReg rm, uint8_t imm);
void x86_vhaddpd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vhaddps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vhsubpd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vhsubps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vinsertf128(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm);
void x86_vinserti128(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm);
void x86_vmaxpd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vmaxps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vmaxsd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vmaxss(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vminpd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vminps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vminsd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vminss(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vmovapd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vmovaps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vmovdqa(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vmovdqu(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vmovhlps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vmovlhps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vmulpd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vmulps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vmulsd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vmulss(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vorpd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vorps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpabsb(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpabsd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpabsw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpackssdw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpacksswb(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpackuswb(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpaddb(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpaddd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpaddq(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpaddsb(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpaddsw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpaddusb(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpaddusw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpaddw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpalignr(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm);
void x86_vpand(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpandn(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpavgb(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpavgw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpcmpeqb(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpcmpeqd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpcmpeqw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpcmpgtb(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpcmpgtd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpcmpgtw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vperm2f128(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm);
void x86_vperm2i128(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm);
void x86_vpermd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpermilpd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpermilps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpermpd(SecBuf *s, X86XmmReg d, X86XmmReg rm, uint8_t imm);
void x86_vpermps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpcmpeqq(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpbroadcastb(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpbroadcastw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpbroadcastd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpbroadcastq(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpermq(SecBuf *s, X86XmmReg d, X86XmmReg rm, uint8_t imm);
void x86_vphaddd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vphaddsw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vphaddw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vphsubd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vphsubsw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vphsubw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpmaddubsw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpmaddwd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpmaskmovd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpmaskmovq(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpmaxsw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpmaxub(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpminsw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpminub(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpmulhrsw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpmulhuw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpmulhw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpmullw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpmuludq(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpor(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpsadbw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpshufb(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpshufd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm);
void x86_vpshufhw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm);
void x86_vpshuflw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm);
void x86_vpsignb(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpsignd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpsignw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpslld(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpsllq(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpsllvd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpsllw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpsrad(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpsravd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpsraw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpsrld(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpsrlq(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpsrlvd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpsrlw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpsubb(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpsubd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpsubq(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpsubsb(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpsubsw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpsubusb(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpsubusw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpsubw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpunpckhbw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpunpckhdq(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpunpckhqdq(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpunpckhwd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpunpcklbw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpunpckldq(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpunpcklqdq(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpunpcklwd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpxor(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vrcpps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vrcpsd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vrcpss(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vrsqrtps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vrsqrtsd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vrsqrtss(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vshufpd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm);
void x86_vshufps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm);
void x86_vsqrtpd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vsqrtps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vsqrtsd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vsqrtss(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vsubpd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vsubps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vsubsd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vsubss(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vunpckhpd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vunpckhps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vunpcklpd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vunpcklps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vxorpd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vxorps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vmovups_rm256(SecBuf *s, X86XmmReg dst, X86Mem src);
void x86_vmovups_mr256(SecBuf *s, X86Mem dst, X86XmmReg src);
void x86_vmovdqa_rm256(SecBuf *s, X86XmmReg dst, X86Mem src);
void x86_vmovdqa_mr256(SecBuf *s, X86Mem dst, X86XmmReg src);
void x86_vmovdqu_rm256(SecBuf *s, X86XmmReg dst, X86Mem src);
void x86_vmovdqu_mr256(SecBuf *s, X86Mem dst, X86XmmReg src);

void x86_vbroadcastf128(SecBuf *s, X86XmmReg d, X86Mem m);
void x86_vlddqu256(SecBuf *s, X86XmmReg d, X86Mem m);
void x86_vmovntdqa256(SecBuf *s, X86XmmReg d, X86Mem m);
void x86_vpmaskmovd_mr(SecBuf *s, X86Mem m, X86XmmReg data, X86XmmReg mask);
void x86_vpmaskmovd_rm(SecBuf *s, X86XmmReg d, X86XmmReg mask, X86Mem m);
void x86_vpmaskmovq_mr(SecBuf *s, X86Mem m, X86XmmReg data, X86XmmReg mask);
void x86_vpmaskmovq_rm(SecBuf *s, X86XmmReg d, X86XmmReg mask, X86Mem m);
void x86_vblendpd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm);
void x86_vblendps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm);
void x86_vblendvpd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, X86XmmReg mask);
void x86_vblendvps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, X86XmmReg mask);
void x86_vcvtdq2pd(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vcvtdq2ps(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vcvtpd2dq(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vcvtph2ps(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vcvtph2ps256(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vcvtpd2ps(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vcvtps2dq(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vcvtps2ph(SecBuf *s, X86XmmReg d, X86XmmReg rm, uint8_t imm);
void x86_vcvtps2ph256(SecBuf *s, X86XmmReg d, X86XmmReg rm, uint8_t imm);
void x86_vcvtps2pd(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vcvttpd2dq(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vcvttps2dq(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vdpps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm);
void x86_vmovddup(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vmovmskpd(SecBuf *s, X86XmmReg dstGp, X86XmmReg src);
void x86_vmovmskps(SecBuf *s, X86XmmReg dstGp, X86XmmReg src);
void x86_vmovntdq_m256(SecBuf *s, X86Mem m, X86XmmReg sr);
void x86_vmovntpd_m256(SecBuf *s, X86Mem m, X86XmmReg sr);
void x86_vmovntps_m256(SecBuf *s, X86Mem m, X86XmmReg sr);
void x86_vmovshdup(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vmovsldup(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vmpsadbw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm);
void x86_vpblendd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm);
void x86_vpblendvb(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, X86XmmReg mask);
void x86_vpblendw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm);
void x86_vpcmpgtq(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpermilpd_i(SecBuf *s, X86XmmReg d, X86XmmReg rm, uint8_t imm);
void x86_vpermilps_i(SecBuf *s, X86XmmReg d, X86XmmReg rm, uint8_t imm);
void x86_vpmaxsb(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpmaxsd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpmaxud(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpmaxuw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpminsb(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpminsd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpminud(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpminuw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpmovmskb256(SecBuf *s, X86XmmReg dstGp, X86XmmReg src);
void x86_vpmovsxbd(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vpmovsxbq(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vpmovsxbw(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vpmovsxdq(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vpmovsxwd(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vpmovsxwq(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vpmovzxbd(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vpmovzxbq(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vpmovzxbw(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vpmovzxdq(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vpmovzxwd(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vpmovzxwq(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vpmuldq(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpmulld(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vpslld_i(SecBuf *s, X86XmmReg d, uint8_t imm);
void x86_vpslld_r(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg cnt);
void x86_vpslldq_i(SecBuf *s, X86XmmReg d, uint8_t imm);
void x86_vpsllq_i(SecBuf *s, X86XmmReg d, uint8_t imm);
void x86_vpsllq_r(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg cnt);
void x86_vpsllw_i(SecBuf *s, X86XmmReg d, uint8_t imm);
void x86_vpsllw_r(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg cnt);
void x86_vpsrad_i(SecBuf *s, X86XmmReg d, uint8_t imm);
void x86_vpsrad_r(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg cnt);
void x86_vpsraw_i(SecBuf *s, X86XmmReg d, uint8_t imm);
void x86_vpsraw_r(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg cnt);
void x86_vpsrld_i(SecBuf *s, X86XmmReg d, uint8_t imm);
void x86_vpsrld_r(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg cnt);
void x86_vpsrldq_i(SecBuf *s, X86XmmReg d, uint8_t imm);
void x86_vpsrlq_i(SecBuf *s, X86XmmReg d, uint8_t imm);
void x86_vpsrlq_r(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg cnt);
void x86_vpsrlw_i(SecBuf *s, X86XmmReg d, uint8_t imm);
void x86_vpsrlw_r(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg cnt);
void x86_vptest(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vroundpd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm);
void x86_vroundps(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm);
void x86_vtestpd(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vtestps(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vpackusdw(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm);
void x86_vextractf128_pd(SecBuf *s, X86XmmReg d, X86XmmReg rm, uint8_t imm);
void x86_vinsertf128_pd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm);
void x86_vpcmpud512(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm);
void x86_vpmovqd512(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vprord512_i(SecBuf *s, X86XmmReg d, uint8_t imm);
void x86_vpsrld512_i(SecBuf *s, X86XmmReg d, uint8_t imm);
void x86_vpsrlq512_i(SecBuf *s, X86XmmReg d, uint8_t imm);
void x86_vshufi32x4(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm);
void x86_vpternlogd512(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm);
void x86_vpaddd512(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, int k);
void x86_vpaddq512(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, int k);
void x86_vpandd512(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, int k);
void x86_vpandnd512(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, int k);
void x86_vpandnq512(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, int k);
void x86_vpandq512(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, int k);
void x86_vpord512(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, int k);
void x86_vporq512(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, int k);
void x86_vpsubd512(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, int k);
void x86_vpsubq512(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, int k);
void x86_vpunpckhdq512(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, int k);
void x86_vpunpckhqdq512(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, int k);
void x86_vpunpckldq512(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, int k);
void x86_vpunpcklqdq512(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, int k);
void x86_vpxord512(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, int k);
void x86_vpxorq512(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, int k);
void x86_vmovups_mr512(SecBuf *s, X86Mem m, X86XmmReg sr);
void x86_vmovups_rm512(SecBuf *s, X86XmmReg d, X86Mem m);
void x86_vpbroadcastd512(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, int k);
void x86_vpbroadcastq512(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, int k);
void x86_vpcmpeqd512(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, int k);
void x86_kmovw_r32_k1(SecBuf *s, X86Reg dstGp);
void x86_vextractf64x4(SecBuf *s, X86XmmReg d, X86XmmReg rm, uint8_t imm);
void x86_vmovdqu32_mr256(SecBuf *s, X86Mem m, X86XmmReg sr);
void x86_vpmovqd256(SecBuf *s, X86XmmReg d, X86XmmReg rm);
void x86_vprord128_i(SecBuf *s, X86XmmReg d, uint8_t imm);
void x86_vprord256_i(SecBuf *s, X86XmmReg d, uint8_t imm);
#endif // X86_ENC_H
