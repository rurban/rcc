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

static void emit1(SecBuf *s, uint8_t b) { secbuf_emit8(s, b); }
static void emit2(SecBuf *s, uint8_t a, uint8_t b) {
    emit1(s, a);
    emit1(s, b);
}
static void emit3(SecBuf *s, uint8_t a, uint8_t b, uint8_t c) {
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
static uint8_t modrxmm(int mod, X86XmmReg reg, X86XmmReg rm) {
    return (uint8_t)((mod << 6) | ((reg & 7) << 3) | (rm & 7));
}

// SIB byte
static uint8_t sib(int scale, int index, int base) {
    int ss = (scale == 8) ? 3 : (scale == 4) ? 2
        : (scale == 2)                       ? 1
                                             : 0;
    return (uint8_t)((ss << 6) | ((index & 7) << 3) | (base & 7));
}

// Emit REX if any of R,X,B > X86_RDI or force W=1
static void maybe_rex(SecBuf *s, int W, int R, int X, int B) {
    if (W || R > X86_RDI || X > X86_RDI || B > X86_RDI)
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

// REX.W for 64-bit; no REX for 32-bit; 0x66 for 16-bit; special for 8-bit
static void rex_for_size(SecBuf *s, int size, X86Reg reg, X86Reg rm) {
    if (size == 2)
        emit1(s, 0x66);
    if (size == 8)
        maybe_rex(s, 1, reg, 0, rm);
    else if (size == 4)
        maybe_rex(s, 0, reg, 0, rm);
    else if (size == 2 || size == 1) {
        // For 16-bit and 8-bit ops with registers >= R8, need REX
        if (reg >= X86_R8 || rm >= X86_R8)
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
void x86_jmp_rel32(SecBuf *s, int32_t rel32) {
    emit1(s, 0xe9);
    emit_imm32(s, rel32);
}
void x86_jmp_rel8(SecBuf *s, int8_t rel8) { emit2(s, 0xeb, (uint8_t)rel8); }
void x86_jmp_r(SecBuf *s, X86Reg r) {
    if (r > X86_RDI) emit1(s, rex(0, 0, 0, 1));
    emit2(s, 0xff, modrm(3, 4, r));
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
void x86_mfence(SecBuf *s) { emit3(s, 0x0f, 0xae, 0xf0); }
void x86_cpuid(SecBuf *s) { emit2(s, 0x0f, 0xa2); }
void x86_ud2(SecBuf *s) { emit2(s, 0x0f, 0x0b); }

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
static void sse_rr_np(SecBuf *s, uint8_t op, X86XmmReg d, X86XmmReg sr) {
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, op, modrxmm(3, d, sr));
}
static void sse_rr_66(SecBuf *s, uint8_t op, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0x66);
    maybe_rex(s, 0, (int)d, 0, (int)sr);
    emit3(s, 0x0f, op, modrxmm(3, d, sr));
}
static void sse_rr_f3(SecBuf *s, uint8_t op, X86XmmReg d, X86XmmReg sr) {
    emit1(s, 0xf3);
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
void x86_rsqrtps(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_np(s, 0x52, d, sr); }
void x86_sqrtss(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_f3(s, 0x51, d, sr); }
void x86_rcpps(SecBuf *s, X86XmmReg d, X86XmmReg sr) { sse_rr_np(s, 0x53, d, sr); }
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
