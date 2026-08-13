// SPDX-License-Identifier: LGPL-2.1-or-later
// x86-64 instruction encoder.
// Reference: Intel® 64 and IA-32 Architectures Software Developer's Manual.
#ifndef ARCH_ARM64
#include "x86_enc.h"
#include "obj.h"
#include <stdint.h>
#include <assert.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Encoding helpers
// ---------------------------------------------------------------------------

void emit1(SecBuf *s, uint8_t b) { secbuf_emit8(s, b); }
void emit2(SecBuf *s, uint8_t a, uint8_t b) {
    emit1(s, a);
    emit1(s, b);
}
void emit3(SecBuf *s, uint8_t a, uint8_t b, uint8_t c) {
    emit1(s, a);
    emit2(s, b, c);
}
static void emit_imm32(SecBuf *s, int32_t v) { secbuf_emit32le(s, (uint32_t)v); }
static void emit_imm64(SecBuf *s, uint64_t v) { secbuf_emit64le(s, v); }

// REX prefix: W=1→64-bit, R=extra bit for reg, X=extra bit for index, B=extra bit for rm/base
static uint8_t rex(int W, int R, int X, int B) {
    return (uint8_t)(0x40 | (W ? 8 : 0) | (R ? 4 : 0) | (X ? 2 : 0) | (B ? 1 : 0));
}
#define REX_W  0x48
#define REX_WR 0x4c
#define REX_WB 0x49
#define REX_WRB 0x4d

// ModRM byte
static uint8_t modrm(int mod, X86Reg reg, X86Reg rm) {
    return (uint8_t)((mod << 6) | ((reg & 7) << 3) | (rm & 7));
}
uint8_t modrxmm(int mod, X86XmmReg reg, X86XmmReg rm) {
    return (uint8_t)((mod << 6) | ((reg & 7) << 3) | (rm & 7));
}

// SIB byte
static uint8_t sib(int scale, int index, int base) {
    int ss = (scale == 8) ? 3 : (scale == 4) ? 2
        : (scale == 2)                       ? 1
                                             : 0;
    return (uint8_t)((ss << 6) | ((index & 7) << 3) | (base & 7));
}

// Emit REX if any register is >= 4 (RSP) — needed for both the 8-bit
// register remap (SPL/BPL/SIL/DIL vs. AH/CH/DH/BH) and for extended
// registers R8-R15.
void maybe_rex(SecBuf *s, int W, int R, int X, int B) {
    if (W || R >= X86_RSP || X >= X86_RSP || B >= X86_RSP)
        emit1(s, rex(W, R > X86_RDI, X > X86_RDI, B > X86_RDI));
}

// Emit displacement
static void emit_disp(SecBuf *s, int64_t disp, int mod) {
    if (mod == 1)
        emit1(s, (uint8_t)(int8_t)disp);
    else if (mod == 2)
        emit_imm32(s, (int32_t)disp);
}

// Choose mod based on displacement value
static int disp_mod(int64_t d, X86Reg base) {
    if (base == X86_NOREG) return 0; // absolute
    if (d == 0 && (base & 7) != X86_RBP) return 0;
    if (d >= -128 && d <= 127) return 1;
    return 2;
}

// Emit a memory operand [base + index*scale + disp] after opcode byte
// reg_field is the /r field of ModRM
static void emit_mem(SecBuf *s, X86Reg base, X86Reg idx, int scale, int64_t disp, X86Reg reg_f) {
    if (base == X86_RIP) {
        // RIP-relative: use ModRM with mod=00, rm=101 (disp32)
        emit1(s, modrm(0, reg_f, X86_RBP));
        emit_imm32(s, (int32_t)disp);
        return;
    }
    if (base == X86_NOREG) {
        // Absolute: use ModRM with mod=00, rm=101 (disp32)
        emit1(s, modrm(0, reg_f, X86_RBP));
        emit_imm32(s, (int32_t)disp);
        return;
    }
    int mod = disp_mod(disp, base);
    bool need_sib = idx != X86_NOREG || (base & 7) == 4; // RSP/R12 base needs SIB
    if (need_sib) {
        emit1(s, modrm(mod, reg_f, 4)); // rm=4 signals SIB
        int si = (idx == X86_NOREG) ? 4 : (int)idx; // index=4 = no index in SIB
        emit1(s, sib(scale, si, base));
    } else {
        emit1(s, modrm(mod, reg_f, base));
        // Special case: mod=0 with rm=5 (RBP/R13) must use disp8 even for 0
        if (mod == 0 && (base & 7) == X86_RBP) {
            // RBP base with 0 disp needs mod=1, disp8=0
            // Recalculate: force mod=1
            secbuf_patch32le(s, s->len - 1, modrm(1, reg_f, base));
            secbuf_emit8(s, 0);
            return;
        }
    }
    emit_disp(s, disp, mod);
}

// ---------------------------------------------------------------------------
// Size prefix helpers
// ---------------------------------------------------------------------------

// Emit operand-size prefix (0x66) for 16-bit ops
static void size16_pfx(SecBuf *s, int size) {
    if (size == 2) emit1(s, 0x66);
}

// REX prefix selection by operand size (W for 64-bit, none for 32-bit,
// special-case for 8/16-bit registers >= R8). The 16-bit 0x66 operand-size
// override is NOT emitted here — every caller already calls size16_pfx()
// immediately beforehand, so doing it here too would double it up.
static void rex_for_size(SecBuf *s, int size, X86Reg reg, X86Reg rm) {
    if (size == 8)
        maybe_rex(s, 1, reg, 0, rm);
    else if (size == 4)
        maybe_rex(s, 0, reg, 0, rm);
    else if (size == 2 || size == 1) {
        // 16-bit ops: REX only needed for R8+ registers. 8-bit ops
        // additionally need REX forced for RSP..RDI (spl/bpl/sil/dil) even
        // though their index isn't > RDI: without a REX prefix, that same
        // index instead selects one of the legacy high-byte registers
        // (ah/bh/ch/dh) — a completely different physical register that
        // just happens to share the same 3-bit ModRM encoding.
        bool need_rex = reg >= X86_R8 || rm >= X86_R8 ||
            (size == 1 && ((reg >= X86_RSP && reg <= X86_RDI) || (rm >= X86_RSP && rm <= X86_RDI)));
        if (need_rex)
            maybe_rex(s, 0, reg, 0, rm);
    }
}

// Adjust opcode for operand size (most opcodes: 8-bit=op, rest=op+1)
static uint8_t opsize(uint8_t base_op8, int size) {
    return size == 1 ? base_op8 : base_op8 + 1;
}

// ---------------------------------------------------------------------------
// MOV
// ---------------------------------------------------------------------------
void x86_mov_rr(SecBuf *s, int size, X86Reg dst, X86Reg src) {
    size16_pfx(s, size);
    if (size == 8)
        emit1(s, rex(1, src > X86_RDI, 0, dst > X86_RDI));
    else if (size == 4 || size == 1)
        maybe_rex(s, 0, src, 0, dst);
    emit1(s, opsize(0x88, size));
    emit1(s, modrm(3, src, dst));
}

void x86_mov_ri(SecBuf *s, int size, X86Reg dst, int64_t imm) {
    size16_pfx(s, size);
    if (size == 8 && imm >= -0x80000000LL && imm <= 0x7fffffffLL) {
        // Use MOV r/m64, imm32 (sign-extended)
        emit1(s, rex(1, 0, 0, dst > X86_RDI));
        emit2(s, 0xc7, modrm(3, 0, (int)dst));
        emit_imm32(s, (int32_t)imm);
        return;
    }
    if (size == 8) {
        emit1(s, rex(1, 0, 0, dst > X86_RDI));
    } else if (dst > X86_RDI)
        emit1(s, rex(0, 0, 0, 1));
    emit1(s, (size == 1 ? 0xb0 : 0xb8) + (dst & 7));
    if (size == 1) emit1(s, (uint8_t)imm);
    else if (size == 2)
        secbuf_emit16le(s, (uint16_t)imm);
    else if (size == 4)
        emit_imm32(s, (int32_t)imm);
    else
        emit_imm64(s, (uint64_t)imm);
}

void x86_movabs(SecBuf *s, X86Reg dst, uint64_t imm64) {
    emit1(s, rex(1, 0, 0, dst > X86_RDI));
    emit1(s, 0xb8 + (dst & 7));
    emit_imm64(s, imm64);
}

void x86_mov_rm(SecBuf *s, int size, X86Reg dst, X86Mem src) {
    size16_pfx(s, size);
    int needrex = (size == 8) || dst > X86_RDI || src.base > X86_RDI || (src.index != X86_NOREG && src.index > X86_RDI);
    if (needrex) emit1(s, rex(size == 8, dst > X86_RDI, src.index > X86_RDI, src.base > X86_RDI));
    emit1(s, opsize(0x8a, size));
    emit_mem(s, src.base, src.index, src.scale, src.disp, dst);
}

void x86_mov_mr(SecBuf *s, int size, X86Mem dst, X86Reg src) {
    size16_pfx(s, size);
    int needrex = (size == 8) || src > X86_RDI || dst.base > X86_RDI || (dst.index != X86_NOREG && dst.index > X86_RDI);
    if (needrex) emit1(s, rex(size == 8, src > X86_RDI, dst.index > X86_RDI, dst.base > X86_RDI));
    emit1(s, opsize(0x88, size));
    emit_mem(s, dst.base, dst.index, dst.scale, dst.disp, src);
}

void x86_mov_mi(SecBuf *s, int size, X86Mem dst, int32_t imm) {
    size16_pfx(s, size);
    int needrex = (size == 8) || dst.base > X86_RDI || (dst.index != X86_NOREG && dst.index > X86_RDI);
    if (needrex) emit1(s, rex(size == 8, 0, dst.index > X86_RDI, dst.base > X86_RDI));
    emit1(s, opsize(0xc6, size));
    emit_mem(s, dst.base, dst.index, dst.scale, dst.disp, 0);
    if (size == 1) emit1(s, (uint8_t)imm);
    else if (size == 2)
        secbuf_emit16le(s, (uint16_t)imm);
    else
        emit_imm32(s, imm);
}

void x86_add_mi(SecBuf *s, int size, X86Mem dst, int32_t imm) {
    size16_pfx(s, size);
    int needrex = (size == 8) || dst.base > X86_RDI || (dst.index != X86_NOREG && dst.index > X86_RDI);
    if (needrex) emit1(s, rex(size == 8, 0, dst.index > X86_RDI, dst.base > X86_RDI));
    emit1(s, opsize(0x80, size)); // 0x80=r/m8 imm8; 0x81=r/m imm32; 0x83=r/m imm8-sign
    emit_mem(s, dst.base, dst.index, dst.scale, dst.disp, 0); // /0 = ADD
    if (size == 1) emit1(s, (uint8_t)imm);
    else if (size == 2)
        secbuf_emit16le(s, (uint16_t)imm);
    else
        emit_imm32(s, imm);
}
void x86_sub_mi(SecBuf *s, int size, X86Mem dst, int32_t imm) {
    size16_pfx(s, size);
    int needrex = (size == 8) || dst.base > X86_RDI || (dst.index != X86_NOREG && dst.index > X86_RDI);
    if (needrex) emit1(s, rex(size == 8, 0, dst.index > X86_RDI, dst.base > X86_RDI));
    emit1(s, opsize(0x80, size));
    emit_mem(s, dst.base, dst.index, dst.scale, dst.disp, 5); // /5 = SUB
    if (size == 1) emit1(s, (uint8_t)imm);
    else if (size == 2)
        secbuf_emit16le(s, (uint16_t)imm);
    else
        emit_imm32(s, imm);
}
void x86_or_mi(SecBuf *s, int size, X86Mem dst, int32_t imm) {
    int needrex = (size == 8) || dst.base > X86_RDI || (dst.index != X86_NOREG && dst.index > X86_RDI);
    if (needrex) emit1(s, rex(size == 8, 0, dst.index > X86_RDI, dst.base > X86_RDI));
    emit1(s, opsize(0x80, size));
    emit_mem(s, dst.base, dst.index, dst.scale, dst.disp, 1);
    if (size == 1) emit1(s, (uint8_t)imm);
    else if (size == 2)
        secbuf_emit16le(s, (uint16_t)imm);
    else
        emit_imm32(s, imm);
}
void x86_cmp_mi(SecBuf *s, int size, X86Mem dst, int32_t imm) {
    int needrex = (size == 8) || dst.base > X86_RDI || (dst.index != X86_NOREG && dst.index > X86_RDI);
    if (needrex) emit1(s, rex(size == 8, 0, dst.index > X86_RDI, dst.base > X86_RDI));
    emit1(s, opsize(0x80, size));
    emit_mem(s, dst.base, dst.index, dst.scale, dst.disp, 7);
    if (size == 1) emit1(s, (uint8_t)imm);
    else if (size == 2)
        secbuf_emit16le(s, (uint16_t)imm);
    else
        emit_imm32(s, imm);
}
void x86_and_mi(SecBuf *s, int size, X86Mem dst, int32_t imm) {
    int needrex = (size == 8) || dst.base > X86_RDI || (dst.index != X86_NOREG && dst.index > X86_RDI);
    if (needrex) emit1(s, rex(size == 8, 0, dst.index > X86_RDI, dst.base > X86_RDI));
    emit1(s, opsize(0x80, size));
    emit_mem(s, dst.base, dst.index, dst.scale, dst.disp, 4); // /4 = AND
    if (size == 1) emit1(s, (uint8_t)imm);
    else if (size == 2)
        secbuf_emit16le(s, (uint16_t)imm);
    else
        emit_imm32(s, imm);
}
void x86_xor_mi(SecBuf *s, int size, X86Mem dst, int32_t imm) {
    int needrex = (size == 8) || dst.base > X86_RDI || (dst.index != X86_NOREG && dst.index > X86_RDI);
    if (needrex) emit1(s, rex(size == 8, 0, dst.index > X86_RDI, dst.base > X86_RDI));
    emit1(s, opsize(0x80, size));
    emit_mem(s, dst.base, dst.index, dst.scale, dst.disp, 6); // /6 = XOR
    if (size == 1) emit1(s, (uint8_t)imm);
    else if (size == 2)
        secbuf_emit16le(s, (uint16_t)imm);
    else
        emit_imm32(s, imm);
}
void x86_movsx(SecBuf *s, int dst_sz, int src_sz, X86Reg dst, X86Reg src) {
    if (dst_sz == 4 && src_sz == 1) {
        maybe_rex(s, 0, dst, 0, src);
        emit3(s, 0x0f, 0xbe, modrm(3, dst, src));
    } else if (dst_sz == 4 && src_sz == 2) {
        maybe_rex(s, 0, dst, 0, src);
        emit3(s, 0x0f, 0xbf, modrm(3, dst, src));
    } else if (dst_sz == 8 && src_sz == 4) {
        emit1(s, rex(1, dst > X86_RDI, 0, src > X86_RDI));
        emit2(s, 0x63, modrm(3, dst, src));
    } else if (dst_sz == 8 && src_sz == 1) {
        emit1(s, rex(1, dst > X86_RDI, 0, src > X86_RDI));
        emit3(s, 0x0f, 0xbe, modrm(3, dst, src));
    } else if (dst_sz == 8 && src_sz == 2) {
        emit1(s, rex(1, dst > X86_RDI, 0, src > X86_RDI));
        emit3(s, 0x0f, 0xbf, modrm(3, dst, src));
    } else if (dst_sz == 2 && src_sz == 1) {
        emit1(s, 0x66);
        maybe_rex(s, 0, dst, 0, src);
        emit3(s, 0x0f, 0xbe, modrm(3, dst, src));
    }
}

void x86_movzx(SecBuf *s, int dst_sz, int src_sz, X86Reg dst, X86Reg src) {
    if (dst_sz == 4 && src_sz == 1) {
        maybe_rex(s, 0, dst, 0, src);
        emit3(s, 0x0f, 0xb6, modrm(3, dst, src));
    } else if (dst_sz == 4 && src_sz == 2) {
        maybe_rex(s, 0, dst, 0, src);
        emit3(s, 0x0f, 0xb7, modrm(3, dst, src));
    } else if (dst_sz == 8 && src_sz == 1) {
        emit1(s, rex(1, dst > X86_RDI, 0, src > X86_RDI));
        emit3(s, 0x0f, 0xb6, modrm(3, dst, src));
    } else if (dst_sz == 8 && src_sz == 2) {
        emit1(s, rex(1, dst > X86_RDI, 0, src > X86_RDI));
        emit3(s, 0x0f, 0xb7, modrm(3, dst, src));
    } else if (dst_sz == 8 && src_sz == 4) {
        // 32-bit move zero-extends to 64-bit automatically
        maybe_rex(s, 0, dst, 0, src);
        emit2(s, 0x89, modrm(3, src, dst)); // MOV r32, r32
    }
}

void x86_movsx_rm(SecBuf *s, int dst_sz, int src_sz, X86Reg dst, X86Mem src) {
    if (dst_sz == 8 && src_sz == 4) {
        emit1(s, rex(1, dst > X86_RDI, src.index > X86_RDI, src.base > X86_RDI));
        emit1(s, 0x63);
        emit_mem(s, src.base, src.index, src.scale, src.disp, dst);
    } else {
        uint8_t opcode = (src_sz == 1) ? 0xbe : 0xbf;
        int w = (dst_sz == 8) ? 1 : 0;
        maybe_rex(s, w, dst, src.index != X86_NOREG ? src.index : 0, src.base);
        emit2(s, 0x0f, opcode);
        emit_mem(s, src.base, src.index, src.scale, src.disp, dst);
    }
}

void x86_movzx_rm(SecBuf *s, int dst_sz, int src_sz, X86Reg dst, X86Mem src) {
    uint8_t opcode = (src_sz == 1) ? 0xb6 : 0xb7;
    int w = (dst_sz == 8) ? 1 : 0;
    maybe_rex(s, w, dst, src.index != X86_NOREG ? src.index : 0, src.base);
    emit2(s, 0x0f, opcode);
    emit_mem(s, src.base, src.index, src.scale, src.disp, dst);
}

void x86_lea(SecBuf *s, int size, X86Reg dst, X86Mem src) {
    if (size == 2) emit1(s, 0x66);
    emit1(s, rex(size == 8, dst > X86_RDI, src.index > X86_RDI, src.base > X86_RDI));
    emit1(s, 0x8d);
    emit_mem(s, src.base, src.index, src.scale, src.disp, dst);
}

// ---------------------------------------------------------------------------
// Generic ALU: op_rr, op_ri, op_rm
// (base_op: 0=ADD,1=OR,2=ADC,3=SBB,4=AND,5=SUB,6=XOR,7=CMP)
// ---------------------------------------------------------------------------
static void alu_rr(SecBuf *s, int size, int op, X86Reg dst, X86Reg src) {
    size16_pfx(s, size);
    rex_for_size(s, size, src, dst);
    emit1(s, (uint8_t)((op * 8) | (size == 1 ? 0 : 1)));
    emit1(s, modrm(3, src, dst));
}

static void alu_ri(SecBuf *s, int size, int op, X86Reg dst, int32_t imm) {
    size16_pfx(s, size);
    rex_for_size(s, size, X86_NOREG, dst);
    if (size != 1 && imm >= -128 && imm <= 127) {
        emit1(s, 0x83);
        emit1(s, modrm(3, op, dst));
        emit1(s, (uint8_t)(int8_t)imm);
    } else if (size == 1) {
        emit1(s, 0x80);
        emit1(s, modrm(3, op, dst));
        emit1(s, (uint8_t)imm);
    } else if (dst == X86_RAX) {
        emit1(s, (uint8_t)((op * 8) | (size == 1 ? 4 : 5)));
        // AX's short form (0x3D + imm16) still only takes a 16-bit
        // immediate even though the general-register path below sign-
        // extends to imm32 — an operand like "cmpw $0xff80, %ax" (imm
        // parsed as the unsigned 65408, not sign-extended to -128) would
        // otherwise emit a stray extra two bytes into the instruction
        // stream, corrupting everything that follows it.
        if (size == 2) secbuf_emit16le(s, (uint16_t)imm);
        else
            emit_imm32(s, imm);
    } else {
        emit1(s, 0x81);
        emit1(s, modrm(3, op, dst));
        if (size == 2) secbuf_emit16le(s, (uint16_t)imm);
        else
            emit_imm32(s, imm);
    }
}

static void alu_rm(SecBuf *s, int size, int op, X86Reg dst, X86Mem src) {
    size16_pfx(s, size);
    int w = size == 8;
    emit1(s, rex(w, dst > X86_RDI, src.index > X86_RDI, src.base > X86_RDI));
    emit1(s, (uint8_t)((op * 8) | (size == 1 ? 2 : 3)));
    emit_mem(s, src.base, src.index, src.scale, src.disp, dst);
}

// Register-to-memory store form (r/m, r): "addl %eax, mem" and friends —
// the mirror image of alu_rm (mem, r/reg loaded FROM memory). Without this,
// any ALU op whose destination is a memory operand and source a register
// silently encoded nothing (see ALU_OP's dispatch in asm.c).
static void alu_mr(SecBuf *s, int size, int op, X86Mem dst, X86Reg src) {
    size16_pfx(s, size);
    int w = size == 8;
    emit1(s, rex(w, src > X86_RDI, dst.index > X86_RDI, dst.base > X86_RDI));
    emit1(s, (uint8_t)((op * 8) | (size == 1 ? 0 : 1)));
    emit_mem(s, dst.base, dst.index, dst.scale, dst.disp, src);
}

void x86_add_rr(SecBuf *s, int sz, X86Reg d, X86Reg sr) { alu_rr(s, sz, 0, d, sr); }
void x86_add_ri(SecBuf *s, int sz, X86Reg d, int32_t i) { alu_ri(s, sz, 0, d, i); }
void x86_add_rm(SecBuf *s, int sz, X86Reg d, X86Mem m) { alu_rm(s, sz, 0, d, m); }
void x86_add_mr(SecBuf *s, int sz, X86Mem d, X86Reg sr) { alu_mr(s, sz, 0, d, sr); }
void x86_sub_rr(SecBuf *s, int sz, X86Reg d, X86Reg sr) { alu_rr(s, sz, 5, d, sr); }
void x86_sub_ri(SecBuf *s, int sz, X86Reg d, int32_t i) { alu_ri(s, sz, 5, d, i); }
void x86_sub_rm(SecBuf *s, int sz, X86Reg d, X86Mem m) { alu_rm(s, sz, 5, d, m); }
void x86_sub_mr(SecBuf *s, int sz, X86Mem d, X86Reg sr) { alu_mr(s, sz, 5, d, sr); }
void x86_and_rr(SecBuf *s, int sz, X86Reg d, X86Reg sr) { alu_rr(s, sz, 4, d, sr); }
void x86_and_ri(SecBuf *s, int sz, X86Reg d, int32_t i) { alu_ri(s, sz, 4, d, i); }
void x86_and_rm(SecBuf *s, int sz, X86Reg d, X86Mem m) { alu_rm(s, sz, 4, d, m); }
void x86_and_mr(SecBuf *s, int sz, X86Mem d, X86Reg sr) { alu_mr(s, sz, 4, d, sr); }
void x86_or_rr(SecBuf *s, int sz, X86Reg d, X86Reg sr) { alu_rr(s, sz, 1, d, sr); }
void x86_or_ri(SecBuf *s, int sz, X86Reg d, int32_t i) { alu_ri(s, sz, 1, d, i); }
void x86_or_rm(SecBuf *s, int sz, X86Reg d, X86Mem m) { alu_rm(s, sz, 1, d, m); }
void x86_or_mr(SecBuf *s, int sz, X86Mem d, X86Reg sr) { alu_mr(s, sz, 1, d, sr); }
void x86_xor_rr(SecBuf *s, int sz, X86Reg d, X86Reg sr) { alu_rr(s, sz, 6, d, sr); }
void x86_xor_ri(SecBuf *s, int sz, X86Reg d, int32_t i) { alu_ri(s, sz, 6, d, i); }
void x86_xor_rm(SecBuf *s, int sz, X86Reg d, X86Mem m) { alu_rm(s, sz, 6, d, m); }
void x86_xor_mr(SecBuf *s, int sz, X86Mem d, X86Reg sr) { alu_mr(s, sz, 6, d, sr); }
void x86_cmp_rr(SecBuf *s, int sz, X86Reg a, X86Reg b) { alu_rr(s, sz, 7, a, b); }
void x86_cmp_ri(SecBuf *s, int sz, X86Reg a, int32_t i) { alu_ri(s, sz, 7, a, i); }
void x86_cmp_rm(SecBuf *s, int sz, X86Reg a, X86Mem b) { alu_rm(s, sz, 7, a, b); }
void x86_cmp_mr(SecBuf *s, int sz, X86Mem a, X86Reg b) {
    size16_pfx(s, sz);
    emit1(s, rex(sz == 8, b > X86_RDI, a.index > X86_RDI, a.base > X86_RDI));
    emit1(s, opsize(0x38, sz));
    emit_mem(s, a.base, a.index, a.scale, a.disp, b);
}
void x86_test_rr(SecBuf *s, int sz, X86Reg a, X86Reg b) {
    size16_pfx(s, sz);
    rex_for_size(s, sz, b, a);
    emit2(s, opsize(0x84, sz), modrm(3, b, a));
}
void x86_test_ri(SecBuf *s, int sz, X86Reg a, int32_t imm) {
    size16_pfx(s, sz);
    rex_for_size(s, sz, X86_NOREG, a);
    if (a == X86_RAX) emit1(s, opsize(0xa8, sz));
    else {
        emit1(s, opsize(0xf6, sz));
        emit1(s, modrm(3, 0, a));
    }
    if (sz == 1) emit1(s, (uint8_t)imm);
    else if (sz == 2)
        secbuf_emit16le(s, (uint16_t)imm);
    else
        emit_imm32(s, imm);
}

// Multiply
void x86_imul_rr(SecBuf *s, int sz, X86Reg dst, X86Reg src) {
    size16_pfx(s, sz);
    emit1(s, rex(sz == 8, dst > X86_RDI, 0, src > X86_RDI));
    emit3(s, 0x0f, 0xaf, modrm(3, dst, src));
}
void x86_imul_rri(SecBuf *s, int sz, X86Reg dst, X86Reg src, int32_t imm) {
    size16_pfx(s, sz);
    emit1(s, rex(sz == 8, dst > X86_RDI, 0, src > X86_RDI));
    if (imm >= -128 && imm <= 127) {
        emit2(s, 0x6b, modrm(3, dst, src));
        emit1(s, (uint8_t)(int8_t)imm);
    } else {
        emit2(s, 0x69, modrm(3, dst, src));
        if (sz == 2) secbuf_emit16le(s, (uint16_t)imm);
        else
            emit_imm32(s, imm);
    }
}
void x86_imul_r(SecBuf *s, int sz, X86Reg src) {
    size16_pfx(s, sz);
    rex_for_size(s, sz, X86_NOREG, src);
    emit2(s, opsize(0xf6, sz), modrm(3, 5, src));
}
void x86_idiv_r(SecBuf *s, int sz, X86Reg src) {
    size16_pfx(s, sz);
    rex_for_size(s, sz, X86_NOREG, src);
    emit2(s, opsize(0xf6, sz), modrm(3, 7, src));
}
void x86_div_r(SecBuf *s, int sz, X86Reg src) {
    size16_pfx(s, sz);
    rex_for_size(s, sz, X86_NOREG, src);
    emit2(s, opsize(0xf6, sz), modrm(3, 6, src));
}
void x86_neg_r(SecBuf *s, int sz, X86Reg r) {
    size16_pfx(s, sz);
    rex_for_size(s, sz, X86_NOREG, r);
    emit2(s, opsize(0xf6, sz), modrm(3, 3, r));
}
void x86_not_r(SecBuf *s, int sz, X86Reg r) {
    size16_pfx(s, sz);
    rex_for_size(s, sz, X86_NOREG, r);
    emit2(s, opsize(0xf6, sz), modrm(3, 2, r));
}
void x86_inc_r(SecBuf *s, int sz, X86Reg r) {
    size16_pfx(s, sz);
    rex_for_size(s, sz, X86_NOREG, r);
    emit2(s, opsize(0xfe, sz), modrm(3, 0, r));
}
void x86_dec_r(SecBuf *s, int sz, X86Reg r) {
    size16_pfx(s, sz);
    rex_for_size(s, sz, X86_NOREG, r);
    emit2(s, opsize(0xfe, sz), modrm(3, 1, r));
}
void x86_inc_m(SecBuf *s, int sz, X86Mem m) {
    int needrex = (sz == 8) || m.base > X86_RDI || (m.index != X86_NOREG && m.index > X86_RDI);
    if (sz == 2) emit1(s, 0x66);
    if (needrex) emit1(s, rex(sz == 8, 0, m.index > X86_RDI, m.base > X86_RDI));
    emit1(s, opsize(0xfe, sz));
    emit_mem(s, m.base, m.index, m.scale, m.disp, 0);
}
void x86_dec_m(SecBuf *s, int sz, X86Mem m) {
    int needrex = (sz == 8) || m.base > X86_RDI || (m.index != X86_NOREG && m.index > X86_RDI);
    if (sz == 2) emit1(s, 0x66);
    if (needrex) emit1(s, rex(sz == 8, 0, m.index > X86_RDI, m.base > X86_RDI));
    emit1(s, opsize(0xfe, sz));
    emit_mem(s, m.base, m.index, m.scale, m.disp, 1);
}
void x86_neg_m(SecBuf *s, int sz, X86Mem m) {
    int needrex = (sz == 8) || m.base > X86_RDI || (m.index != X86_NOREG && m.index > X86_RDI);
    if (sz == 2) emit1(s, 0x66);
    if (needrex) emit1(s, rex(sz == 8, 0, m.index > X86_RDI, m.base > X86_RDI));
    emit1(s, opsize(0xf6, sz));
    emit_mem(s, m.base, m.index, m.scale, m.disp, 3);
}
void x86_not_m(SecBuf *s, int sz, X86Mem m) {
    int needrex = (sz == 8) || m.base > X86_RDI || (m.index != X86_NOREG && m.index > X86_RDI);
    if (sz == 2) emit1(s, 0x66);
    if (needrex) emit1(s, rex(sz == 8, 0, m.index > X86_RDI, m.base > X86_RDI));
    emit1(s, opsize(0xf6, sz));
    emit_mem(s, m.base, m.index, m.scale, m.disp, 2);
}
void x86_cdq(SecBuf *s) { emit1(s, 0x99); }
void x86_cqo(SecBuf *s) { emit2(s, 0x48, 0x99); }
void x86_cbw(SecBuf *s) { emit2(s, 0x66, 0x98); }
void x86_cwde(SecBuf *s) { emit1(s, 0x98); }
void x86_cdqe(SecBuf *s) { emit2(s, rex(1, 0, 0, 0), 0x98); }
void x86_cwd(SecBuf *s) { emit2(s, 0x66, 0x99); }

// Shifts
static void shift_ri(SecBuf *s, int sz, int op, X86Reg r, uint8_t imm) {
    size16_pfx(s, sz);
    rex_for_size(s, sz, X86_NOREG, r);
    if (imm == 1) {
        emit2(s, opsize(0xd0, sz), modrm(3, op, r));
    } else {
        emit2(s, opsize(0xc0, sz), modrm(3, op, r));
        emit1(s, imm);
    }
}
static void shift_rcl(SecBuf *s, int sz, int op, X86Reg r) {
    size16_pfx(s, sz);
    rex_for_size(s, sz, X86_NOREG, r);
    emit2(s, opsize(0xd2, sz), modrm(3, op, r));
}
void x86_shl_ri(SecBuf *s, int sz, X86Reg r, uint8_t i) { shift_ri(s, sz, 4, r, i); }
void x86_shr_ri(SecBuf *s, int sz, X86Reg r, uint8_t i) { shift_ri(s, sz, 5, r, i); }
void x86_sar_ri(SecBuf *s, int sz, X86Reg r, uint8_t i) { shift_ri(s, sz, 7, r, i); }
void x86_ror_ri(SecBuf *s, int sz, X86Reg r, uint8_t i) { shift_ri(s, sz, 1, r, i); }
void x86_rol_ri(SecBuf *s, int sz, X86Reg r, uint8_t i) { shift_ri(s, sz, 0, r, i); }
void x86_shl_rcl(SecBuf *s, int sz, X86Reg r) { shift_rcl(s, sz, 4, r); }
void x86_shr_rcl(SecBuf *s, int sz, X86Reg r) { shift_rcl(s, sz, 5, r); }
void x86_sar_rcl(SecBuf *s, int sz, X86Reg r) { shift_rcl(s, sz, 7, r); }
void x86_rcl_ri(SecBuf *s, int sz, X86Reg r, uint8_t i) { shift_ri(s, sz, 2, r, i); }
void x86_rcr_ri(SecBuf *s, int sz, X86Reg r, uint8_t i) { shift_ri(s, sz, 3, r, i); }
void x86_rcl_rcl(SecBuf *s, int sz, X86Reg r) { shift_rcl(s, sz, 2, r); }
void x86_rcr_rcl(SecBuf *s, int sz, X86Reg r) { shift_rcl(s, sz, 3, r); }
void x86_rol_rcl(SecBuf *s, int sz, X86Reg r) { shift_rcl(s, sz, 0, r); }
void x86_ror_rcl(SecBuf *s, int sz, X86Reg r) { shift_rcl(s, sz, 1, r); }

// SETcc
void x86_setcc(SecBuf *s, X86Cond cc, X86Reg dst) {
    if (dst >= 4) emit1(s, rex(0, 0, 0, dst > X86_RDI));
    emit3(s, 0x0f, (uint8_t)(0x90 | cc), modrm(3, 0, dst));
}

// CMOVcc
void x86_cmovcc(SecBuf *s, int sz, X86Cond cc, X86Reg dst, X86Reg src) {
    size16_pfx(s, sz);
    emit1(s, rex(sz == 8, dst > X86_RDI, 0, src > X86_RDI));
    emit3(s, 0x0f, (uint8_t)(0x40 | cc), modrm(3, dst, src));
}

// Bit ops
static void bop(SecBuf *s, int sz, uint8_t op2, X86Reg dst, X86Reg src) {
    size16_pfx(s, sz);
    emit1(s, rex(sz == 8, dst > X86_RDI, 0, src > X86_RDI));
    emit3(s, 0x0f, op2, modrm(3, dst, src));
}
void x86_bsf(SecBuf *s, int sz, X86Reg d, X86Reg sr) { bop(s, sz, 0xbc, d, sr); }
void x86_bsr(SecBuf *s, int sz, X86Reg d, X86Reg sr) { bop(s, sz, 0xbd, d, sr); }
void x86_popcnt(SecBuf *s, int sz, X86Reg d, X86Reg sr) {
    emit1(s, 0xf3);
    bop(s, sz, 0xb8, d, sr);
}
void x86_lzcnt(SecBuf *s, int sz, X86Reg d, X86Reg sr) {
    emit1(s, 0xf3);
    bop(s, sz, 0xbd, d, sr);
}
void x86_tzcnt(SecBuf *s, int sz, X86Reg d, X86Reg sr) {
    emit1(s, 0xf3);
    bop(s, sz, 0xbc, d, sr);
}
// 0F xx /r with dst=register, src=memory (BSF/BSR's r/m32/64 source
// form) -- the mirror image of bop_mr's dst=memory shape. Missing this
// meant a memory-operand "bsr (%mem), %reg" (or "bsf") had nowhere to
// go: encode_x86's dispatch called the register-register x86_bsr/x86_bsf
// unconditionally with whatever parse_x86_reg64() returned for a
// non-register operand string like "(%rbx)" -- garbage that, empirically,
// decoded to a fixed physical register rather than erroring, silently
// reading the wrong value. Found via GMP's own longlong.h count_leading_
// zeros/count_trailing_zeros macros, which pass their input operand as
// "rm" (register-or-memory, compiler's choice) -- rcc's own encode_x86
// always resolves "rm" to the memory alternative (see AsmOperand.is_memory
// in parser.c), so BSR/BSF's *only* reachable operand shape from that
// macro is the memory one.
static void bop_rm(SecBuf *s, int sz, uint8_t op2, X86Reg dst, X86Mem src) {
    size16_pfx(s, sz);
    bool needrex = (sz == 8) || dst > X86_RDI || src.base > X86_RDI ||
        (src.index != X86_NOREG && src.index > X86_RDI);
    if (needrex) emit1(s, rex(sz == 8, dst > X86_RDI, src.index > X86_RDI, src.base > X86_RDI));
    emit2(s, 0x0f, op2);
    emit_mem(s, src.base, src.index, src.scale, src.disp, dst);
}
void x86_bsf_rm(SecBuf *s, int sz, X86Reg d, X86Mem sm) { bop_rm(s, sz, 0xbc, d, sm); }
void x86_bsr_rm(SecBuf *s, int sz, X86Reg d, X86Mem sm) { bop_rm(s, sz, 0xbd, d, sm); }
void x86_bswap(SecBuf *s, int sz, X86Reg r) {
    if (sz == 8) emit1(s, rex(1, 0, 0, r > X86_RDI));
    else if (r > X86_RDI)
        emit1(s, rex(0, 0, 0, 1));
    emit2(s, 0x0f, (uint8_t)(0xc8 + (r & 7)));
}

// Two-byte-opcode r/m, r form (0F xx /r) with a memory destination:
// BT/BTS/BTR/BTC, XADD, CMPXCHG all share this shape.
static void bop_mr(SecBuf *s, int sz, uint8_t op2, X86Mem dst, X86Reg src) {
    size16_pfx(s, sz);
    bool needrex = (sz == 8) || src > X86_RDI || dst.base > X86_RDI ||
        (dst.index != X86_NOREG && dst.index > X86_RDI);
    if (needrex) emit1(s, rex(sz == 8, src > X86_RDI, dst.index > X86_RDI, dst.base > X86_RDI));
    emit2(s, 0x0f, op2);
    emit_mem(s, dst.base, dst.index, dst.scale, dst.disp, src);
}

void x86_bt_mr(SecBuf *s, int sz, X86Mem d, X86Reg sr) { bop_mr(s, sz, 0xa3, d, sr); }
void x86_bts_mr(SecBuf *s, int sz, X86Mem d, X86Reg sr) { bop_mr(s, sz, 0xab, d, sr); }
void x86_btr_mr(SecBuf *s, int sz, X86Mem d, X86Reg sr) { bop_mr(s, sz, 0xb3, d, sr); }
void x86_btc_mr(SecBuf *s, int sz, X86Mem d, X86Reg sr) { bop_mr(s, sz, 0xbb, d, sr); }
void x86_bt_rr(SecBuf *s, int sz, X86Reg d, X86Reg sr) { bop(s, sz, 0xa3, d, sr); }
void x86_bts_rr(SecBuf *s, int sz, X86Reg d, X86Reg sr) { bop(s, sz, 0xab, d, sr); }
void x86_btr_rr(SecBuf *s, int sz, X86Reg d, X86Reg sr) { bop(s, sz, 0xb3, d, sr); }
void x86_btc_rr(SecBuf *s, int sz, X86Reg d, X86Reg sr) { bop(s, sz, 0xbb, d, sr); }

// XADD r/m, r (0F C0/C1 /r): adds src into dst, dst's original value -> src.
void x86_xadd_mr(SecBuf *s, int sz, X86Mem d, X86Reg sr) {
    bop_mr(s, sz, opsize(0xc0, sz), d, sr);
}
void x86_xadd_rr(SecBuf *s, int sz, X86Reg d, X86Reg sr) {
    bop(s, sz, opsize(0xc0, sz), d, sr);
}

// CMPXCHG r/m, r (0F B0/B1 /r): compares r/m with AL/AX/EAX/RAX; if equal,
// ZF=1 and r/m = src, else ZF=0 and AL/AX/EAX/RAX = r/m.
void x86_cmpxchg_mr(SecBuf *s, int sz, X86Mem d, X86Reg sr) {
    bop_mr(s, sz, opsize(0xb0, sz), d, sr);
}
void x86_cmpxchg_rr(SecBuf *s, int sz, X86Reg d, X86Reg sr) {
    bop(s, sz, opsize(0xb0, sz), d, sr);
}

// Stack
void x86_push(SecBuf *s, X86Reg r) {
    if (r > X86_RDI) emit1(s, rex(0, 0, 0, 1));
    emit1(s, (uint8_t)(0x50 + (r & 7)));
}
// PUSH r/m64 (FF /6): "pushq DISP(%reg)" — e.g. the kernel's own IRET-frame
// copy loops ("pushq 5*8(%rsp)") and syscall-entry pt_regs pushes
// ("pushq 6*8(%rdi)" etc). No REX.W: like CALL/JMP's FF /2 and /4, PUSH's
// default operand size in 64-bit mode is already 64-bit.
void x86_push_m(SecBuf *s, X86Mem m) {
    bool needrex = m.base > X86_RDI || (m.index != X86_NOREG && m.index > X86_RDI);
    if (needrex) emit1(s, rex(0, 0, m.index > X86_RDI, m.base > X86_RDI));
    emit1(s, 0xff);
    emit_mem(s, m.base, m.index, m.scale, m.disp, 6);
}
void x86_pop(SecBuf *s, X86Reg r) {
    if (r > X86_RDI) emit1(s, rex(0, 0, 0, 1));
    emit1(s, (uint8_t)(0x58 + (r & 7)));
}
void x86_push_imm(SecBuf *s, int32_t imm) {
    if (imm >= -128 && imm <= 127) {
        emit2(s, 0x6a, (uint8_t)(int8_t)imm);
    } else {
        emit1(s, 0x68);
        emit_imm32(s, imm);
    }
}

// Control flow
void x86_call_rel32(SecBuf *s, int32_t rel32) {
    emit1(s, 0xe8);
    emit_imm32(s, rel32);
}
void x86_call_r(SecBuf *s, X86Reg r) {
    if (r > X86_RDI) emit1(s, rex(0, 0, 0, 1));
    emit2(s, 0xff, modrm(3, 2, r));
}
// CALL m64 (FF /2): indirect call through a memory operand — "call
// *mem", e.g. paravirt's "call *%[paravirt_opptr]" through a pv_ops
// function-pointer slot. No REX.W: the default operand size for FF /2
// in 64-bit mode is already 64-bit.
void x86_call_m(SecBuf *s, X86Mem m) {
    bool needrex = m.base > X86_RDI || (m.index != X86_NOREG && m.index > X86_RDI);
    if (needrex) emit1(s, rex(0, 0, m.index > X86_RDI, m.base > X86_RDI));
    emit1(s, 0xff);
    emit_mem(s, m.base, m.index, m.scale, m.disp, 2);
}
void x86_jmp_rel32(SecBuf *s, int32_t rel32) {
    emit1(s, 0xe9);
    emit_imm32(s, rel32);
}
void x86_jmp_rel8(SecBuf *s, int8_t rel8) { emit2(s, 0xeb, (uint8_t)rel8); }
void x86_jmp_r(SecBuf *s, X86Reg r) {
    if (r > X86_RDI) emit1(s, rex(0, 0, 0, 1));
    emit2(s, 0xff, modrm(3, 4, r));
}
// JMP m64 (FF /4): indirect jump through a memory operand — "jmp *mem".
void x86_jmp_m(SecBuf *s, X86Mem m) {
    bool needrex = m.base > X86_RDI || (m.index != X86_NOREG && m.index > X86_RDI);
    if (needrex) emit1(s, rex(0, 0, m.index > X86_RDI, m.base > X86_RDI));
    emit1(s, 0xff);
    emit_mem(s, m.base, m.index, m.scale, m.disp, 4);
}
void x86_jcc_rel32(SecBuf *s, X86Cond cc, int32_t rel32) {
    emit2(s, 0x0f, (uint8_t)(0x80 | cc));
    emit_imm32(s, rel32);
}
void x86_jcc_rel8(SecBuf *s, X86Cond cc, int8_t rel8) {
    emit2(s, (uint8_t)(0x70 | cc), (uint8_t)rel8);
}
void x86_ret(SecBuf *s) { emit1(s, 0xc3); }
void x86_leave(SecBuf *s) { emit1(s, 0xc9); }
void x86_nop(SecBuf *s) { emit1(s, 0x90); }
void x86_int(SecBuf *s, uint8_t imm8) {
    if (imm8 == 3)
        emit1(s, 0xCC);
    else {
        emit1(s, 0xCD);
        emit1(s, imm8);
    }
}

// Misc
void x86_xchg_rr(SecBuf *s, int sz, X86Reg a, X86Reg b) {
    size16_pfx(s, sz);
    emit1(s, rex(sz == 8, a > X86_RDI, 0, b > X86_RDI));
    emit2(s, opsize(0x86, sz), modrm(3, a, b));
}
void x86_lock_prefix(SecBuf *s) { emit1(s, 0xf0); }
void x86_rep_prefix(SecBuf *s) { emit1(s, 0xf3); }
void x86_repne_prefix(SecBuf *s) { emit1(s, 0xf2); }
void x86_seg_prefix(SecBuf *s, uint8_t byte) { emit1(s, byte); }
void x86_cld(SecBuf *s) { emit1(s, 0xfc); }
void x86_stosb(SecBuf *s) { emit1(s, 0xaa); }
void x86_movsb(SecBuf *s) { emit1(s, 0xa4); }
void x86_cmpsb(SecBuf *s) { emit1(s, 0xa6); }
void x86_scasb(SecBuf *s) { emit1(s, 0xae); }

// Size-aware string instructions (movsw/l/q, stosw/l/q, cmpsw/l/q,
// scasw/l/q): all implicit-operand (rsi/rdi/rcx), no ModRM. The kernel's
// rep-prefixed string ops (e.g. copy_user_generic's "rep movsb", generic
// memset/memcpy/memcmp fallbacks) need every size, not just byte.
static void strop_size_pfx(SecBuf *s, int size) {
    if (size == 2) emit1(s, 0x66);
    else if (size == 8)
        emit1(s, rex(1, 0, 0, 0));
}
void x86_movs(SecBuf *s, int size) {
    strop_size_pfx(s, size);
    emit1(s, size == 1 ? 0xa4 : 0xa5);
}
void x86_stos(SecBuf *s, int size) {
    strop_size_pfx(s, size);
    emit1(s, size == 1 ? 0xaa : 0xab);
}
void x86_cmps(SecBuf *s, int size) {
    strop_size_pfx(s, size);
    emit1(s, size == 1 ? 0xa6 : 0xa7);
}
void x86_scas(SecBuf *s, int size) {
    strop_size_pfx(s, size);
    emit1(s, size == 1 ? 0xae : 0xaf);
}
void x86_lods(SecBuf *s, int size) {
    strop_size_pfx(s, size);
    emit1(s, size == 1 ? 0xac : 0xad);
}
void x86_mfence(SecBuf *s) { emit3(s, 0x0f, 0xae, 0xf0); }
void x86_lfence(SecBuf *s) { emit3(s, 0x0f, 0xae, 0xe8); }
void x86_sfence(SecBuf *s) { emit3(s, 0x0f, 0xae, 0xf8); }
void x86_cpuid(SecBuf *s) { emit2(s, 0x0f, 0xa2); }
void x86_rdtsc(SecBuf *s) { emit2(s, 0x0f, 0x31); }
void x86_rdtscp(SecBuf *s) { emit3(s, 0x0f, 0x01, 0xf9); }
void x86_clac(SecBuf *s) { emit3(s, 0x0f, 0x01, 0xca); }
void x86_stac(SecBuf *s) { emit3(s, 0x0f, 0x01, 0xcb); }
void x86_iretq(SecBuf *s) { emit2(s, 0x48, 0xcf); }
void x86_lahf(SecBuf *s) { emit1(s, 0x9f); }
void x86_sahf(SecBuf *s) { emit1(s, 0x9e); }
void x86_clc(SecBuf *s) { emit1(s, 0xf8); }
void x86_stc(SecBuf *s) { emit1(s, 0xf9); }
void x86_std(SecBuf *s) { emit1(s, 0xfd); }
void x86_endbr32(SecBuf *s) {
    emit2(s, 0xf3, 0x0f);
    emit2(s, 0x1e, 0xfb);
}
void x86_endbr64(SecBuf *s) {
    emit2(s, 0xf3, 0x0f);
    emit2(s, 0x1e, 0xfa);
}
void x86_int3(SecBuf *s) { emit1(s, 0xcc); }
void x86_int1(SecBuf *s) { emit1(s, 0xf1); }
void x86_syscall(SecBuf *s) { emit2(s, 0x0f, 0x05); }
void x86_sysenter(SecBuf *s) { emit2(s, 0x0f, 0x34); }
void x86_sysexit(SecBuf *s) { emit2(s, 0x0f, 0x35); }
void x86_sysret(SecBuf *s) { emit2(s, 0x0f, 0x07); }
// SYSRETQ (REX.W + 0F 07): returns to 64-bit mode, vs. plain SYSRET/
// SYSRETL (0F 07 alone) returning to 32-bit compatibility mode.
void x86_sysretq(SecBuf *s) { emit3(s, 0x48, 0x0f, 0x07); }
// RDRAND/RDSEED r/m (0F C7 /6, /7): the register operand's REX.W bit
// selects 64-bit, and a 66 prefix (like other GPR ops) selects 16-bit.
void x86_rdrand(SecBuf *s, int sz, X86Reg r) {
    if (sz == 2) emit1(s, 0x66);
    maybe_rex(s, sz == 8, X86_NOREG, X86_NOREG, r);
    emit3(s, 0x0f, 0xc7, modrm(3, 6, r));
}
void x86_rdseed(SecBuf *s, int sz, X86Reg r) {
    if (sz == 2) emit1(s, 0x66);
    maybe_rex(s, sz == 8, X86_NOREG, X86_NOREG, r);
    emit3(s, 0x0f, 0xc7, modrm(3, 7, r));
}
// CRC32 r32/r64, r/m8/16/32/64 (F2 0F 38 F0/F1 /r): dst is always a 32- or
// 64-bit GPR (never itself 8/16-bit); src_size selects the opcode (F0 for
// an 8-bit source, F1 otherwise) and, for a 16-bit source, an extra 66
// prefix ahead of the mandatory F2.
void x86_crc32(SecBuf *s, int dst_sz, int src_sz, X86Reg dst, X86Reg src) {
    if (src_sz == 2) emit1(s, 0x66);
    emit1(s, 0xf2);
    maybe_rex(s, dst_sz == 8, dst, X86_NOREG, src);
    emit3(s, 0x0f, 0x38, src_sz == 1 ? 0xf0 : 0xf1);
    emit1(s, modrm(3, dst, src));
}

// INVPCID r64, m128 (64-bit mode): the register operand's width is fixed at
// 64 bits by the mode itself, so unlike most instructions REX.W is neither
// needed nor meaningful here.
void x86_invpcid(SecBuf *s, X86Reg type_reg, X86Mem desc) {
    emit1(s, 0x66);
    maybe_rex(s, 0, type_reg, desc.index, desc.base);
    emit3(s, 0x0f, 0x38, 0x82);
    emit_mem(s, desc.base, desc.index, desc.scale, desc.disp, type_reg);
}

// RD/WR{FS,GS}BASE r32/r64: mandatory F3 prefix, opcode extension in the
// ModRM reg field (mod=11, register-direct only — no memory form exists).
static void fsgsbase_op(SecBuf *s, int size, int digit, X86Reg r) {
    emit1(s, 0xf3);
    maybe_rex(s, size == 8, X86_NOREG, X86_NOREG, r);
    emit2(s, 0x0f, 0xae);
    emit1(s, modrm(3, digit, r));
}
void x86_rdfsbase(SecBuf *s, int size, X86Reg r) { fsgsbase_op(s, size, 0, r); }
void x86_rdgsbase(SecBuf *s, int size, X86Reg r) { fsgsbase_op(s, size, 1, r); }
void x86_wrfsbase(SecBuf *s, int size, X86Reg r) { fsgsbase_op(s, size, 2, r); }
void x86_wrgsbase(SecBuf *s, int size, X86Reg r) { fsgsbase_op(s, size, 3, r); }

// MUL r/m (F6/F7 group, /4): implicit RDX:RAX = RAX * r/m.
void x86_mul_r(SecBuf *s, int size, X86Reg src) {
    size16_pfx(s, size);
    rex_for_size(s, size, X86_NOREG, src);
    emit2(s, opsize(0xf6, size), modrm(3, 4, src));
}
void x86_mul_m(SecBuf *s, int size, X86Mem m) {
    int needrex = (size == 8) || m.base > X86_RDI || (m.index != X86_NOREG && m.index > X86_RDI);
    if (size == 2) emit1(s, 0x66);
    if (needrex) emit1(s, rex(size == 8, 0, m.index > X86_RDI, m.base > X86_RDI));
    emit1(s, opsize(0xf6, size));
    emit_mem(s, m.base, m.index, m.scale, m.disp, 4);
}

// ADC (op=2) / SBB (op=3): same group-1 ALU shape as ADD/SUB/AND/OR/XOR/CMP.
void x86_adc_rr(SecBuf *s, int sz, X86Reg d, X86Reg sr) { alu_rr(s, sz, 2, d, sr); }
void x86_adc_ri(SecBuf *s, int sz, X86Reg d, int32_t i) { alu_ri(s, sz, 2, d, i); }
void x86_adc_rm(SecBuf *s, int sz, X86Reg d, X86Mem m) { alu_rm(s, sz, 2, d, m); }
void x86_adc_mr(SecBuf *s, int sz, X86Mem d, X86Reg sr) { alu_mr(s, sz, 2, d, sr); }
void x86_adc_mi(SecBuf *s, int size, X86Mem dst, int32_t imm) {
    int needrex = (size == 8) || dst.base > X86_RDI || (dst.index != X86_NOREG && dst.index > X86_RDI);
    if (size == 2) emit1(s, 0x66);
    if (needrex) emit1(s, rex(size == 8, 0, dst.index > X86_RDI, dst.base > X86_RDI));
    emit1(s, opsize(0x80, size));
    emit_mem(s, dst.base, dst.index, dst.scale, dst.disp, 2); // /2 = ADC
    if (size == 1) emit1(s, (uint8_t)imm);
    else if (size == 2)
        secbuf_emit16le(s, (uint16_t)imm);
    else
        emit_imm32(s, imm);
}
void x86_sbb_rr(SecBuf *s, int sz, X86Reg d, X86Reg sr) { alu_rr(s, sz, 3, d, sr); }
void x86_sbb_ri(SecBuf *s, int sz, X86Reg d, int32_t i) { alu_ri(s, sz, 3, d, i); }
void x86_sbb_rm(SecBuf *s, int sz, X86Reg d, X86Mem m) { alu_rm(s, sz, 3, d, m); }
void x86_sbb_mr(SecBuf *s, int sz, X86Mem d, X86Reg sr) { alu_mr(s, sz, 3, d, sr); }
void x86_sbb_mi(SecBuf *s, int size, X86Mem dst, int32_t imm) {
    int needrex = (size == 8) || dst.base > X86_RDI || (dst.index != X86_NOREG && dst.index > X86_RDI);
    if (size == 2) emit1(s, 0x66);
    if (needrex) emit1(s, rex(size == 8, 0, dst.index > X86_RDI, dst.base > X86_RDI));
    emit1(s, opsize(0x80, size));
    emit_mem(s, dst.base, dst.index, dst.scale, dst.disp, 3); // /3 = SBB
    if (size == 1) emit1(s, (uint8_t)imm);
    else if (size == 2)
        secbuf_emit16le(s, (uint16_t)imm);
    else
        emit_imm32(s, imm);
}

void x86_rdmsr(SecBuf *s) { emit2(s, 0x0f, 0x32); }
void x86_wrmsr(SecBuf *s) { emit2(s, 0x0f, 0x30); }

// CMPXCHG8B/16B m64/m128 (0F C7 /1): 16B variant just adds REX.W.
static void cmpxchgNb_m(SecBuf *s, bool w, X86Mem m) {
    bool needrex = w || m.base > X86_RDI || (m.index != X86_NOREG && m.index > X86_RDI);
    if (needrex) emit1(s, rex(w, 0, m.index > X86_RDI, m.base > X86_RDI));
    emit2(s, 0x0f, 0xc7);
    emit_mem(s, m.base, m.index, m.scale, m.disp, 1);
}
void x86_cmpxchg8b_m(SecBuf *s, X86Mem m) { cmpxchgNb_m(s, false, m); }
void x86_cmpxchg16b_m(SecBuf *s, X86Mem m) { cmpxchgNb_m(s, true, m); }

void x86_wbinvd(SecBuf *s) { emit2(s, 0x0f, 0x09); }
void x86_sti(SecBuf *s) { emit1(s, 0xfb); }
void x86_cli(SecBuf *s) { emit1(s, 0xfa); }
void x86_hlt(SecBuf *s) { emit1(s, 0xf4); }
// PUSHF/POPF have no 32-bit form in 64-bit mode; the "q" suffix is just
// GAS's explicit spelling of the same single-byte opcode as the bare form.
void x86_pushfq(SecBuf *s) { emit1(s, 0x9c); }
void x86_popfq(SecBuf *s) { emit1(s, 0x9d); }

void x86_outb_dx(SecBuf *s) { emit1(s, 0xee); }
void x86_outw_dx(SecBuf *s) { emit2(s, 0x66, 0xef); }
void x86_outl_dx(SecBuf *s) { emit1(s, 0xef); }
void x86_outb_imm(SecBuf *s, uint8_t imm8) { emit2(s, 0xe6, imm8); }
void x86_outw_imm(SecBuf *s, uint8_t imm8) { emit3(s, 0x66, 0xe7, imm8); }
void x86_outl_imm(SecBuf *s, uint8_t imm8) { emit2(s, 0xe7, imm8); }
void x86_inb_dx(SecBuf *s) { emit1(s, 0xec); }
void x86_inw_dx(SecBuf *s) { emit2(s, 0x66, 0xed); }
void x86_inl_dx(SecBuf *s) { emit1(s, 0xed); }
void x86_inb_imm(SecBuf *s, uint8_t imm8) { emit2(s, 0xe4, imm8); }
void x86_inw_imm(SecBuf *s, uint8_t imm8) { emit3(s, 0x66, 0xe5, imm8); }
void x86_inl_imm(SecBuf *s, uint8_t imm8) { emit2(s, 0xe5, imm8); }
void x86_insb(SecBuf *s) { emit1(s, 0x6c); }
void x86_insw(SecBuf *s) { emit2(s, 0x66, 0x6d); }
void x86_insl(SecBuf *s) { emit1(s, 0x6d); }
void x86_outsb(SecBuf *s) { emit1(s, 0x6e); }
void x86_outsw(SecBuf *s) { emit2(s, 0x66, 0x6f); }
void x86_outsl(SecBuf *s) { emit1(s, 0x6f); }
void x86_vmcall(SecBuf *s) { emit3(s, 0x0f, 0x01, 0xc1); }
void x86_vmmcall(SecBuf *s) { emit3(s, 0x0f, 0x01, 0xd9); }

// PREFETCHt0/NTA (0F 18 /1, /0) and PREFETCHW (0F 0D /1) — memory-only.
static void prefetch_m(SecBuf *s, uint8_t op2, int digit, X86Mem m) {
    bool needrex = m.base > X86_RDI || (m.index != X86_NOREG && m.index > X86_RDI);
    if (needrex) emit1(s, rex(0, 0, m.index > X86_RDI, m.base > X86_RDI));
    emit2(s, 0x0f, op2);
    emit_mem(s, m.base, m.index, m.scale, m.disp, digit);
}
void x86_prefetcht0(SecBuf *s, X86Mem m) { prefetch_m(s, 0x18, 1, m); }
void x86_prefetchnta(SecBuf *s, X86Mem m) { prefetch_m(s, 0x18, 0, m); }
void x86_prefetchw(SecBuf *s, X86Mem m) { prefetch_m(s, 0x0d, 1, m); }

// CLFLUSH (0F AE /7), CLFLUSHOPT (66 0F AE /7), CLWB (66 0F AE /6) — mem-only.
void x86_clflush(SecBuf *s, X86Mem m) { prefetch_m(s, 0xae, 7, m); }
void x86_clflushopt(SecBuf *s, X86Mem m) {
    emit1(s, 0x66);
    prefetch_m(s, 0xae, 7, m);
}
void x86_clwb(SecBuf *s, X86Mem m) {
    emit1(s, 0x66);
    prefetch_m(s, 0xae, 6, m);
}

// LGDT/LIDT/SGDT/SIDT (0F 01 /digit): load/store the (limit,base)
// pseudo-descriptor — always a memory operand, never a register.
void x86_lgdt(SecBuf *s, X86Mem m) { prefetch_m(s, 0x01, 2, m); }
void x86_lidt(SecBuf *s, X86Mem m) { prefetch_m(s, 0x01, 3, m); }
void x86_sgdt(SecBuf *s, X86Mem m) { prefetch_m(s, 0x01, 0, m); }
void x86_sidt(SecBuf *s, X86Mem m) { prefetch_m(s, 0x01, 1, m); }
// INVLPG (0F 01 /7): invalidate the TLB entry for a memory operand's page.
void x86_invlpg(SecBuf *s, X86Mem m) { prefetch_m(s, 0x01, 7, m); }

// LLDT/STR/LTR (0F 00 /digit): operate on a 16-bit selector, register or
// memory — always implicitly 16-bit, no operand-size prefix needed.
static void seldesc_r(SecBuf *s, int digit, X86Reg r) {
    if (r > X86_RDI) emit1(s, rex(0, 0, 0, 1));
    emit2(s, 0x0f, 0x00);
    emit1(s, modrm(3, digit, r));
}
void x86_lldt_r(SecBuf *s, X86Reg r) { seldesc_r(s, 2, r); }
void x86_lldt_m(SecBuf *s, X86Mem m) { prefetch_m(s, 0x00, 2, m); }
void x86_ltr_r(SecBuf *s, X86Reg r) { seldesc_r(s, 3, r); }
void x86_ltr_m(SecBuf *s, X86Mem m) { prefetch_m(s, 0x00, 3, m); }
void x86_str_r(SecBuf *s, X86Reg r) { seldesc_r(s, 1, r); }
void x86_str_m(SecBuf *s, X86Mem m) { prefetch_m(s, 0x00, 1, m); }

void x86_pause(SecBuf *s) { emit2(s, 0xf3, 0x90); }
void x86_swapgs(SecBuf *s) { emit3(s, 0x0f, 0x01, 0xf8); }
void x86_rdpmc(SecBuf *s) { emit2(s, 0x0f, 0x33); }
void x86_rdpkru(SecBuf *s) { emit3(s, 0x0f, 0x01, 0xee); }
void x86_wrpkru(SecBuf *s) { emit3(s, 0x0f, 0x01, 0xef); }

// VERW r/m16 (0F 00 /5) — memory form only needed so far.
void x86_verw_m(SecBuf *s, X86Mem m) { prefetch_m(s, 0x00, 5, m); }

// RDPID r32/r64 (F3 0F C7 /7): register-direct only, no memory form.
void x86_rdpid(SecBuf *s, X86Reg dst) {
    emit1(s, 0xf3);
    maybe_rex(s, 0, X86_NOREG, X86_NOREG, dst);
    emit2(s, 0x0f, 0xc7);
    emit1(s, modrm(3, 7, dst));
}

// LSL r32/r64, r/m16 (0F 03 /r): AT&T "lsl src, dst" — dst is the reg field.
void x86_lsl_rr(SecBuf *s, X86Reg src, X86Reg dst) {
    maybe_rex(s, 0, dst, X86_NOREG, src);
    emit2(s, 0x0f, 0x03);
    emit1(s, modrm(3, dst, src));
}
void x86_lsl_rm(SecBuf *s, X86Mem src, X86Reg dst) {
    bool needrex = dst > X86_RDI || src.base > X86_RDI || (src.index != X86_NOREG && src.index > X86_RDI);
    if (needrex) emit1(s, rex(0, dst > X86_RDI, src.index > X86_RDI, src.base > X86_RDI));
    emit2(s, 0x0f, 0x03);
    emit_mem(s, src.base, src.index, src.scale, src.disp, dst);
}

void x86_cmc(SecBuf *s) { emit1(s, 0xf5); }
void x86_clts(SecBuf *s) { emit2(s, 0x0f, 0x06); }
void x86_invd(SecBuf *s) { emit2(s, 0x0f, 0x08); }
void x86_wbnoinvd(SecBuf *s) { emit3(s, 0xf3, 0x0f, 0x09); }
void x86_wait(SecBuf *s) { emit1(s, 0x9b); }
void x86_xgetbv(SecBuf *s) { emit3(s, 0x0f, 0x01, 0xd0); }
void x86_xsetbv(SecBuf *s) { emit3(s, 0x0f, 0x01, 0xd1); }
void x86_serialize(SecBuf *s) { emit3(s, 0x0f, 0x01, 0xe8); }
// VERR r/m16 (0F 00 /4) — memory form only, mirrors the existing VERW_m.
void x86_verr_m(SecBuf *s, X86Mem m) { prefetch_m(s, 0x00, 4, m); }
// LAR r32/r64, r/m16 (0F 02 /r): AT&T "lar src, dst" — dst is the reg field.
void x86_lar_rr(SecBuf *s, X86Reg src, X86Reg dst) {
    maybe_rex(s, 0, dst, X86_NOREG, src);
    emit2(s, 0x0f, 0x02);
    emit1(s, modrm(3, dst, src));
}
void x86_lar_rm(SecBuf *s, X86Mem src, X86Reg dst) {
    bool needrex = dst > X86_RDI || src.base > X86_RDI || (src.index != X86_NOREG && src.index > X86_RDI);
    if (needrex) emit1(s, rex(0, dst > X86_RDI, src.index > X86_RDI, src.base > X86_RDI));
    emit2(s, 0x0f, 0x02);
    emit_mem(s, src.base, src.index, src.scale, src.disp, dst);
}
// SMSW/LMSW (0F 01 /4, /6) and SLDT (0F 00 /0): a 16-bit selector/status
// word, register or memory, same shape as LLDT/STR/LTR above.
void x86_smsw_r(SecBuf *s, X86Reg r) { seldesc_r(s, 4, r); }
void x86_smsw_m(SecBuf *s, X86Mem m) { prefetch_m(s, 0x01, 4, m); }
void x86_lmsw_r(SecBuf *s, X86Reg r) { seldesc_r(s, 6, r); }
void x86_lmsw_m(SecBuf *s, X86Mem m) { prefetch_m(s, 0x01, 6, m); }
void x86_sldt_r(SecBuf *s, X86Reg r) { seldesc_r(s, 0, r); }
void x86_sldt_m(SecBuf *s, X86Mem m) { prefetch_m(s, 0x00, 0, m); }
// UD0/UD1 r32, r/m32 (0F FF /r, 0F B9 /r): deliberate #UD-raising forms
// that (unlike UD2) still decode a ModRM byte, register-register only.
void x86_ud0(SecBuf *s, X86Reg dst, X86Reg src) {
    maybe_rex(s, 0, dst, X86_NOREG, src);
    emit2(s, 0x0f, 0xff);
    emit1(s, modrm(3, dst, src));
}
void x86_ud1(SecBuf *s, X86Reg dst, X86Reg src) {
    maybe_rex(s, 0, dst, X86_NOREG, src);
    emit2(s, 0x0f, 0xb9);
    emit1(s, modrm(3, dst, src));
}
// UD1 reg, r/m memory form. The kernel's __WARN_trap() (arch/x86/entry/
// entry.S) is the one real user: "ud1 (%edx), %_ASM_ARG1" deliberately
// encodes a 32-bit-addressed memory operand (67 prefix) so decode_bug()
// (arch/x86/kernel/traps.c) can tell this UD1 apart from every other UD1
// site in the tree by R/M shape alone, always with REX.W set (the
// register operand is the full 64-bit arg register) — exactly the fixed
// "67 48 0f b9 /r" shape, not a generally width-selectable encoding.
void x86_ud1_m(SecBuf *s, bool addr32, X86Reg reg, X86Mem m) {
    if (addr32) emit1(s, 0x67);
    maybe_rex(s, 1, reg, m.index, m.base);
    emit2(s, 0x0f, 0xb9);
    emit_mem(s, m.base, m.index, m.scale, m.disp, reg);
}

// FXSAVE/FXRSTOR/XSAVE-family: 0F AE or 0F C7 with a /digit sub-opcode and
// a single memory operand (never a register — mod==11 encodes a totally
// different, unrelated instruction on this same opcode byte, e.g. LFENCE/
// MFENCE/SFENCE live at 0F AE /5,/6,/7 mod==11). The "64" name variants
// (fxsave64, xsave64, ...) are the identical opcode with REX.W forced —
// not a separate opcode — matching how the kernel's fpu/internal.h picks
// between the two at compile time via inline-asm alternatives.
static void fpstate_m(SecBuf *s, uint8_t op2, int digit, int w, X86Mem m) {
    bool needrex = w || m.base > X86_RDI || (m.index != X86_NOREG && m.index > X86_RDI);
    if (needrex) emit1(s, rex(w, 0, m.index > X86_RDI, m.base > X86_RDI));
    emit2(s, 0x0f, op2);
    emit_mem(s, m.base, m.index, m.scale, m.disp, digit);
}
void x86_fxsave(SecBuf *s, int w, X86Mem m) { fpstate_m(s, 0xae, 0, w, m); }
void x86_fxrstor(SecBuf *s, int w, X86Mem m) { fpstate_m(s, 0xae, 1, w, m); }
void x86_xsave(SecBuf *s, int w, X86Mem m) { fpstate_m(s, 0xae, 4, w, m); }
void x86_xrstor(SecBuf *s, int w, X86Mem m) { fpstate_m(s, 0xae, 5, w, m); }
void x86_xsaveopt(SecBuf *s, int w, X86Mem m) { fpstate_m(s, 0xae, 6, w, m); }
void x86_xsavec(SecBuf *s, int w, X86Mem m) { fpstate_m(s, 0xc7, 4, w, m); }
void x86_xsaves(SecBuf *s, int w, X86Mem m) { fpstate_m(s, 0xc7, 5, w, m); }
void x86_xrstors(SecBuf *s, int w, X86Mem m) { fpstate_m(s, 0xc7, 3, w, m); }
void x86_ud2(SecBuf *s) { emit2(s, 0x0f, 0x0b); }

// x87 control/status instructions: single-byte opcode (D8-DF range, NOT
// 0F-prefixed like the rest of this file), no REX.W meaning. Each waiting
// ("fXXX") form is its non-waiting ("fnXXX") twin with a leading FWAIT
// (0x9B) — real hardware, real GAS, two genuinely different opcode
// sequences sharing one mnemonic root.
static void x87_m(SecBuf *s, uint8_t op, int digit, X86Mem m) {
    bool needrex = m.base > X86_RDI || (m.index != X86_NOREG && m.index > X86_RDI);
    if (needrex) emit1(s, rex(0, 0, m.index > X86_RDI, m.base > X86_RDI));
    emit1(s, op);
    emit_mem(s, m.base, m.index, m.scale, m.disp, digit);
}
void x86_fninit(SecBuf *s) { emit2(s, 0xdb, 0xe3); }
void x86_finit(SecBuf *s) {
    emit1(s, 0x9b);
    x86_fninit(s);
}
void x86_fnclex(SecBuf *s) { emit2(s, 0xdb, 0xe2); }
void x86_fclex(SecBuf *s) {
    emit1(s, 0x9b);
    x86_fnclex(s);
}
void x86_fnop(SecBuf *s) { emit2(s, 0xd9, 0xd0); }
void x86_fldcw_m(SecBuf *s, X86Mem m) { x87_m(s, 0xd9, 5, m); }
void x86_fnstcw_m(SecBuf *s, X86Mem m) { x87_m(s, 0xd9, 7, m); }
void x86_fstcw_m(SecBuf *s, X86Mem m) {
    emit1(s, 0x9b);
    x86_fnstcw_m(s, m);
}
void x86_fnstenv_m(SecBuf *s, X86Mem m) { x87_m(s, 0xd9, 6, m); }
void x86_fstenv_m(SecBuf *s, X86Mem m) {
    emit1(s, 0x9b);
    x86_fnstenv_m(s, m);
}
void x86_fnstsw_m(SecBuf *s, X86Mem m) { x87_m(s, 0xdd, 7, m); }
void x86_fnstsw_ax(SecBuf *s) { emit2(s, 0xdf, 0xe0); }
void x86_fstsw_m(SecBuf *s, X86Mem m) {
    emit1(s, 0x9b);
    x86_fnstsw_m(s, m);
}
void x86_fstsw_ax(SecBuf *s) {
    emit1(s, 0x9b);
    x86_fnstsw_ax(s);
}

// Far return (0xCB, or 0xCA iw to also pop imm16 bytes off the stack) —
// AT&T's "lret"/"lretq" are just alternate spellings of the same opcodes.
void x86_retf(SecBuf *s) { emit1(s, 0xcb); }
void x86_retf_imm(SecBuf *s, uint16_t imm16) {
    emit1(s, 0xca);
    secbuf_emit16le(s, imm16);
}
// ENTER imm16, imm8 (0xC8 iw ib): allocate a stack frame of frame_size
// bytes, nesting is almost always 0 in practice (real-mode/32-bit nested
// Pascal-style display support is the only user of a nonzero value).
void x86_enter(SecBuf *s, uint16_t frame_size, uint8_t nesting) {
    emit1(s, 0xc8);
    secbuf_emit16le(s, frame_size);
    emit1(s, nesting);
}
void x86_prefetcht1(SecBuf *s, X86Mem m) { prefetch_m(s, 0x18, 2, m); }
void x86_prefetcht2(SecBuf *s, X86Mem m) { prefetch_m(s, 0x18, 3, m); }
void x86_prefetchwt1(SecBuf *s, X86Mem m) { prefetch_m(s, 0x0d, 2, m); }
// MONITOR/MWAIT (0F 01 C8/C9): operands (address in rax, hints in
// ecx/edx) are always implicit registers — real assembly (including the
// kernel's mwait_idle()) writes them out only as documentation, they
// don't affect the encoding.
void x86_monitor(SecBuf *s) { emit3(s, 0x0f, 0x01, 0xc8); }
void x86_mwait(SecBuf *s) { emit3(s, 0x0f, 0x01, 0xc9); }
void x86_rsm(SecBuf *s) { emit2(s, 0x0f, 0xaa); }
void x86_xtest(SecBuf *s) { emit3(s, 0x0f, 0x01, 0xd6); }
void x86_xend(SecBuf *s) { emit3(s, 0x0f, 0x01, 0xd5); }
// CLZERO (0F 01 FC): zeroes the cache line at the implicit address in RAX.
void x86_clzero(SecBuf *s) { emit3(s, 0x0f, 0x01, 0xfc); }
// CLDEMOTE m8 (0F 1C /0): hint, memory operand only.
void x86_cldemote_m(SecBuf *s, X86Mem m) { prefetch_m(s, 0x1c, 0, m); }
// XABORT imm8 (C6 F8 ib), XBEGIN rel32 (C7 F8 rel32) — RTM transactional
// memory. XBEGIN's rel32 branches to the abort handler on a hardware
// abort, same fixup shape as a plain JMP.
void x86_xabort(SecBuf *s, uint8_t imm8) { emit3(s, 0xc6, 0xf8, imm8); }
void x86_xbegin_rel32(SecBuf *s, int32_t rel32) {
    emit2(s, 0xc7, 0xf8);
    emit_imm32(s, rel32);
}

// ---------------------------------------------------------------------------
// SSE / FP helpers
// ---------------------------------------------------------------------------
// SSE2 prefix: F2=double, F3=single
static void sse_rr(SecBuf *s, uint8_t pfx, uint8_t op, X86XmmReg d, X86XmmReg sr) {
    emit1(s, pfx);
    maybe_rex(s, 0, d, 0, sr);
    emit3(s, 0x0f, op, modrxmm(3, d, sr));
}
static void sse_rm(SecBuf *s, uint8_t pfx, uint8_t op, X86XmmReg d, X86Mem m) {
    emit1(s, pfx);
    maybe_rex(s, 0, d, m.index > X86_RDI ? m.index : 0, m.base);
    emit2(s, 0x0f, op);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (X86Reg)d);
}
static void sse_mr(SecBuf *s, uint8_t pfx, uint8_t op, X86Mem m, X86XmmReg sr) {
    emit1(s, pfx);
    maybe_rex(s, 0, (int)sr, m.index > X86_RDI ? m.index : 0, m.base);
    emit2(s, 0x0f, op);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (X86Reg)sr);
}

void x86_movsd_rr(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr(s, 0xf2, 0x10, d, sr); }
void x86_movsd_rm(SecBuf *s, X86XmmReg d, X86Mem m) { sse_rm(s, 0xf2, 0x10, d, m); }
void x86_movsd_mr(SecBuf *s, X86Mem m, X86XmmReg sr) { sse_mr(s, 0xf2, 0x11, m, sr); }
void x86_movss_rr(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr(s, 0xf3, 0x10, d, sr); }
void x86_movss_rm(SecBuf *s, X86XmmReg d, X86Mem m) { sse_rm(s, 0xf3, 0x10, d, m); }
void x86_movss_mr(SecBuf *s, X86Mem m, X86XmmReg sr) { sse_mr(s, 0xf3, 0x11, m, sr); }
void x86_addsd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr(s, 0xf2, 0x58, d, sr); }
void x86_subsd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr(s, 0xf2, 0x5c, d, sr); }
void x86_mulsd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr(s, 0xf2, 0x59, d, sr); }
void x86_divsd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr(s, 0xf2, 0x5e, d, sr); }
void x86_addss(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr(s, 0xf3, 0x58, d, sr); }
void x86_subss(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr(s, 0xf3, 0x5c, d, sr); }
void x86_mulss(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr(s, 0xf3, 0x59, d, sr); }
void x86_divss(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr(s, 0xf3, 0x5e, d, sr); }
void x86_movq_r_xmm(SecBuf *s, X86XmmReg d, X86Reg sr) {
    // movq %r64, %xmm: 66 REX.W 0F 6E /r (reg=d, rm=sr)
    emit1(s, 0x66);
    emit1(s, rex(1, (int)d > 7, 0, sr > X86_RDI));
    emit3(s, 0x0f, 0x6e, modrm(3, (X86Reg)d, sr));
}
void x86_movq_xmm_r(SecBuf *s, X86Reg d, X86XmmReg sr) {
    // movq %xmm, %r64: 66 REX.W 0F 7E /r (reg=sr, rm=d)
    emit1(s, 0x66);
    emit1(s, rex(1, (int)sr > 7, 0, d > X86_RDI));
    emit3(s, 0x0f, 0x7e, modrxmm(3, sr, (X86XmmReg)d));
}
void x86_ucomisd(SecBuf *s, X86XmmReg a, X86XmmReg b) {
    emit1(s, 0x66);
    maybe_rex(s, 0, a, 0, b);
    emit3(s, 0x0f, 0x2e, modrxmm(3, a, b));
}
void x86_ucomiss(SecBuf *s, X86XmmReg a, X86XmmReg b) {
    maybe_rex(s, 0, a, 0, b);
    emit3(s, 0x0f, 0x2e, modrxmm(3, a, b));
}
void x86_comisd(SecBuf *s, X86XmmReg a, X86XmmReg b) {
    emit1(s, 0x66);
    maybe_rex(s, 0, a, 0, b);
    emit3(s, 0x0f, 0x2f, modrxmm(3, a, b));
}
void x86_cvtsi2sd(SecBuf *s, int srcsz, X86XmmReg d, X86Reg sr) {
    emit2(s, 0xf2, rex(srcsz == 8, (int)d > 7, 0, sr > X86_RDI));
    emit3(s, 0x0f, 0x2a, modrxmm(3, d, (X86XmmReg)sr));
}
void x86_cvtsi2ss(SecBuf *s, int srcsz, X86XmmReg d, X86Reg sr) {
    emit2(s, 0xf3, rex(srcsz == 8, (int)d > 7, 0, sr > X86_RDI));
    emit3(s, 0x0f, 0x2a, modrxmm(3, d, (X86XmmReg)sr));
}
void x86_cvttsd2si(SecBuf *s, int dstsz, X86Reg d, X86XmmReg sr) {
    emit2(s, 0xf2, rex(dstsz == 8, d > X86_RDI, 0, (int)sr > 7));
    emit3(s, 0x0f, 0x2c, modrxmm(3, (X86XmmReg)d, sr));
}
void x86_cvttss2si(SecBuf *s, int dstsz, X86Reg d, X86XmmReg sr) {
    emit2(s, 0xf3, rex(dstsz == 8, d > X86_RDI, 0, (int)sr > 7));
    emit3(s, 0x0f, 0x2c, modrxmm(3, (X86XmmReg)d, sr));
}
void x86_cvtsd2ss(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr(s, 0xf2, 0x5a, d, sr); }
void x86_cvtss2sd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr(s, 0xf3, 0x5a, d, sr); }
void x86_xorpd(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    // xorpd: 66 0F 57 /r
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x57, modrxmm(3, d, sr));
}
void x86_xorps(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    // xorps: 0F 57 /r (no mandatory prefix)
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x57, modrxmm(3, d, sr));
}
void x86_movaps(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    // movaps: 0F 28 /r
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x28, modrxmm(3, d, sr));
}
void x86_movaps_mr(SecBuf *s, X86Mem m, X86XmmReg sr) {
    // movaps: 0F 29 /r (store)
    maybe_rex(s, 0, (int)sr, m.index > 7 ? m.index : 0, m.base);
    emit2(s, 0x0f, 0x29);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)sr);
}

void x86_movdqu_mr(SecBuf *s, X86Mem m, X86XmmReg sr) {
    // movdqu: F3 0F 7F /r (store, unaligned)
    emit1(s, 0xf3);
    maybe_rex(s, 0, (int)sr, m.index > 7 ? m.index : 0, m.base);
    emit2(s, 0x0f, 0x7f);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)sr);
}
void x86_pxor(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    // pxor: 66 0F EF /r
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0xef, modrxmm(3, d, sr));
}

// ---------------------------------------------------------------------------
// Packed SSE (128-bit vector) — used by __attribute__((vector_size(16)))
// ---------------------------------------------------------------------------
// Packed-single ops have no mandatory prefix (0F xx); packed-double add 0x66.
void sse_rr_np(SecBuf *s, uint8_t op, X86XmmReg d, X86XmmReg sr) {
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, op, modrxmm(3, d, sr));
}
void sse_rr_66(SecBuf *s, uint8_t op, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, op, modrxmm(3, d, sr));
}
void sse_rr_f3(SecBuf *s, uint8_t op, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0xf3);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, op, modrxmm(3, d, sr));
}
void sse_rr_f2(SecBuf *s, uint8_t op, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0xf2);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, op, modrxmm(3, d, sr));
}

// movups xmm, m128 (load, unaligned): 0F 10 /r
void x86_movups_rm(SecBuf *s, X86XmmReg d, X86Mem m) {
    maybe_rex(s, 0, (int)d, m.index > 7 ? m.index : 0, m.base);
    emit2(s, 0x0f, 0x10);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)d);
}
// movups m128, xmm (store, unaligned): 0F 11 /r
void x86_movups_mr(SecBuf *s, X86Mem m, X86XmmReg sr) {
    maybe_rex(s, 0, (int)sr, m.index > 7 ? m.index : 0, m.base);
    emit2(s, 0x0f, 0x11);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)sr);
}

// Packed-single float arithmetic (xmm, xmm)
void x86_addps(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_np(s, 0x58, d, sr); }
void x86_subps(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_np(s, 0x5c, d, sr); }
void x86_mulps(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_np(s, 0x59, d, sr); }
void x86_divps(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_np(s, 0x5e, d, sr); }
void x86_minps(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_np(s, 0x5d, d, sr); }
void x86_maxps(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_np(s, 0x5f, d, sr); }
// Packed-single bitwise
void x86_andps(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_np(s, 0x54, d, sr); }
void x86_andnps(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_np(s, 0x55, d, sr); }
void x86_orps(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_np(s, 0x56, d, sr); }
// Packed-single unary (dst = op(src))
void x86_sqrtps(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_np(s, 0x51, d, sr); }
void x86_sqrtss(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_f3(s, 0x51, d, sr); }
// Packed/scalar-double unary sqrt (dst = op(src))
void x86_sqrtpd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x51, d, sr); }
void x86_sqrtsd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_f2(s, 0x51, d, sr); }
// Packed-single shuffles / unpacks
void x86_unpcklps(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_np(s, 0x14, d, sr); }
void x86_unpckhps(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_np(s, 0x15, d, sr); }
void x86_movhlps(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_np(s, 0x12, d, sr); }
void x86_movlhps(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_np(s, 0x16, d, sr); }
// cmpps xmm, xmm, imm8: 0F C2 /r ib (imm: 0=eq,1=lt,2=le,4=neq,5=nlt,6=nle)
void x86_cmpps(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm) {
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0xc2, modrxmm(3, d, sr));
    emit1(s, imm);
}
// shufps xmm, xmm, imm8: 0F C6 /r ib
void x86_shufps(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm) {
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0xc6, modrxmm(3, d, sr));
    emit1(s, imm);
}
// shufpd xmm, xmm, imm8: 66 0F C6 /r ib
void x86_shufpd(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0xc6, modrxmm(3, d, sr));
    emit1(s, imm);
}

// movsldup xmm, xmm: F3 0F 12 /r (SSE3: {s0,s0,s2,s2})
void x86_movsldup(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_f3(s, 0x12, d, sr); }
// movshdup xmm, xmm: F3 0F 16 /r (SSE3: {s1,s1,s3,s3})
void x86_movshdup(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_f3(s, 0x16, d, sr); }
// movmskps r32, xmm: 0F 50 /r (reg=GP dst, rm=xmm src)
void x86_movmskps(SecBuf *s, X86Reg d, X86XmmReg sr) {
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x50, modrxmm(3, (X86XmmReg)d, sr));
}
// Packed-double arithmetic / bitwise (66 0F xx) — for __m128d (2×double)
void x86_addpd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x58, d, sr); }
void x86_subpd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x5c, d, sr); }
void x86_mulpd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x59, d, sr); }
void x86_divpd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x5e, d, sr); }
void x86_andpd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x54, d, sr); }
void x86_orpd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x56, d, sr); }
// Packed integer (66 0F xx) — for integer vector_size types
void x86_paddd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xfe, d, sr); }
void x86_psubd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xfa, d, sr); }
void x86_paddq(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xd4, d, sr); }
void x86_psubq(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xfb, d, sr); }
void x86_paddw(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xfd, d, sr); }
void x86_psubw(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xf9, d, sr); }
void x86_paddb(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xfc, d, sr); }
void x86_psubb(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xf8, d, sr); }
void x86_pand(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xdb, d, sr); }
void x86_por(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xeb, d, sr); }
void x86_pcmpeqd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x76, d, sr); }
void x86_pcmpgtd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x66, d, sr); }
// movmskpd r32, xmm: 66 0F 50 /r (GP dst in the reg field; the index
// trick matches movmskps — X86Reg and X86XmmReg share numbering)
void x86_movmskpd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x50, d, sr); }
// pmovmskb r32, mm/xmm: 66 0F D7 /r (GP dst)
void x86_pmovmskb(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xd7, d, sr); }
// PSHUFB xmm, xmm (SSSE3, 66 0F 38 00 /r): byte-lane shuffle/permute —
// dst[i] = (src[i] & 0x80) ? 0 : dst[src[i] & 0x0f], per byte lane.
void x86_pshufb(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x00);
    emit1(s, modrxmm(3, d, sr));
}
// PSHUFD xmm, xmm, imm8: 66 0F 70 /r ib (like SHUFPS/CMPPS's imm8 shape
// above, but with a mandatory 66 prefix and reading only from `sr`).
void x86_pshufd(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x70, modrxmm(3, d, sr));
    emit1(s, imm);
}
// "Group 14" (66 0F 73 /ext ib) shift-by-immediate: ModRM.reg is a
// fixed group-select value, not a real register; the single xmm
// operand is both source and destination, in ModRM.rm.
static void group14_shift_imm(SecBuf *s, X86XmmReg d, uint8_t ext, uint8_t imm) {
    emit1(s, 0x66);
    maybe_rex(s, 0, 0, 0, (int)d);
    emit3(s, 0x0f, 0x73, (uint8_t)((3 << 6) | (ext << 3) | ((int)d & 7)));
    emit1(s, imm);
}
void x86_pslldq(SecBuf *s, X86XmmReg d, uint8_t imm) { group14_shift_imm(s, d, 7, imm); }
void x86_psrldq(SecBuf *s, X86XmmReg d, uint8_t imm) { group14_shift_imm(s, d, 3, imm); }
void x86_psllq(SecBuf *s, X86XmmReg d, uint8_t imm) { group14_shift_imm(s, d, 6, imm); }
void x86_psrlq(SecBuf *s, X86XmmReg d, uint8_t imm) { group14_shift_imm(s, d, 2, imm); }
// AES-NI (66 0F 38 xx /r), all reg/reg -- real GAS also accepts an
// xmm/m128 second operand, but every real caller (OpenSSL/LibreSSL's
// aesni-x86_64.pl-generated .S files) only ever uses the register form.
void x86_aesenc(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0xdc);
    emit1(s, modrxmm(3, d, sr));
}
void x86_aesenclast(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0xdd);
    emit1(s, modrxmm(3, d, sr));
}
void x86_aesdec(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0xde);
    emit1(s, modrxmm(3, d, sr));
}
void x86_aesdeclast(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0xdf);
    emit1(s, modrxmm(3, d, sr));
}
void x86_aesimc(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0xdb);
    emit1(s, modrxmm(3, d, sr));
}
// AESKEYGENASSIST xmm, xmm, imm8: 66 0F 3A DF /r ib (the "0F 3A" 3-byte
// opcode map, like AES's own "0F 38" map above but with a trailing
// immediate).
void x86_aeskeygenassist(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x3a, 0xdf);
    emit1(s, modrxmm(3, d, sr));
    emit1(s, imm);
}
// PINSRW xmm, r32/m16, imm8: 66 0F C4 /r ib -- insert a 16-bit word
// into one of the xmm register's 8 word lanes (imm8 selects which,
// 0-7). rc4's own SSE2 fast path (rc4-*-x86_64.S) always uses the
// memory-operand form.
void x86_pinsrw_rm(SecBuf *s, X86XmmReg d, X86Mem m, uint8_t imm) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, m.index > 7 ? m.index : 0, m.base);
    emit2(s, 0x0f, 0xc4);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)d);
    emit1(s, imm);
}

// === SSE1/SSE2/SSSE3/SSE4 encoders added for __builtin_ia32_* intrinsic support ===
// two-register forms
void x86_minpd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x5d, d, sr); }
void x86_maxpd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x5f, d, sr); }
void x86_unpcklpd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x14, d, sr); }
void x86_unpckhpd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x15, d, sr); }
void x86_cvttps2pi(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_np(s, 0x2c, d, sr); }
void x86_cvtps2pi(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_np(s, 0x2d, d, sr); }
void x86_cvtpi2ps(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_np(s, 0x2a, d, sr); }
void x86_cvtps2pd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_np(s, 0x5a, d, sr); }
void x86_cvtdq2ps(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_np(s, 0x5b, d, sr); }
void x86_rcpps(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_np(s, 0x53, d, sr); }
void x86_rsqrtps(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_np(s, 0x52, d, sr); }
void x86_comiss(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_np(s, 0x2f, d, sr); }
void x86_pcmpeqb(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x74, d, sr); }
void x86_pcmpeqw(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x75, d, sr); }
void x86_pcmpgtb(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x64, d, sr); }
void x86_pcmpgtw(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x65, d, sr); }
void x86_paddsb(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xec, d, sr); }
void x86_paddsw(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xed, d, sr); }
void x86_paddusb(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xdc, d, sr); }
void x86_paddusw(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xdd, d, sr); }
void x86_psubsb(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xe8, d, sr); }
void x86_psubsw(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xe9, d, sr); }
void x86_psubusb(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xd8, d, sr); }
void x86_psubusw(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xd9, d, sr); }
void x86_pmullw(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xd5, d, sr); }
void x86_pmulhw(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xe5, d, sr); }
void x86_pmulhuw(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xe4, d, sr); }
void x86_pmuludq(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xf4, d, sr); }
void x86_pmaddwd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xf5, d, sr); }
void x86_pavgb(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xe0, d, sr); }
void x86_pavgw(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xe3, d, sr); }
void x86_psadbw(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xf6, d, sr); }
void x86_pmaxub(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xde, d, sr); }
void x86_pmaxsw(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xee, d, sr); }
void x86_pminub(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xda, d, sr); }
void x86_pminsw(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xea, d, sr); }
void x86_punpcklbw(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x60, d, sr); }
void x86_punpcklwd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x61, d, sr); }
void x86_punpckldq(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x62, d, sr); }
void x86_punpcklqdq(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x6c, d, sr); }
void x86_punpckhbw(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x68, d, sr); }
void x86_punpckhwd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x69, d, sr); }
void x86_punpckhdq(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x6a, d, sr); }
void x86_punpckhqdq(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x6d, d, sr); }
void x86_packsswb(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x63, d, sr); }
void x86_packssdw(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x6b, d, sr); }
void x86_packuswb(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x67, d, sr); }
void x86_addsubpd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xd0, d, sr); }
void x86_haddpd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x7c, d, sr); }
void x86_hsubpd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x7d, d, sr); }
void x86_cvtpd2ps(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x5a, d, sr); }
void x86_cvtps2dq(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x5b, d, sr); }
void x86_cvttpd2dq(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xe6, d, sr); }
void x86_cvtpi2pd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x2a, d, sr); }
void x86_cvtpd2pi(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x2d, d, sr); }
void x86_cvttpd2pi(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0x2c, d, sr); }
void x86_maxss(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_f3(s, 0x5f, d, sr); }
void x86_maxsd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_f2(s, 0x5f, d, sr); }
void x86_minss(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_f3(s, 0x5d, d, sr); }
void x86_minsd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_f2(s, 0x5d, d, sr); }
void x86_rcpss(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_f3(s, 0x53, d, sr); }
void x86_rsqrtss(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_f3(s, 0x52, d, sr); }
// cvtsd2si r32/64, xmm: F2 0F 2D /r (uses current MXCSR rounding).
// Prefix order matters: F2/F3 must precede the REX byte (see cvttsd2si).
void x86_cvtsd2si(SecBuf *s, int dstsz, X86Reg d, X86XmmReg sr) {
    emit2(s, 0xf2, rex(dstsz == 8, (int)d > 7, 0, (int)sr > 7));
    emit3(s, 0x0f, 0x2d, modrxmm(3, (X86XmmReg)d, sr));
}
// cvtss2si r32/64, xmm: F3 0F 2D /r
void x86_cvtss2si(SecBuf *s, int dstsz, X86Reg d, X86XmmReg sr) {
    emit2(s, 0xf3, rex(dstsz == 8, (int)d > 7, 0, (int)sr > 7));
    emit3(s, 0x0f, 0x2d, modrxmm(3, (X86XmmReg)d, sr));
}
// psllq xmm, xmm (shift left by count in xmm2's low bits): 66 0F F3 /r
void x86_psllq_r(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xf3, d, sr); }
// psrlq xmm, xmm: 66 0F D3 /r
void x86_psrlq_r(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xd3, d, sr); }
void x86_psrlw_r(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xd1, d, sr); }
void x86_psrld_r(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xd2, d, sr); }
void x86_psraw_r(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xe1, d, sr); }
void x86_psrad_r(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xe2, d, sr); }
void x86_psllw_r(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xf1, d, sr); }
void x86_pslld_r(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xf2, d, sr); }
void x86_maskmovdqu(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_66(s, 0xf7, d, sr); }
void x86_cvtdq2pd(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_f3(s, 0xe6, d, sr); }
void x86_cvtpd2dq(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_f2(s, 0xe6, d, sr); }
void x86_addsubps(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_f2(s, 0xd0, d, sr); }
void x86_haddps(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_f2(s, 0x7c, d, sr); }
void x86_hsubps(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_f2(s, 0x7d, d, sr); }
// immediate forms
void x86_pshuflw(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm) {
    emit1(s, 0xf2);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x70, modrxmm(3, d, sr));
    emit1(s, imm);
}
void x86_pshufhw(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm) {
    emit1(s, 0xf3);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x70, modrxmm(3, d, sr));
    emit1(s, imm);
}
void x86_pshufw(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm) {
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x70, modrxmm(3, d, sr));
    emit1(s, imm);
}
void x86_cmpss(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm) {
    emit1(s, 0xf3);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0xc2, modrxmm(3, d, sr));
    emit1(s, imm);
}
void x86_cmpsd(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm) {
    emit1(s, 0xf2);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0xc2, modrxmm(3, d, sr));
    emit1(s, imm);
}
void x86_cmppd(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0xc2, modrxmm(3, d, sr));
    emit1(s, imm);
}
// 66 0F 38 xx /r
void x86_phaddw(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x01);
    emit1(s, modrxmm(3, d, sr));
}
void x86_phaddd(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x02);
    emit1(s, modrxmm(3, d, sr));
}
void x86_phaddsw(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x03);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pmaddubsw(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x04);
    emit1(s, modrxmm(3, d, sr));
}
void x86_phsubw(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x05);
    emit1(s, modrxmm(3, d, sr));
}
void x86_phsubd(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x06);
    emit1(s, modrxmm(3, d, sr));
}
void x86_phsubsw(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x07);
    emit1(s, modrxmm(3, d, sr));
}
void x86_psignb(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x08);
    emit1(s, modrxmm(3, d, sr));
}
void x86_psignw(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x09);
    emit1(s, modrxmm(3, d, sr));
}
void x86_psignd(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x0a);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pmulhrsw(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x0b);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pabsb(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x1c);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pabsw(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x1d);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pabsd(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x1e);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pblendvb(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x0c);
    emit1(s, modrxmm(3, d, sr));
}
void x86_blendvps(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x0d);
    emit1(s, modrxmm(3, d, sr));
}
void x86_blendvpd(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x0e);
    emit1(s, modrxmm(3, d, sr));
}
void x86_ptest(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x17);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pmovsxbw(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x20);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pmovsxbd(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x21);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pmovsxbq(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x22);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pmovsxwd(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x23);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pmovsxwq(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x24);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pmovsxdq(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x25);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pmuldq(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x28);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pcmpeqq(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x29);
    emit1(s, modrxmm(3, d, sr));
}
// movntdqa xmm, m128: 66 0F 38 2A /r (non-temporal aligned load)
void x86_movntdqa_rm(SecBuf *s, X86Mem m, X86XmmReg d) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, m.index > 7 ? m.index : 0, m.base);
    emit2(s, 0x0f, 0x38);
    emit1(s, 0x2a);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)d);
}
void x86_packusdw(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x2b);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pmovzxbw(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x30);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pmovzxbd(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x31);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pmovzxbq(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x32);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pmovzxwd(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x33);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pmovzxwq(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x34);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pmovzxdq(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x35);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pcmpgtq(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x37);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pminsb(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x38);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pminsd(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x39);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pminuw(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x3a);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pminud(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x3b);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pmaxsb(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x3c);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pmaxsd(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x3d);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pmaxuw(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x3e);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pmaxud(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x3f);
    emit1(s, modrxmm(3, d, sr));
}
void x86_pmulld(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x40);
    emit1(s, modrxmm(3, d, sr));
}
void x86_phminposuw(SecBuf *s, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0x41);
    emit1(s, modrxmm(3, d, sr));
}
// 66 0F 3A xx /r ib
void x86_roundps(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x3a, 0x08);
    emit1(s, modrxmm(3, d, sr));
    emit1(s, imm);
}
void x86_roundpd(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x3a, 0x09);
    emit1(s, modrxmm(3, d, sr));
    emit1(s, imm);
}
void x86_roundss(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x3a, 0x0a);
    emit1(s, modrxmm(3, d, sr));
    emit1(s, imm);
}
void x86_roundsd(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x3a, 0x0b);
    emit1(s, modrxmm(3, d, sr));
    emit1(s, imm);
}
void x86_blendps(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x3a, 0x0c);
    emit1(s, modrxmm(3, d, sr));
    emit1(s, imm);
}
void x86_blendpd(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x3a, 0x0d);
    emit1(s, modrxmm(3, d, sr));
    emit1(s, imm);
}
void x86_pblendw(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x3a, 0x0e);
    emit1(s, modrxmm(3, d, sr));
    emit1(s, imm);
}
void x86_palignr(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x3a, 0x0f);
    emit1(s, modrxmm(3, d, sr));
    emit1(s, imm);
}
void x86_insertps(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x3a, 0x21);
    emit1(s, modrxmm(3, d, sr));
    emit1(s, imm);
}
void x86_dpps(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x3a, 0x40);
    emit1(s, modrxmm(3, d, sr));
    emit1(s, imm);
}
void x86_dppd(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x3a, 0x41);
    emit1(s, modrxmm(3, d, sr));
    emit1(s, imm);
}
void x86_mpsadbw(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x3a, 0x42);
    emit1(s, modrxmm(3, d, sr));
    emit1(s, imm);
}
void x86_pclmulqdq(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x3a, 0x44);
    emit1(s, modrxmm(3, d, sr));
    emit1(s, imm);
}
void x86_pcmpestrm(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x3a, 0x60);
    emit1(s, modrxmm(3, d, sr));
    emit1(s, imm);
}
void x86_pcmpestri(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x3a, 0x61);
    emit1(s, modrxmm(3, d, sr));
    emit1(s, imm);
}
void x86_pcmpistrm(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x3a, 0x62);
    emit1(s, modrxmm(3, d, sr));
    emit1(s, imm);
}
void x86_pcmpistri(SecBuf *s, X86XmmReg d, X86XmmReg sr, uint8_t imm) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x3a, 0x63);
    emit1(s, modrxmm(3, d, sr));
    emit1(s, imm);
}
// === memory-form / special SSE encoders for __builtin_ia32_* support ===
// movdqa m128, xmm: 66 0F 6F /r (aligned load)
void x86_movdqa_rm(SecBuf *s, X86Mem m, X86XmmReg d) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, m.index > 7 ? m.index : 0, m.base);
    emit2(s, 0x0f, 0x6f);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)d);
}
// movdqa xmm, m128: 66 0F 7F /r (aligned store)
void x86_movdqa_mr(SecBuf *s, X86Mem m, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)sr, m.index > 7 ? m.index : 0, m.base);
    emit2(s, 0x0f, 0x7f);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)sr);
}
// movdqu m128, xmm: F3 0F 6F /r (unaligned load)
void x86_movdqu_rm(SecBuf *s, X86Mem m, X86XmmReg d) {
    emit1(s, 0xf3);
    maybe_rex(s, 0, (int)d, m.index > 7 ? m.index : 0, m.base);
    emit2(s, 0x0f, 0x6f);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)d);
}
// movq m64, xmm: F3 0F 7E /r (64-bit load, zero-extends; also movq xmm,xmm)
void x86_movq_rm(SecBuf *s, X86Mem m, X86XmmReg d) {
    emit1(s, 0xf3);
    maybe_rex(s, 0, (int)d, m.index > 7 ? m.index : 0, m.base);
    emit2(s, 0x0f, 0x7e);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)d);
}
// movq xmm, m64: 66 0F D6 /r (64-bit store)
void x86_movq_mr(SecBuf *s, X86Mem m, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)sr, m.index > 7 ? m.index : 0, m.base);
    emit2(s, 0x0f, 0xd6);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)sr);
}
// movd r32, xmm: 66 0F 6E /r
void x86_movd_r_xmm(SecBuf *s, X86XmmReg d, X86Reg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x6e, modrxmm(3, d, (X86XmmReg)sr));
}
// movd xmm, r32: 66 0F 7E /r
void x86_movd_xmm_r(SecBuf *s, X86Reg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x7e, modrxmm(3, (X86XmmReg)d, sr));
}
// movmskpd r32, xmm: 66 0F 50 /r
// pextrw r32, xmm, imm8: 66 0F C5 /r ib
void x86_pextrw(SecBuf *s, X86Reg d, X86XmmReg sr, uint8_t imm) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0xc5, modrxmm(3, (X86XmmReg)d, sr));
    emit1(s, imm);
}
// movntps m128, xmm: 0F 2B /r (non-temporal store)
void x86_movntps_m(SecBuf *s, X86Mem m, X86XmmReg sr) {
    maybe_rex(s, 0, (int)sr, m.index > 7 ? m.index : 0, m.base);
    emit2(s, 0x0f, 0x2b);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)sr);
}
// movntpd m128, xmm: 66 0F 2B /r
void x86_movntpd_m(SecBuf *s, X86Mem m, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)sr, m.index > 7 ? m.index : 0, m.base);
    emit2(s, 0x0f, 0x2b);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)sr);
}
// movntdq m128, xmm: 66 0F E7 /r
void x86_movntdq_m(SecBuf *s, X86Mem m, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)sr, m.index > 7 ? m.index : 0, m.base);
    emit2(s, 0x0f, 0xe7);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)sr);
}
// movntq m64, mm: 0F E7 /r (MMX non-temporal store)
void x86_movntq_m(SecBuf *s, X86Mem m, X86XmmReg sr) {
    maybe_rex(s, 0, (int)sr, m.index > 7 ? m.index : 0, m.base);
    emit2(s, 0x0f, 0xe7);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)sr);
}
// movnti m32/64, r: 0F C3 /r (non-temporal integer store)
void x86_movnti_m(SecBuf *s, X86Mem m, X86Reg sr, int size) {
    maybe_rex(s, size == 8, (int)sr, m.index > 7 ? m.index : 0, m.base);
    emit2(s, 0x0f, 0xc3);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)sr);
}
// lddqu xmm, m128: F2 0F F0 /r (unaligned load, no cache-line split penalty)
void x86_lddqu_rm(SecBuf *s, X86Mem m, X86XmmReg d) {
    emit1(s, 0xf2);
    maybe_rex(s, 0, (int)d, m.index > 7 ? m.index : 0, m.base);
    emit2(s, 0x0f, 0xf0);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)d);
}
// ldmxcsr m32: 0F AE /2
void x86_ldmxcsr_m(SecBuf *s, X86Mem m) {
    maybe_rex(s, 0, 0, m.index > 7 ? m.index : 0, m.base);
    emit2(s, 0x0f, 0xae);
    emit_mem(s, m.base, m.index, m.scale, m.disp, 2);
}
// stmxcsr m32: 0F AE /3
void x86_stmxcsr_m(SecBuf *s, X86Mem m) {
    maybe_rex(s, 0, 0, m.index > 7 ? m.index : 0, m.base);
    emit2(s, 0x0f, 0xae);
    emit_mem(s, m.base, m.index, m.scale, m.disp, 3);
}
// emms: 0F 77 (empty MMX state)
void x86_emms(SecBuf *s) { emit2(s, 0x0f, 0x77); }
// femms: 0F 0D C0 (AMD 3DNow! fast emms)
void x86_femms(SecBuf *s) { emit3(s, 0x0f, 0x0d, 0xc0); }
// crc32 r32, r8: F2 0F 38 F0 /r. The 8-bit source register must be a
// low-byte reg: src indices 4-7 (spl/bpl/sil/dil) need a REX byte to be
// addressable at all (without it, rm 4-7 encode ah/ch/dh/bh).
void x86_crc32qi(SecBuf *s, X86Reg d, X86Reg sr) {
    bool need_rex = (int)d > 7 || (int)sr >= 4;
    emit1(s, 0xf2);
    if (need_rex)
        emit1(s, (uint8_t)(0x40 | ((int)d > 7 ? 4 : 0) | ((int)sr > 7 ? 1 : 0)));
    emit3(s, 0x0f, 0x38, 0xf0);
    emit1(s, modrxmm(3, (X86XmmReg)d, (X86XmmReg)sr));
}
// crc32 r32/64, r16/32/64: F2 0F 38 F1 /r (66 prefix for the 16-bit form)
void x86_crc32si(SecBuf *s, X86Reg d, X86Reg sr, int size) {
    if (size == 2)
        emit1(s, 0x66);
    emit1(s, 0xf2);
    maybe_rex(s, size == 8, (int)d, 0, (int)sr);
    emit3(s, 0x0f, 0x38, 0xf1);
    emit1(s, modrxmm(3, (X86XmmReg)d, (X86XmmReg)sr));
}


// ===== VEX (AVX/AVX2) encoders =====
// 2-byte VEX: C5 + R~vvvvLpp (map implied 0F). pp: 0=none 1=66 2=F3 3=F2.
static void vex2(SecBuf *s, int pp, int L, X86XmmReg d, X86XmmReg v, X86XmmReg rm) {
    int R = (int)d >> 3;
    int vvvv = (~(int)v) & 15;
    emit1(s, 0xc5);
    emit1(s, (uint8_t)(((~R & 1) << 7) | (vvvv << 3) | ((L & 1) << 2) | (pp & 3)));
}
// 3-byte VEX: C4 + R~X~B~mmmmm + W~vvvvLpp. map: 1=0F 2=0F38 3=0F3A.
static void vex3(SecBuf *s, int pp, int map, int W, int L, X86XmmReg d, X86XmmReg v, X86XmmReg rm) {
    int R = (int)d >> 3, B = (int)rm >> 3;
    int vvvv = (~(int)v) & 15;
    emit1(s, 0xc4);
    emit1(s, (uint8_t)(((~R & 1) << 7) | (1 << 6) | ((~B & 1) << 5) | (map & 31)));
    emit1(s, (uint8_t)(((~W & 1) << 7) | (vvvv << 3) | ((L & 1) << 2) | (pp & 3)));
}
// reg/reg/reg VEX op (256-bit unless L=0). vvvv = first source.
static void vex_rr(SecBuf *s, int pp, int map, int W, int L, int op, X86XmmReg d, X86XmmReg v, X86XmmReg rm) {
    // The VEX mmmmm field selects the opcode map (1=0F, 2=0F38, 3=0F3A),
    // so only the final opcode byte follows the prefix — no 0F/38/3A bytes.
    // Then the ModRM byte (reg=dst, rm=second source).
    if (map == 1 && (int)d < 8 && (int)rm < 8 && !W) {
        vex2(s, pp, L, d, v, rm);
        emit1(s, (uint8_t)op);
    } else {
        vex3(s, pp, map, W, L, d, v, rm);
        emit1(s, (uint8_t)op);
    }
    emit1(s, modrxmm(3, d, rm));
}
// ===================== EVEX (AVX-512) =====================
// 4-byte EVEX prefix (0x62) for 512-bit ops: P0 = R X B R' 0 0 mmmmm,
// P1 = W vvvv 1 pp, P2 = z L'L b V' aaa. L'L=10 selects 512-bit. aaa is
// the k-mask register (0 = unmasked). Register extension: R/R' cover
// reg bits 4/3, V' covers vvvv bit 4, B covers rm bit 4.
static void evex4(SecBuf *s, int pp, int map, int W, int L, int k, X86XmmReg d, X86XmmReg v, X86XmmReg rm) {
    int Rd = ((int)d >> 4) & 1, Rp = ((int)d >> 3) & 1;
    int Vp = ((int)v >> 3) & 1;
    int Bm = ((int)rm >> 4) & 1;
    int vvvv = (~(int)v) & 15;
    emit1(s, 0x62);
    emit1(s, (uint8_t)(((~Rd & 1) << 7) | (1 << 6) | ((~Bm & 1) << 5) | ((~Rp & 1) << 4) | (map & 15)));
    emit1(s, (uint8_t)(((W & 1) << 7) | (vvvv << 3) | (1 << 2) | (pp & 3))); // EVEX W is NOT inverted
    emit1(s, (uint8_t)(((L & 3) << 5) | ((~Vp & 1) << 3) | (k & 15))); // z=0, b=0
}
// reg/reg/reg EVEX op with optional k-mask (aaa). vvvv = first source.
static void evex_rr(SecBuf *s, int pp, int map, int W, int op, int k, X86XmmReg d, X86XmmReg v, X86XmmReg rm) {
    evex4(s, pp, map, W, 2, k, d, v, rm);
    emit1(s, (uint8_t)op);
    emit1(s, modrxmm(3, d, rm));
}
// EVEX immediate-group shifts (66 0F 71/72/73 /ext ib): dst=rm, ext=reg.
static void evex_group_imm(SecBuf *s, int W, int L, int grp, int ext, int k, X86XmmReg dst, X86XmmReg src, uint8_t imm) {
    // EVEX group-imm form: reg=ext, rm=SRC, vvvv=DST (gcc: vpsrld $1,%zmm_src,%zmm_dst)
    evex4(s, 1, 1, W, L, k, (X86XmmReg)ext, dst, src);
    emit1(s, (uint8_t)grp);
    emit1(s, modrxmm(3, ext, src));
    emit1(s, imm);
}
// EVEX 3-op with imm (0F3A map).
static void evex_rr_imm(SecBuf *s, int map, int op, int k, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm) {
    evex4(s, 1, map, 0, 2, k, d, v, rm);
    emit1(s, (uint8_t)op);
    emit1(s, modrxmm(3, d, rm));
    emit1(s, imm);
}
// EVEX mask compare: result in a k register (k0 invalid as mask; k1 used).
static void evex_kcmp(SecBuf *s, int map, int op, int W, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm) {
    evex4(s, 1, map, W, 2, 0, d, v, rm); // aaa=0, k-dst in ModRM.reg
    emit1(s, (uint8_t)op);
    emit1(s, modrxmm(3, d, rm));
    emit1(s, imm);
}
// VPMOVQD zmm -> ymm: 512-bit source (L'L=10), 256-bit dest.
void x86_vpmovqd512(SecBuf *s, X86XmmReg d, X86XmmReg rm) {
    evex4(s, 1, 2, 0, 2, 0, d, X86_XMM0, rm);
    emit1(s, 0x35);
    emit1(s, modrxmm(3, d, rm));
}
// 512-bit 3-op ALU encoders (exported for codegen). k=0 = unmasked.
// 64-byte vector moves: vmovups zmm, m512 / m512, zmm
void x86_vmovups_rm512(SecBuf *s, X86XmmReg d, X86Mem m) {
    evex4(s, 0, 1, 0, 2, 0, d, X86_XMM0, (X86XmmReg)m.base);
    emit1(s, 0x10);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)d);
}
void x86_vmovups_mr512(SecBuf *s, X86Mem m, X86XmmReg sr) {
    evex4(s, 0, 1, 0, 2, 0, sr, X86_XMM0, (X86XmmReg)m.base);
    emit1(s, 0x11);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)sr);
}
// vpbroadcastd/q: EVEX.512.66.0F38.W0 58/59 (xmm source -> zmm dest)
void x86_vpbroadcastd512(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, int k) {
    evex_rr(s, 1, 2, 0, 0x58, k, d, X86_XMM0, rm);
}
void x86_vpbroadcastq512(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, int k) {
    evex_rr(s, 1, 2, 1, 0x59, k, d, X86_XMM0, rm);
}
// kmovw r32, k1 (VEX.128.0F.W0 90 /r): move a k-mask to a GP register.
void x86_kmovw_r32_k1(SecBuf *s, X86Reg dstGp) {
    vex2(s, 0, 0, (X86XmmReg)dstGp, X86_XMM0, X86_XMM1); // L=0, pp=0, vvvv=1111
    emit1(s, 0x93); // KMOVW r32, k: reg=r32, rm=k
    emit1(s, modrxmm(3, dstGp, X86_XMM1));
}
// vpcmpeqd: EVEX.512.66.0F.W0 76
void x86_vpcmpeqd512(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, int k) {
    evex_rr(s, 1, 1, 0, 0x76, k, d, v, rm);
}

#define EVEX512_3OP(fn, pp, map, W, op) \
    void fn(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, int k) { evex_rr(s, pp, map, W, op, k, d, v, rm); }
EVEX512_3OP(x86_vpaddd512, 1, 1, 0, 0xfe)
EVEX512_3OP(x86_vpaddq512, 1, 1, 1, 0xd4)
EVEX512_3OP(x86_vpsubd512, 1, 1, 0, 0xfa)
EVEX512_3OP(x86_vpsubq512, 1, 1, 1, 0xfb)
EVEX512_3OP(x86_vpandd512, 1, 2, 0, 0xdb)
EVEX512_3OP(x86_vpandq512, 1, 2, 1, 0xdb)
EVEX512_3OP(x86_vpandnd512, 1, 2, 0, 0xdf)
EVEX512_3OP(x86_vpandnq512, 1, 2, 1, 0xdf)
EVEX512_3OP(x86_vpord512, 1, 2, 0, 0xeb) // 512/256 macro
EVEX512_3OP(x86_vporq512, 1, 2, 1, 0xeb) // 512/256 macro
EVEX512_3OP(x86_vpxord512, 1, 2, 0, 0xef) // 512/256 macro
EVEX512_3OP(x86_vpxorq512, 1, 2, 1, 0xef) // 512/256 macro
EVEX512_3OP(x86_vpunpckldq512, 1, 1, 0, 0x62) // 512/256 macro
EVEX512_3OP(x86_vpunpckhdq512, 1, 1, 0, 0x6a) // 512/256 macro
EVEX512_3OP(x86_vpunpcklqdq512, 1, 1, 1, 0x6c) // 512/256 macro
EVEX512_3OP(x86_vpunpckhqdq512, 1, 1, 1, 0x6d) // 512/256 macro
// 512-bit imm shifts: vpsrld/vpsrlq (72/73 /2), vprord (72 /1)
void x86_vpsrld512_i(SecBuf *s, X86XmmReg d, uint8_t imm) {
    evex_group_imm(s, 0, 2, 0x72, 2, 0, d, d, imm);
}
void x86_vpsrlq512_i(SecBuf *s, X86XmmReg d, uint8_t imm) {
    evex_group_imm(s, 1, 2, 0x73, 2, 0, d, d, imm);
}
void x86_vprord512_i(SecBuf *s, X86XmmReg d, uint8_t imm) {
    evex_group_imm(s, 0, 2, 0x72, 0, 0, d, d, imm); // VPRORD = 72 /0 (right rotate)
}
// vshufi32x4 zmm: 66 0F 3A 43 /r ib
void x86_vshufi32x4(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm) {
    evex_rr_imm(s, 3, 0x43, 0, d, v, rm, imm);
}
// VPRORD xmm: EVEX.128.66.0F.W0 72 /0 ib (L'L=00).
void x86_vprord128_i(SecBuf *s, X86XmmReg d, uint8_t imm) {
    evex_group_imm(s, 0, 0, 0x72, 0, 0, d, d, imm);
}
void x86_vprord256_i(SecBuf *s, X86XmmReg d, uint8_t imm) {
    evex_group_imm(s, 0, 1, 0x72, 0, 0, d, d, imm);
}
// VPMOVQD ymm -> xmm: EVEX.256.66.0F38.W0 35 (L'L=01).
void x86_vpmovqd256(SecBuf *s, X86XmmReg d, X86XmmReg rm) {
    evex4(s, 1, 2, 0, 1, 0, d, X86_XMM0, rm);
    emit1(s, 0x35);
    emit1(s, modrxmm(3, d, rm));
}
// VMOVDQU32 m256, ymm{k}: EVEX.256.66.0F38.W0 2F (masked 256-bit store).
void x86_vmovdqu32_mr256(SecBuf *s, X86Mem m, X86XmmReg sr) {
    evex4(s, 1, 2, 0, 1, 0, sr, X86_XMM0, (X86XmmReg)m.base);
    emit1(s, 0x2f);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)sr);
}
// VEXTRACTF64X4 zmm -> ymm: EVEX.512.66.0F3A.W1 1B /r ib (dst=rm).
void x86_vextractf64x4(SecBuf *s, X86XmmReg d, X86XmmReg rm, uint8_t imm) {
    evex4(s, 1, 3, 1, 2, 0, rm, X86_XMM0, d);
    emit1(s, 0x1b);
    emit1(s, modrxmm(3, rm, d));
    emit1(s, imm);
}
// vpcmpud (predicate in imm): result in k1 (movmsk-like); the ModRM.reg
// field holds the k destination, so d is encoded there and aaa=0.
void x86_vpcmpud512(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm) {
    evex_kcmp(s, 3, 0x1f, 0, d, v, rm, imm);
}


// vmovups ymm, m256: VEX.256.0F.WIG 10 /r (load)
void x86_vmovups_rm256(SecBuf *s, X86XmmReg d, X86Mem m) {
    vex3(s, 0, 1, 0, 1, d, X86_XMM0, (X86XmmReg)m.base);
    emit1(s, 0x10);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)d);
}
// vmovups m256, ymm: VEX.256.0F.WIG 11 /r (store)
void x86_vmovups_mr256(SecBuf *s, X86Mem m, X86XmmReg sr) {
    vex3(s, 0, 1, 0, 1, sr, X86_XMM0, (X86XmmReg)m.base);
    emit1(s, 0x11);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)sr);
}
// vmovdqa ymm, m256 (aligned): VEX.256.66.0F 6F /r
void x86_vmovdqa_rm256(SecBuf *s, X86XmmReg d, X86Mem m) {
    vex3(s, 1, 1, 0, 1, d, X86_XMM0, (X86XmmReg)m.base);
    emit1(s, 0x6f);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)d);
}
void x86_vmovdqa_mr256(SecBuf *s, X86Mem m, X86XmmReg sr) {
    vex3(s, 1, 1, 0, 1, sr, X86_XMM0, (X86XmmReg)m.base);
    emit1(s, 0x7f);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)sr);
}
// vmovdqu ymm, m256 (unaligned): VEX.256.F3.0F 6F /r
void x86_vmovdqu_rm256(SecBuf *s, X86XmmReg d, X86Mem m) {
    vex3(s, 2, 1, 0, 1, d, X86_XMM0, (X86XmmReg)m.base);
    emit1(s, 0x6f);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)d);
}
void x86_vmovdqu_mr256(SecBuf *s, X86Mem m, X86XmmReg sr) {
    vex3(s, 2, 1, 0, 1, sr, X86_XMM0, (X86XmmReg)m.base);
    emit1(s, 0x7f);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)sr);
}
// reg/reg/reg 256-bit ALU ops. All three operands are YMM registers.
#define VEX256_OP(fn, pp, op) \
    void fn(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm) { vex_rr(s, pp, 1, 0, 1, op, d, v, rm); }
VEX256_OP(x86_vaddps, 0, 0x58)
VEX256_OP(x86_vsubps, 0, 0x5c)
VEX256_OP(x86_vmulps, 0, 0x59)
VEX256_OP(x86_vdivps, 0, 0x5e)
VEX256_OP(x86_vmaxps, 0, 0x5f)
VEX256_OP(x86_vminps, 0, 0x5d)
VEX256_OP(x86_vsqrtps, 0, 0x51)
VEX256_OP(x86_vrcpps, 0, 0x53)
VEX256_OP(x86_vrsqrtps, 0, 0x52)
VEX256_OP(x86_vandps, 0, 0x54)
VEX256_OP(x86_vandnps, 0, 0x55)
VEX256_OP(x86_vorps, 0, 0x56)
VEX256_OP(x86_vxorps, 0, 0x57)
VEX256_OP(x86_vunpcklps, 0, 0x14)
VEX256_OP(x86_vunpckhps, 0, 0x15)
VEX256_OP(x86_vaddpd, 1, 0x58)
VEX256_OP(x86_vsubpd, 1, 0x5c)
VEX256_OP(x86_vmulpd, 1, 0x59)
VEX256_OP(x86_vdivpd, 1, 0x5e)
VEX256_OP(x86_vmaxpd, 1, 0x5f)
VEX256_OP(x86_vminpd, 1, 0x5d)
VEX256_OP(x86_vsqrtpd, 1, 0x51)
VEX256_OP(x86_vandpd, 1, 0x54)
VEX256_OP(x86_vandnpd, 1, 0x55)
VEX256_OP(x86_vorpd, 1, 0x56)
VEX256_OP(x86_vxorpd, 1, 0x57)
VEX256_OP(x86_vunpcklpd, 1, 0x14)
VEX256_OP(x86_vunpckhpd, 1, 0x15)
VEX256_OP(x86_vaddss, 3, 0x58) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vsubss, 3, 0x5c) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vmulss, 3, 0x59) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vdivss, 3, 0x5e) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vmaxss, 3, 0x5f) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vminss, 3, 0x5d) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vsqrtss, 3, 0x51) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vrcpss, 3, 0x53) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vrsqrtss, 3, 0x52) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vaddsd, 2, 0x58) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vsubsd, 2, 0x5c) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vmulsd, 2, 0x59) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vdivsd, 2, 0x5e) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vmaxsd, 2, 0x5f) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vminsd, 2, 0x5d) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vsqrtsd, 2, 0x51) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vrcpsd, 2, 0x53) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vrsqrtsd, 2, 0x52) // 0F/0F38/0F3A VEX.256 macro
// integer (VEX.256.66): VEX256_OP with pp=1
VEX256_OP(x86_vpaddb, 1, 0xfc) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpaddw, 1, 0xfd) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpaddd, 1, 0xfe) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpaddq, 1, 0xd4) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpsubb, 1, 0xf8) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpsubw, 1, 0xf9) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpsubd, 1, 0xfa) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpsubq, 1, 0xfb) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpaddsb, 1, 0xec) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpaddsw, 1, 0xed) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpaddusb, 1, 0xdc) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpaddusw, 1, 0xdd) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpsubsb, 1, 0xe8) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpsubsw, 1, 0xe9) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpsubusb, 1, 0xd8) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpsubusw, 1, 0xd9) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpand, 1, 0xdb) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpandn, 1, 0xdf) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpor, 1, 0xeb) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpxor, 1, 0xef) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpcmpeqb, 1, 0x74) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpcmpeqw, 1, 0x75) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpcmpeqd, 1, 0x76) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpcmpgtb, 1, 0x64) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpcmpgtw, 1, 0x65) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpcmpgtd, 1, 0x66) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpmullw, 1, 0xd5) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpmulhw, 1, 0xe5) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpmulhuw, 1, 0xe4) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpmuludq, 1, 0xf4) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpmaddwd, 1, 0xf5) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpavgb, 1, 0xe0) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpavgw, 1, 0xe3) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpsadbw, 1, 0xf6) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpmaxub, 1, 0xde) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpmaxsw, 1, 0xee) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpminub, 1, 0xda) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpminsw, 1, 0xea) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpacksswb, 1, 0x63) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpackssdw, 1, 0x6b) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpackuswb, 1, 0x67) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpunpcklbw, 1, 0x60) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpunpcklwd, 1, 0x61) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpunpckldq, 1, 0x62) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpunpcklqdq, 1, 0x6c) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpunpckhbw, 1, 0x68) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpunpckhwd, 1, 0x69) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpunpckhdq, 1, 0x6a) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpunpckhqdq, 1, 0x6d) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpsllw, 1, 0xf1) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpslld, 1, 0xf2) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpsllq, 1, 0xf3) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpsrlw, 1, 0xd1) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpsrld, 1, 0xd2) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpsrlq, 1, 0xd3) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpsraw, 1, 0xe1) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vpsrad, 1, 0xe2) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vhaddps, 2, 0x7c) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vhsubps, 2, 0x7d) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vaddsubps, 2, 0xd0) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vhaddpd, 1, 0x7c) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vhsubpd, 1, 0x7d) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vaddsubpd, 1, 0xd0) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vmovhlps, 0, 0x12) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vmovlhps, 0, 0x16) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vmovaps, 0, 0x28) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vmovapd, 1, 0x28) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vmovdqa, 1, 0x6f) // 0F/0F38/0F3A VEX.256 macro
VEX256_OP(x86_vmovdqu, 2, 0x6f) // 0F/0F38/0F3A VEX.256 macro
// vpshufd/pshuflw/pshufhw/shufps/shufpd with imm
#define VEX256_IMM(fn, pp, op) \
    void fn(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm) { vex_rr(s, pp, 1, 0, 1, op, d, v, rm); emit1(s, imm); }
VEX256_IMM(x86_vpshufd, 1, 0x70) // 0F/0F38/0F3A VEX.256 macro
VEX256_IMM(x86_vpshuflw, 3, 0x70) // 0F/0F38/0F3A VEX.256 macro
VEX256_IMM(x86_vpshufhw, 2, 0x70) // 0F/0F38/0F3A VEX.256 macro
VEX256_IMM(x86_vshufps, 0, 0xc6) // 0F/0F38/0F3A VEX.256 macro
VEX256_IMM(x86_vshufpd, 1, 0xc6) // 0F/0F38/0F3A VEX.256 macro
VEX256_IMM(x86_vcmpps, 0, 0xc2) // 0F/0F38/0F3A VEX.256 macro
VEX256_IMM(x86_vcmppd, 1, 0xc2) // 0F/0F38/0F3A VEX.256 macro
VEX256_IMM(x86_vcmpss, 3, 0xc2) // 0F/0F38/0F3A VEX.256 macro
VEX256_IMM(x86_vcmpsd, 2, 0xc2) // 0F/0F38/0F3A VEX.256 macro

// 0F38-map 256-bit ops (pp=1, map=2)
#define VEX256_38(fn, op)     void fn(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm) { vex_rr(s, 1, 2, 1, 1, op, d, v, rm); }
VEX256_38(x86_vpshufb, 0x00) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vpabsb, 0x1c) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vpabsw, 0x1d) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vpabsd, 0x1e) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vpsignb, 0x08) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vpsignw, 0x09) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vpsignd, 0x0a) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vphaddw, 0x01) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vphaddd, 0x02) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vphaddsw, 0x03) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vphsubw, 0x05) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vphsubd, 0x06) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vphsubsw, 0x07) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vpmaddubsw, 0x04) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vpmulhrsw, 0x0b) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vpcmpeqq, 0x29) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vpermd, 0x36) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vpermps, 0x16) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vpmaskmovd, 0x8c) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vpmaskmovq, 0x8d) // 0F/0F38/0F3A VEX.256 macro
// vpalignr: 0F3A map (map=3), imm
void x86_vpalignr(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm) {
    vex_rr(s, 1, 3, 1, 1, 0x0f, d, v, rm);
    emit1(s, imm);
}
// vperm2f128 / vperm2i128: 0F3A 06/46, imm
void x86_vperm2f128(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm) {
    vex_rr(s, 1, 3, 1, 1, 0x06, d, v, rm);
    emit1(s, imm);
}
void x86_vperm2i128(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm) {
    vex_rr(s, 1, 3, 1, 1, 0x46, d, v, rm);
    emit1(s, imm);
}
// vpermq/vpermpd: 0F3A 00/01, imm (v is the only source)
void x86_vpermq(SecBuf *s, X86XmmReg d, X86XmmReg rm, uint8_t imm) {
    vex_rr(s, 1, 3, 0, 1, 0x00, d, X86_XMM0, rm);
    emit1(s, imm);
}
void x86_vpermpd(SecBuf *s, X86XmmReg d, X86XmmReg rm, uint8_t imm) {
    vex_rr(s, 1, 3, 0, 1, 0x01, d, X86_XMM0, rm);
    emit1(s, imm);
}
// vpermilps/vpermilpd (variable): 0F38 0C/0D
VEX256_38(x86_vpermilps, 0x0c)
VEX256_38(x86_vpermilpd, 0x0d)
// variable shifts (AVX2): vpsllvd/q 0F38 47, vpsrlvd/q 45, vpsravd 46
VEX256_38(x86_vpsllvd, 0x47)
VEX256_38(x86_vpsrlvd, 0x45)
VEX256_38(x86_vpsravd, 0x46)
// vbroadcastss/sd: 0F38 18/19 (rm is the scalar source)
VEX256_38(x86_vbroadcastss, 0x18)
VEX256_38(x86_vbroadcastsd, 0x19)
// vpbroadcastb/w/d/q: VEX.256.66.0F38.WIG 78-7B /r (xmm scalar source)
VEX256_38(x86_vpbroadcastb, 0x78)
VEX256_38(x86_vpbroadcastw, 0x79)
VEX256_38(x86_vpbroadcastd, 0x7a)
VEX256_38(x86_vpbroadcastq, 0x7b)
// vextractf128/vextracti128: 0F3A 19/39 — xmm dest, ymm source, imm
void x86_vextractf128(SecBuf *s, X86XmmReg d, X86XmmReg rm, uint8_t imm) {
    // d=dst(xmm), rm=src(ymm): dst is encoded in ModRM.rm for VEXTRACT*
    vex_rr(s, 1, 3, 1, 1, 0x1c, rm, X86_XMM0, d);
    emit1(s, imm);
}
void x86_vextracti128(SecBuf *s, X86XmmReg d, X86XmmReg rm, uint8_t imm) {
    vex_rr(s, 1, 3, 1, 1, 0x39, rm, X86_XMM0, d);
    emit1(s, imm);
}
// vinsertf128/vinserti128: 0F3A 18/38 — ymm dest, ymm src1, xmm src2, imm
void x86_vinsertf128(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm) {
    vex_rr(s, 1, 3, 1, 1, 0x18, d, v, rm);
    emit1(s, imm);
}
void x86_vinserti128(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm) {
    vex_rr(s, 1, 3, 1, 1, 0x38, d, v, rm);
    emit1(s, imm);
}
void x86_vextractf128_pd(SecBuf *s, X86XmmReg d, X86XmmReg rm, uint8_t imm) {
    vex_rr(s, 1, 3, 1, 1, 0x19, rm, X86_XMM0, d);
    emit1(s, imm);
}
void x86_vinsertf128_pd(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm) {
    vex_rr(s, 1, 3, 1, 1, 0x19, d, v, rm);
    emit1(s, imm);
}

// ===================== AVX/AVX2 256-bit additions =====================
// 2-operand VEX.256 (vvvv=1111): dst=ModRM.reg, src=ModRM.rm.
#define VEX256_2OP(fn, pp, op) \
    void fn(SecBuf *s, X86XmmReg d, X86XmmReg rm) { vex_rr(s, pp, 1, 0, 1, op, d, 0, rm); }
VEX256_2OP(x86_vmovddup, 2, 0x12)
VEX256_2OP(x86_vmovshdup, 3, 0x16)
VEX256_2OP(x86_vmovsldup, 3, 0x12)
VEX256_2OP(x86_vcvtdq2ps, 0, 0x5b)
VEX256_2OP(x86_vcvtps2dq, 1, 0x5b)
VEX256_2OP(x86_vcvttps2dq, 2, 0x5b)
VEX256_2OP(x86_vcvtps2pd, 0, 0x5a)
VEX256_2OP(x86_vcvtpd2ps, 1, 0x5a)
VEX256_2OP(x86_vcvtdq2pd, 2, 0xe6)
VEX256_2OP(x86_vcvtpd2dq, 3, 0xe6)
VEX256_2OP(x86_vcvttpd2dq, 1, 0xe6)
// 2-operand 0F38 map (pmovsx/pmovzx)
#define VEX256_2OP_38(fn, op) \
    void fn(SecBuf *s, X86XmmReg d, X86XmmReg rm) { vex_rr(s, 1, 2, 1, 1, op, d, 0, rm); }
VEX256_2OP_38(x86_vpmovsxbw, 0x20)
VEX256_2OP_38(x86_vpmovsxbd, 0x21)
VEX256_2OP_38(x86_vpmovsxbq, 0x22)
VEX256_2OP_38(x86_vpmovsxwd, 0x23)
VEX256_2OP_38(x86_vpmovsxwq, 0x24)
VEX256_2OP_38(x86_vpmovsxdq, 0x25)
VEX256_2OP_38(x86_vpmovzxbw, 0x30)
VEX256_2OP_38(x86_vpmovzxbd, 0x31)
VEX256_2OP_38(x86_vpmovzxbq, 0x32)
VEX256_2OP_38(x86_vpmovzxwd, 0x33)
VEX256_2OP_38(x86_vpmovzxwq, 0x34)
VEX256_2OP_38(x86_vpmovzxdq, 0x35)
// vptest/vtestps/vtestpd: set flags from AND results (2-op, dst=reg)
VEX256_2OP_38(x86_vptest, 0x17)
VEX256_2OP_38(x86_vtestps, 0x0e)
VEX256_2OP_38(x86_vtestpd, 0x0f)
// vpermilps/vpermilpd IMMEDIATE forms (0F3A 05/04, 2-op) — the variable
// forms (0F38 0C/0D) already exist as x86_vpermilps/x86_vpermilpd.
#define VEX256_2OP_IMM3A(fn, pp, op) \
    void fn(SecBuf *s, X86XmmReg d, X86XmmReg rm, uint8_t imm) { vex_rr(s, pp, 3, 1, 1, op, d, 0, rm); emit1(s, imm); }
VEX256_2OP_IMM3A(x86_vpermilps_i, 1, 0x04)
VEX256_2OP_IMM3A(x86_vpermilpd_i, 1, 0x05)
// 3-operand 0F3A-map with imm8 (vblend*, vround*, vdpps, vmpsadbw)
#define VEX256_IMM3A(fn, pp, op) \
    void fn(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, uint8_t imm) { vex_rr(s, pp, 3, 1, 1, op, d, v, rm); emit1(s, imm); }
VEX256_IMM3A(x86_vblendps, 1, 0x0c) // 0F/0F38/0F3A VEX.256 macro
VEX256_IMM3A(x86_vblendpd, 1, 0x0d) // 0F/0F38/0F3A VEX.256 macro
VEX256_IMM3A(x86_vpblendw, 1, 0x0e) // 0F/0F38/0F3A VEX.256 macro
VEX256_IMM3A(x86_vpblendd, 1, 0x02) // 0F/0F38/0F3A VEX.256 macro
VEX256_IMM3A(x86_vroundps, 1, 0x08) // 0F/0F38/0F3A VEX.256 macro
VEX256_IMM3A(x86_vroundpd, 1, 0x09) // 0F/0F38/0F3A VEX.256 macro
VEX256_IMM3A(x86_vdpps, 1, 0x40) // 0F/0F38/0F3A VEX.256 macro
VEX256_IMM3A(x86_vmpsadbw, 1, 0x42) // 0F/0F38/0F3A VEX.256 macro
// vblendvps/vblendvpd/vpblendvb: 4th operand (mask) in imm8[7:4] (is4)
#define VEX256_IS4(fn, pp, op) \
    void fn(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg rm, X86XmmReg mask) { vex_rr(s, pp, 3, 1, 1, op, d, v, rm); emit1(s, (uint8_t)((int)mask << 4)); }
VEX256_IS4(x86_vblendvps, 1, 0x4a) // 0F/0F38/0F3A VEX.256 macro
VEX256_IS4(x86_vblendvpd, 1, 0x4b) // 0F/0F38/0F3A VEX.256 macro
VEX256_IS4(x86_vpblendvb, 1, 0x4c) // 0F/0F38/0F3A VEX.256 macro
// 0F38-map additions: pmulld, pmuldq, pcmpgtq, min/max 8/32
VEX256_38(x86_vpmulld, 0x40) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vpmuldq, 0x28) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vpcmpgtq, 0x37) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vpackusdw, 0x2b) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vpminsb, 0x38) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vpminsd, 0x39) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vpminuw, 0x3a) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vpminud, 0x3b) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vpmaxsb, 0x3c) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vpmaxsd, 0x3d) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vpmaxuw, 0x3e) // 0F/0F38/0F3A VEX.256 macro
VEX256_38(x86_vpmaxud, 0x3f) // 0F/0F38/0F3A VEX.256 macro
// variable-count shifts: count in XMM (128-bit), vvvv=src, reg=count
#define VEX256_SHIFT_VAR(fn, op) \
    void fn(SecBuf *s, X86XmmReg d, X86XmmReg v, X86XmmReg cnt) { vex_rr(s, 1, 1, 0, 1, op, d, v, cnt); }
VEX256_SHIFT_VAR(x86_vpsllw_r, 0xf1) // 0F/0F38/0F3A VEX.256 macro
VEX256_SHIFT_VAR(x86_vpslld_r, 0xf2) // 0F/0F38/0F3A VEX.256 macro
VEX256_SHIFT_VAR(x86_vpsllq_r, 0xf3) // 0F/0F38/0F3A VEX.256 macro
VEX256_SHIFT_VAR(x86_vpsrlw_r, 0xd1) // 0F/0F38/0F3A VEX.256 macro
VEX256_SHIFT_VAR(x86_vpsrld_r, 0xd2) // 0F/0F38/0F3A VEX.256 macro
VEX256_SHIFT_VAR(x86_vpsrlq_r, 0xd3) // 0F/0F38/0F3A VEX.256 macro
VEX256_SHIFT_VAR(x86_vpsraw_r, 0xe1) // 0F/0F38/0F3A VEX.256 macro
VEX256_SHIFT_VAR(x86_vpsrad_r, 0xe2) // 0F/0F38/0F3A VEX.256 macro
// immediate-count shifts: 66 0F 71/72/73 /ext ib, dst=rm, ext=reg, vvvv=1111
#define VEX256_SHIFT_IMM(fn, grp, ext) \
    void fn(SecBuf *s, X86XmmReg d, uint8_t imm) { vex_rr(s, 1, 1, 0, 1, grp, (X86XmmReg)ext, 0, d); emit1(s, imm); }
VEX256_SHIFT_IMM(x86_vpsllw_i, 0x71, 6) // 0F/0F38/0F3A VEX.256 macro
VEX256_SHIFT_IMM(x86_vpslld_i, 0x72, 6) // 0F/0F38/0F3A VEX.256 macro
VEX256_SHIFT_IMM(x86_vpsllq_i, 0x73, 6) // 0F/0F38/0F3A VEX.256 macro
VEX256_SHIFT_IMM(x86_vpsrlw_i, 0x71, 2) // 0F/0F38/0F3A VEX.256 macro
VEX256_SHIFT_IMM(x86_vpsrld_i, 0x72, 2) // 0F/0F38/0F3A VEX.256 macro
VEX256_SHIFT_IMM(x86_vpsrlq_i, 0x73, 2) // 0F/0F38/0F3A VEX.256 macro
VEX256_SHIFT_IMM(x86_vpsraw_i, 0x71, 4) // 0F/0F38/0F3A VEX.256 macro
VEX256_SHIFT_IMM(x86_vpsrad_i, 0x72, 4) // 0F/0F38/0F3A VEX.256 macro
VEX256_SHIFT_IMM(x86_vpslldq_i, 0x73, 7) // 0F/0F38/0F3A VEX.256 macro
VEX256_SHIFT_IMM(x86_vpsrldq_i, 0x73, 3) // 0F/0F38/0F3A VEX.256 macro
// GP-register destinations: vmovmskps/pd, vpmovmskb (dst in ModRM.reg)
#define VEX256_GP(fn, pp, op) \
    void fn(SecBuf *s, X86XmmReg dstGp, X86XmmReg src) { vex_rr(s, pp, 1, 0, 1, op, dstGp, 0, src); }
VEX256_GP(x86_vmovmskps, 0, 0x50) // 0F/0F38/0F3A VEX.256 macro
VEX256_GP(x86_vmovmskpd, 1, 0x50) // 0F/0F38/0F3A VEX.256 macro
VEX256_GP(x86_vpmovmskb256, 1, 0xd7) // 0F/0F38/0F3A VEX.256 macro
// memory 2-op forms: vlddqu, vmovntdqa (dst=reg, mem=rm)
void x86_vlddqu256(SecBuf *s, X86XmmReg d, X86Mem m) {
    vex3(s, 2, 1, 0, 1, d, X86_XMM0, (X86XmmReg)m.base);
    emit1(s, 0xf0);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)d);
}
void x86_vmovntdqa256(SecBuf *s, X86XmmReg d, X86Mem m) {
    vex3(s, 1, 2, 1, 1, d, X86_XMM0, (X86XmmReg)m.base);
    emit1(s, 0x2a);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)d);
}
void x86_vbroadcastf128(SecBuf *s, X86XmmReg d, X86Mem m) {
    vex3(s, 1, 2, 1, 1, d, X86_XMM0, (X86XmmReg)m.base);
    emit1(s, 0x1a);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)d);
}
// non-temporal stores: vmovntps/pd/dq m256, ymm (mem=rm, src=reg)
#define VEX256_NTSTORE(fn, pp, map, op) \
    void fn(SecBuf *s, X86Mem m, X86XmmReg sr) { vex3(s, pp, map, 0, 1, sr, X86_XMM0, (X86XmmReg)m.base); emit1(s, op); emit_mem(s, m.base, m.index, m.scale, m.disp, (int)sr); }
VEX256_NTSTORE(x86_vmovntps_m256, 0, 1, 0x2b)
VEX256_NTSTORE(x86_vmovntpd_m256, 1, 1, 0x2b)
VEX256_NTSTORE(x86_vmovntdq_m256, 1, 1, 0xe7)
// masked loads/stores: vpmaskmovd/q (0F38 8C-8F). Load: dst=reg, mem=rm, mask=vvvv.
void x86_vpmaskmovd_rm(SecBuf *s, X86XmmReg d, X86XmmReg mask, X86Mem m) {
    vex3(s, 1, 2, 1, 1, d, mask, (X86XmmReg)m.base);
    emit1(s, 0x8c);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)d);
}
void x86_vpmaskmovd_mr(SecBuf *s, X86Mem m, X86XmmReg data, X86XmmReg mask) {
    vex3(s, 1, 2, 1, 1, data, mask, (X86XmmReg)m.base);
    emit1(s, 0x8e);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)data);
}
void x86_vpmaskmovq_rm(SecBuf *s, X86XmmReg d, X86XmmReg mask, X86Mem m) {
    vex3(s, 1, 2, 1, 1, d, mask, (X86XmmReg)m.base);
    emit1(s, 0x8d);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)d);
}
void x86_vpmaskmovq_mr(SecBuf *s, X86Mem m, X86XmmReg data, X86XmmReg mask) {
    vex3(s, 1, 2, 1, 1, data, mask, (X86XmmReg)m.base);
    emit1(s, 0x8f);
    emit_mem(s, m.base, m.index, m.scale, m.disp, (int)data);
}


// x87
void x86_fldl_m(SecBuf *s, X86Mem m) {
    maybe_rex(s, 0, 0, m.index > 7 ? m.index : 0, m.base);
    emit1(s, 0xdd);
    emit_mem(s, m.base, m.index, m.scale, m.disp, 0);
}
void x86_fstpt_m(SecBuf *s, X86Mem m) {
    maybe_rex(s, 0, 0, m.index > 7 ? m.index : 0, m.base);
    emit1(s, 0xdb);
    emit_mem(s, m.base, m.index, m.scale, m.disp, 7);
}
void x86_fldt_m(SecBuf *s, X86Mem m) {
    // fldt: DB /5 (load m80 extended onto x87 stack)
    maybe_rex(s, 0, 0, m.index > 7 ? m.index : 0, m.base);
    emit1(s, 0xdb);
    emit_mem(s, m.base, m.index, m.scale, m.disp, 5);
}
void x86_fstpl_m(SecBuf *s, X86Mem m) {
    // fstpl: DD /3 (pop x87 stack, store as m64 double)
    maybe_rex(s, 0, 0, m.index > 7 ? m.index : 0, m.base);
    emit1(s, 0xdd);
    emit_mem(s, m.base, m.index, m.scale, m.disp, 3);
}

// SSE compare (ucomisd/ucomiss) — fix the encoding above:
// ucomisd is: 66 0F 2E /r (compare, no prefix for ucomiss)
// Above implementation has a bug; let me fix:
// Actually SSE helper needs to be refactored. The above sse_rr takes pfx as first byte,
// but ucomisd needs 66 0F 2E, not F2 0F 2E. Let me just emit directly:
// (these overwrite the implementations above)
// They're already correctly using the `emit1(s,0x66)` + sse_rr(0x0f,...) pattern which works.
#endif /* !ARCH_ARM64 */
