// SPDX-License-Identifier: LGPL-2.1-or-later

#include "rcc.h"
#include "asm.h"
#include "codegen_asm.h"

// Packed vector codegen: the 8/16/32/64-byte element-wise vector paths
// (gen_vector, gen_vector32_x86, gen_vector64_x86, gen_vector_splat) and
// the whole __builtin_ia32_* SIMD intrinsic dispatcher (gen_ia32_builtin
// plus its ia32_*/avx*/avx512* helpers). Split out of codegen.c; the
// gen()/gen_addr() dispatch that routes into these stays in codegen.c,
// as does the shared slot/state machinery (alloc_int128_*, the struct-ret
// offsets) exported here via codegen_asm.h.

VReg ia32_vaddr(Node *a);
VReg gen_vector(Node *node);
#ifndef ARCH_ARM64
VReg gen_vector32_x86(Node *node);
void avx_loadY(X86XmmReg y, Node *a);
VReg avx_slot_addr(void);
VReg avx_store(void);
VReg gen_vector64_x86(Node *node);
void avx512_loadZ(X86XmmReg z, Node *a);
VReg avx512_slot_addr(void);
VReg avx512_store(void);
#endif

// ===== 32-byte (AVX, YMM) vector ops =====
// Element-wise 32-byte vector arithmetic on x86-64 via VEX.256 encoders.
// Mirrors the 16-byte gen_vector path (fixed YMM2/YMM1 operands, slot-
// resident operands/results); the register allocator never manages YMM
// regs, exactly like the XMM path.
#ifndef ARCH_ARM64
VReg gen_vector32_x86(Node *node) {
    Type *ty = node->ty;
    Type *elem = ty ? ty->base : NULL;
    int esz = elem ? (int)elem->size : 4;
    bool flt = elem && is_flonum(elem);
    bool lvec = node->lhs && node->lhs->ty && node->lhs->ty->is_vector;
    bool rvec = node->rhs && node->rhs->ty && node->rhs->ty->is_vector;

    if (node->kind == ND_NEG || node->kind == ND_BITNOT) {
        VReg a = gen_addr(node->lhs);
        x86_vmovups_rm256(cg_sec, X86_XMM2, x86_mem(REG(a), 0));
        free_reg(a);
        if (node->kind == ND_BITNOT) {
            x86_vpcmpeqd(cg_sec, X86_XMM3, X86_XMM3, X86_XMM3); // all ones
            x86_vpxor(cg_sec, X86_XMM2, X86_XMM2, X86_XMM3);
        } else if (flt) {
            x86_vxorps(cg_sec, X86_XMM3, X86_XMM3, X86_XMM3);
            if (esz == 8) x86_vsubpd(cg_sec, X86_XMM2, X86_XMM3, X86_XMM2);
            else
                x86_vsubps(cg_sec, X86_XMM2, X86_XMM3, X86_XMM2);
        } else {
            x86_vpxor(cg_sec, X86_XMM3, X86_XMM3, X86_XMM3);
            if (esz == 8) x86_vpsubq(cg_sec, X86_XMM2, X86_XMM3, X86_XMM2);
            else if (esz == 4)
                x86_vpsubd(cg_sec, X86_XMM2, X86_XMM3, X86_XMM2);
            else if (esz == 2)
                x86_vpsubw(cg_sec, X86_XMM2, X86_XMM3, X86_XMM2);
            else
                x86_vpsubb(cg_sec, X86_XMM2, X86_XMM3, X86_XMM2);
        }
        VReg dst = avx_slot_addr();
        x86_vmovups_mr256(cg_sec, x86_mem(REG(dst), 0), X86_XMM2);
        return dst;
    }

    // scalar -> 32-byte broadcast
    if (lvec) {
        VReg a = gen_addr(node->lhs);
        x86_vmovups_rm256(cg_sec, X86_XMM2, x86_mem(REG(a), 0));
        free_reg(a);
    } else {
        VReg r = gen(node->lhs);
        x86_movq_r_xmm(cg_sec, X86_XMM0, REG(r));
        if (flt && esz == 4) asm_cvtsd2ss(cg_sec);
        if (esz == 8) x86_vpbroadcastq(cg_sec, X86_XMM2, X86_XMM0, X86_XMM0);
        else if (esz == 4)
            x86_vbroadcastss(cg_sec, X86_XMM2, X86_XMM0, X86_XMM0);
        else if (esz == 2)
            x86_vpbroadcastw(cg_sec, X86_XMM2, X86_XMM0, X86_XMM0);
        else
            x86_vpbroadcastb(cg_sec, X86_XMM2, X86_XMM0, X86_XMM0);
        free_reg(r);
    }
    if (rvec) {
        VReg b = gen_addr(node->rhs);
        x86_vmovups_rm256(cg_sec, X86_XMM1, x86_mem(REG(b), 0));
        free_reg(b);
    } else {
        VReg r = gen(node->rhs);
        x86_movq_r_xmm(cg_sec, X86_XMM0, REG(r));
        if (flt && esz == 4) asm_cvtsd2ss(cg_sec);
        if (esz == 8) x86_vpbroadcastq(cg_sec, X86_XMM1, X86_XMM0, X86_XMM0);
        else if (esz == 4)
            x86_vbroadcastss(cg_sec, X86_XMM1, X86_XMM0, X86_XMM0);
        else if (esz == 2)
            x86_vpbroadcastw(cg_sec, X86_XMM1, X86_XMM0, X86_XMM0);
        else
            x86_vpbroadcastb(cg_sec, X86_XMM1, X86_XMM0, X86_XMM0);
        free_reg(r);
    }

    VReg dst = avx_slot_addr();
    switch (node->kind) {
    case ND_ADD:
        if (flt) {
            if (esz == 8) x86_vaddpd(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
            else
                x86_vaddps(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
        } else if (esz == 8)
            x86_vpaddq(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
        else if (esz == 4)
            x86_vpaddd(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
        else if (esz == 2)
            x86_vpaddw(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
        else
            x86_vpaddb(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
        break;
    case ND_SUB:
        if (flt) {
            if (esz == 8) x86_vsubpd(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
            else
                x86_vsubps(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
        } else if (esz == 8)
            x86_vpsubq(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
        else if (esz == 4)
            x86_vpsubd(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
        else if (esz == 2)
            x86_vpsubw(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
        else
            x86_vpsubb(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
        break;
    case ND_MUL:
        if (!flt) {
            if (esz == 4) x86_vpmulld(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
            else if (esz == 2)
                x86_vpmullw(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
            else
                error("vector_size: integer vector multiply of %d-byte elements not supported", esz);
            break;
        }
        if (esz == 8) x86_vmulpd(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
        else
            x86_vmulps(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
        break;
    case ND_DIV:
        if (!flt) { error("vector_size: integer vector divide not supported"); }
        if (esz == 8) x86_vdivpd(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
        else
            x86_vdivps(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
        break;
    case ND_BITAND:
        if (flt) {
            if (esz == 8) x86_vandpd(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
            else
                x86_vandps(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
        } else
            x86_vpand(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
        break;
    case ND_BITOR:
        if (flt) {
            if (esz == 8) x86_vorpd(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
            else
                x86_vorps(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
        } else
            x86_vpor(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
        break;
    case ND_BITXOR:
        if (flt) {
            if (esz == 8) x86_vxorpd(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
            else
                x86_vxorps(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
        } else
            x86_vpxor(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
        break;
    case ND_LT:
        if (!flt) { error("vector_size: integer vector compare not supported"); }
        if (esz == 8) x86_vcmppd(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1, 1);
        else
            x86_vcmpps(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1, 1);
        break;
    case ND_LE:
        if (!flt) { error("vector_size: integer vector compare not supported"); }
        if (esz == 8) x86_vcmppd(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1, 2);
        else
            x86_vcmpps(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1, 2);
        break;
    case ND_EQ:
        if (flt) {
            if (esz == 8) x86_vcmppd(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1, 0);
            else
                x86_vcmpps(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1, 0);
        } else if (esz == 8)
            x86_vpcmpeqq(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
        else if (esz == 4)
            x86_vpcmpeqd(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
        else if (esz == 2)
            x86_vpcmpeqw(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
        else
            x86_vpcmpeqb(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1);
        break;
    case ND_NE:
        if (!flt) { error("vector_size: integer vector compare not supported"); }
        if (esz == 8) x86_vcmppd(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1, 4);
        else
            x86_vcmpps(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1, 4);
        break;
    default:
        error("vector_size: unsupported 32-byte vector op %d", node->kind);
    }
    x86_vmovups_mr256(cg_sec, x86_mem(REG(dst), 0), X86_XMM2);
    return dst;
}
#endif

#ifndef ARCH_ARM64
// ===== 64-byte (AVX-512, ZMM) vector ops =====
// Mirrors the 32-byte path with ZMM0-3 and EVEX encoders.
void avx512_loadZ(X86XmmReg z, Node *a) {
    VReg va = ia32_vaddr(a);
    x86_vmovups_rm512(cg_sec, z, x86_mem(REG(va), 0));
    free_reg(va);
}
VReg avx512_slot_addr(void) {
    VReg dst = alloc_int128_addr();
    alloc_int128_slot();
    alloc_int128_slot();
    alloc_int128_slot(); // 64-byte slot (four int128 slots)
    return dst;
}
VReg avx512_store(void) {
    VReg dst = avx512_slot_addr();
    x86_vmovups_mr512(cg_sec, x86_mem(REG(dst), 0), X86_XMM0);
    return dst;
}
VReg gen_vector64_x86(Node *node) {
    Type *ty = node->ty;
    Type *elem = ty ? ty->base : NULL;
    int esz = elem ? (int)elem->size : 4;
    bool flt = elem && is_flonum(elem);

    // ND_NEG / ND_BITNOT: 0 - v / v ^ allones
    if (node->kind == ND_NEG || node->kind == ND_BITNOT) {
        VReg a = gen_addr(node->lhs);
        avx512_loadZ(X86_XMM2, node->lhs);
        free_reg(a);
        if (node->kind == ND_BITNOT) {
            x86_vpternlogd512(cg_sec, X86_XMM3, X86_XMM3, X86_XMM3, 0xff); // all ones
            x86_vpxord512(cg_sec, X86_XMM2, X86_XMM2, X86_XMM3, 0);
        } else if (flt) {
            error("vector_size 64: float negation not supported");
        } else if (esz == 8) {
            x86_vpxorq512(cg_sec, X86_XMM3, X86_XMM3, X86_XMM3, 0);
            x86_vpsubq512(cg_sec, X86_XMM2, X86_XMM3, X86_XMM2, 0);
        } else {
            x86_vpxord512(cg_sec, X86_XMM3, X86_XMM3, X86_XMM3, 0);
            x86_vpsubd512(cg_sec, X86_XMM2, X86_XMM3, X86_XMM2, 0);
        }
        VReg dst = avx512_slot_addr();
        x86_vmovups_mr512(cg_sec, x86_mem(REG(dst), 0), X86_XMM2);
        return dst;
    }

    // Operands: ZMM2 = lhs, ZMM1 = rhs (scalars broadcast via vpbroadcastd/q)
    bool lvec = node->lhs && node->lhs->ty && node->lhs->ty->is_vector;
    bool rvec = node->rhs && node->rhs->ty && node->rhs->ty->is_vector;
    if (lvec) {
        avx512_loadZ(X86_XMM2, node->lhs);
    } else {
        VReg r = gen(node->lhs);
        x86_movq_r_xmm(cg_sec, X86_XMM0, REG(r));
        if (esz == 8) x86_vpbroadcastq512(cg_sec, X86_XMM2, X86_XMM0, X86_XMM0, 0);
        else
            x86_vpbroadcastd512(cg_sec, X86_XMM2, X86_XMM0, X86_XMM0, 0);
        free_reg(r);
    }
    if (rvec) {
        avx512_loadZ(X86_XMM1, node->rhs);
    } else {
        VReg r = gen(node->rhs);
        x86_movq_r_xmm(cg_sec, X86_XMM0, REG(r));
        if (esz == 8) x86_vpbroadcastq512(cg_sec, X86_XMM1, X86_XMM0, X86_XMM0, 0);
        else
            x86_vpbroadcastd512(cg_sec, X86_XMM1, X86_XMM0, X86_XMM0, 0);
        free_reg(r);
    }

    VReg dst = avx512_slot_addr();
    switch (node->kind) {
    case ND_ADD:
        if (esz == 8) x86_vpaddq512(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1, 0);
        else
            x86_vpaddd512(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1, 0);
        break;
    case ND_SUB:
        if (esz == 8) x86_vpsubq512(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1, 0);
        else
            x86_vpsubd512(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1, 0);
        break;
    case ND_BITAND:
        if (esz == 8) x86_vpandq512(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1, 0);
        else
            x86_vpandd512(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1, 0);
        break;
    case ND_BITOR:
        if (esz == 8) x86_vporq512(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1, 0);
        else
            x86_vpord512(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1, 0);
        break;
    case ND_BITXOR:
        if (esz == 8) x86_vpxorq512(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1, 0);
        else
            x86_vpxord512(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1, 0);
        break;
    default:
        error("vector_size 64: unsupported vector op");
    }
    x86_vmovups_mr512(cg_sec, x86_mem(REG(dst), 0), X86_XMM2);
    return dst;
}
#endif

#ifdef ARCH_ARM64
VReg gen_vector_unary_builtin(Node *node);
VReg gen_vector_binary_builtin(Node *node);
#endif
#ifndef ARCH_ARM64
VReg gen_ia32_builtin(Node *node);
#endif

// Scalar -> vector cast: GCC broadcasts the scalar to every lane. The
// scalar value (int/float bit pattern in a GP register) is moved into an
// XMM register and replicated lane-wise; the result materializes in a
// 16-byte slot (vectors are slot-resident in rcc).
#ifndef ARCH_ARM64
VReg gen_vector_splat(Node *scalar, Type *vty) {
    Type *elem = vty ? vty->base : NULL;
    int esz = elem ? (int)elem->size : 4;
    VReg r = gen(scalar);
    VReg dst = alloc_int128_addr();
    if (esz == 8) {
        x86_movq_r_xmm(cg_sec, X86_XMM0, REG(r)); // movq r, %xmm0
        x86_punpcklqdq(cg_sec, X86_XMM0, X86_XMM0); // {v, v}
    } else if (esz == 4) {
        if (elem && is_flonum(elem)) {
            // rcc keeps float VALUES in GP registers as double bit
            // patterns; narrow to single before broadcasting.
            x86_movq_r_xmm(cg_sec, X86_XMM0, REG(r));
            asm_cvtsd2ss(cg_sec);
        } else {
            x86_movd_r_xmm(cg_sec, X86_XMM0, REG(r)); // movd r, %xmm0
        }
        x86_shufps(cg_sec, X86_XMM0, X86_XMM0, 0); // broadcast
    } else if (esz == 2) {
        x86_movd_r_xmm(cg_sec, X86_XMM0, REG(r));
        x86_punpcklwd(cg_sec, X86_XMM0, X86_XMM0);
        x86_punpcklwd(cg_sec, X86_XMM0, X86_XMM0);
    } else {
        x86_movd_r_xmm(cg_sec, X86_XMM0, REG(r));
        x86_punpcklbw(cg_sec, X86_XMM0, X86_XMM0);
        x86_punpcklbw(cg_sec, X86_XMM0, X86_XMM0);
        x86_punpcklbw(cg_sec, X86_XMM0, X86_XMM0);
    }
    free_reg(r);
    x86_movups_mr(cg_sec, x86_mem(REG(dst), 0), X86_XMM0);
    return dst;
}
#endif

VReg gen_vector(Node *node) {
    Type *ty = node->ty;
    Type *elem = ty ? ty->base : NULL;
    int esz __attribute__((unused)) = elem ? (int)elem->size : 4;
    bool flt = elem && is_flonum(elem);

    // Unary: -v (negate) and ~v (bitwise not)
    if (node->kind == ND_NEG || node->kind == ND_BITNOT) {
        VReg a = gen_addr(node->lhs);
#ifdef ARCH_ARM64
        asm_ldr_q(cg_sec, ASM_Q0, a); // ldr q0, [x{a}]
        free_reg(a);
        VReg dst = alloc_int128_addr();
        if (node->kind == ND_BITNOT) {
            asm_not_v16b(cg_sec, ASM_Q0, ASM_Q0); // not v0.16b, v0.16b
        } else if (flt) {
            asm_fneg_v4s(cg_sec, ASM_Q0, ASM_Q0); // fneg v0.4s, v0.4s
        } else {
            // integer negation via 0 - v (use movi + sub)
            asm_eor_v16b(cg_sec, ASM_Q1, ASM_Q1, ASM_Q1); // eor v1, v1, v1 → zero
            // sub v0, v1, v0 (element-size dependent): use sub from zero
            // For simplicity, use NOT+ADD: not v0; addv?? No, need element-wise sub.
            // Easiest: NEG = NOT + ADD immediate 1 per element.
            // For now: if esz==4 use not+sshr? Actually neg = 0 - v.
            // We don't have a per-element integer sub from zero easily.
            // Use: mvni to get all-ones, then sub.
            // Simpler: use sshl with -1? No. Let's just use FMOV zero + SUB.
            // Actually, just error for integer vector negate on ARM64 for now:
            error("vector_size: integer vector negate not yet implemented on arm64");
            return R_NONE;
        }
        asm_str_q(cg_sec, ASM_Q0, dst); // str q0, [x{dst}]
        return dst;
#else
        x86_movups_rm(cg_sec, X86_XMM0, x86_mem(REG(a), 0));
        free_reg(a);
        VReg dst = alloc_int128_addr();
        if (node->kind == ND_BITNOT) {
            x86_pcmpeqd(cg_sec, X86_XMM1, X86_XMM1); // xmm1 = all ones
            x86_pxor(cg_sec, X86_XMM0, X86_XMM1); // ~a
        } else if (flt) {
            // TODO: use sign-bit XOR instead of 0-a to preserve -0 and NaN sign
            x86_xorps(cg_sec, X86_XMM1, X86_XMM1); // xmm1 = 0.0
            if (esz == 8) x86_subpd(cg_sec, X86_XMM1, X86_XMM0);
            else
                x86_subps(cg_sec, X86_XMM1, X86_XMM0); // 0 - a
            x86_movaps(cg_sec, X86_XMM0, X86_XMM1);
        } else {
            x86_pxor(cg_sec, X86_XMM1, X86_XMM1); // xmm1 = 0
            if (esz == 8) x86_psubq(cg_sec, X86_XMM1, X86_XMM0);
            else if (esz == 4)
                x86_psubd(cg_sec, X86_XMM1, X86_XMM0);
            else if (esz == 2)
                x86_psubw(cg_sec, X86_XMM1, X86_XMM0);
            else
                x86_psubb(cg_sec, X86_XMM1, X86_XMM0);
            x86_movaps(cg_sec, X86_XMM0, X86_XMM1);
        }
        x86_movups_mr(cg_sec, x86_mem(REG(dst), 0), X86_XMM0);
        return dst;
#endif
    }

    bool lvec = node->lhs && node->lhs->ty && node->lhs->ty->is_vector;
    bool rvec = node->rhs && node->rhs->ty && node->rhs->ty->is_vector;
#define EMIT_BROADCAST(reg, node) do { \
    Type *_nt = (node)->ty; \
    Type *_et = elem; \
    int _esz = _et ? (int)_et->size : 4; \
    if (_nt && is_flonum(_nt)) { \
        VReg _r = gen(node); \
        x86_movq_r_xmm(cg_sec, reg, REG(_r)); \
        free_reg(_r); \
        if (_esz <= 4) { \
            x86_cvtsd2ss(cg_sec, reg, reg); \
            x86_shufps(cg_sec, reg, reg, 0); \
        } else { \
            x86_movlhps(cg_sec, reg, reg); \
        } \
    } else if (_et && is_flonum(_et)) { \
        VReg _r = gen(node); \
        if (_esz == 8) \
            x86_cvtsi2sd(cg_sec, 4, reg, REG(_r)); \
        else \
            x86_cvtsi2ss(cg_sec, 4, reg, REG(_r)); \
        free_reg(_r); \
        if (_esz <= 4) \
            x86_shufps(cg_sec, reg, reg, 0); \
        else \
            x86_movlhps(cg_sec, reg, reg); \
    } else { \
        VReg _r = gen(node); \
        x86_movq_r_xmm(cg_sec, reg, REG(_r)); \
        free_reg(_r); \
        if (_esz <= 4) \
            x86_shufps(cg_sec, reg, reg, 0); \
        else \
            x86_movlhps(cg_sec, reg, reg); \
    } \
} while(0)

#ifdef ARCH_ARM64
    VReg addr_lhs = R_NONE, addr_rhs = R_NONE;
    if (lvec) {
        addr_lhs = gen_addr(node->lhs);
    }
    if (rvec) {
        addr_rhs = gen_addr(node->rhs);
    }
    if (lvec) {
        asm_ldr_q(cg_sec, ASM_Q2, addr_lhs);
        free_reg(addr_lhs);
    } else {
        VReg a = gen_scalar_addr(node->lhs);
        VReg t = alloc_reg();
        if (esz == 8)
            arm64_ldr_imm(cg_sec, 1, REG(t), REG(a), 0, false);
        else
            arm64_ldr_uoff(cg_sec, 2, REG(t), REG(a), 1);
        free_reg(a);
        asm_dup_gen(cg_sec, ASM_Q2, t, esz);
        free_reg(t);
    }
    if (rvec) {
        asm_ldr_q(cg_sec, ASM_Q1, addr_rhs);
        free_reg(addr_rhs);
    } else {
        VReg b = gen_scalar_addr(node->rhs);
        VReg t = alloc_reg();
        if (esz == 8)
            arm64_ldr_imm(cg_sec, 1, REG(t), REG(b), 0, false);
        else
            arm64_ldr_uoff(cg_sec, 2, REG(t), REG(b), 1);
        free_reg(b);
        asm_dup_gen(cg_sec, ASM_Q1, t, esz);
        free_reg(t);
    }
    asm_fmov_q(cg_sec, ASM_Q0, ASM_Q2); // mov v0.16b, v2.16b  (lhs→Q0)
    VReg dst = alloc_int128_addr();
    switch (node->kind) {
    case ND_ADD:
        if (flt) esz == 8 ? asm_fadd_v2d(cg_sec, ASM_Q0, ASM_Q0, ASM_Q1)
                          : asm_fadd_v4s(cg_sec, ASM_Q0, ASM_Q0, ASM_Q1);
        else
            error("vector_size: integer vector add not yet implemented on arm64");
        break;
    case ND_SUB:
        if (flt) esz == 8 ? asm_fsub_v2d(cg_sec, ASM_Q0, ASM_Q0, ASM_Q1)
                          : asm_fsub_v4s(cg_sec, ASM_Q0, ASM_Q0, ASM_Q1);
        else
            error("vector_size: integer vector sub not yet implemented on arm64");
        break;
    case ND_MUL:
        if (!flt) error("vector_size: integer vector multiply not supported");
        esz == 8 ? asm_fmul_v2d(cg_sec, ASM_Q0, ASM_Q0, ASM_Q1)
                 : asm_fmul_v4s(cg_sec, ASM_Q0, ASM_Q0, ASM_Q1);
        break;
    case ND_DIV:
        if (!flt) error("vector_size: integer vector divide not supported");
        esz == 8 ? asm_fdiv_v2d(cg_sec, ASM_Q0, ASM_Q0, ASM_Q1)
                 : asm_fdiv_v4s(cg_sec, ASM_Q0, ASM_Q0, ASM_Q1);
        break;
    case ND_BITAND:
        asm_and_v16b(cg_sec, ASM_Q0, ASM_Q0, ASM_Q1);
        break;
    case ND_BITOR:
        asm_orr_v16b(cg_sec, ASM_Q0, ASM_Q0, ASM_Q1);
        break;
    case ND_BITXOR:
        asm_eor_v16b(cg_sec, ASM_Q0, ASM_Q0, ASM_Q1);
        break;
    case ND_LT:
        if (!flt) error("vector_size: integer vector compare not supported");
        esz == 8 ? asm_fcmgt_v2d(cg_sec, ASM_Q0, ASM_Q1, ASM_Q0)
                 : asm_fcmgt_v4s(cg_sec, ASM_Q0, ASM_Q1, ASM_Q0);
        break;
    case ND_LE:
        if (!flt) error("vector_size: integer vector compare not supported");
        esz == 8 ? asm_fcmge_v2d(cg_sec, ASM_Q0, ASM_Q1, ASM_Q0)
                 : asm_fcmge_v4s(cg_sec, ASM_Q0, ASM_Q1, ASM_Q0);
        break;
    case ND_EQ:
        if (flt)
            esz == 8 ? asm_fcmeq_v2d(cg_sec, ASM_Q0, ASM_Q0, ASM_Q1)
                     : asm_fcmeq_v4s(cg_sec, ASM_Q0, ASM_Q0, ASM_Q1);
        else
            asm_fcmeq_v4s(cg_sec, ASM_Q0, ASM_Q0, ASM_Q1);
        break;
    case ND_NE:
        if (!flt) error("vector_size: integer vector compare not supported");
        esz == 8 ? asm_fcmeq_v2d(cg_sec, ASM_Q0, ASM_Q0, ASM_Q1)
                 : asm_fcmeq_v4s(cg_sec, ASM_Q0, ASM_Q0, ASM_Q1);
        asm_not_v16b(cg_sec, ASM_Q0, ASM_Q0);
        break;
    default:
        error("vector_size: unsupported vector op %d", node->kind);
    }
    asm_str_q(cg_sec, ASM_Q0, dst); // str q0, [x{dst}]
    return dst;
#else
    /* x86: load lhs into xmm2, rhs into xmm1, with scalar broadcast support */
    VReg addr_lhs = R_NONE, addr_rhs = R_NONE;
    if (lvec) {
        addr_lhs = gen_addr(node->lhs);
    }
    if (rvec) {
        addr_rhs = gen_addr(node->rhs);
    }
    /* Load lhs into xmm2, rhs into xmm1, with scalar broadcast support */
    if (lvec && rvec) {
        x86_movups_rm(cg_sec, X86_XMM2, x86_mem(REG(addr_lhs), 0));
        free_reg(addr_lhs);
        x86_movups_rm(cg_sec, X86_XMM1, x86_mem(REG(addr_rhs), 0));
        free_reg(addr_rhs);
    } else if (lvec) {
        x86_movups_rm(cg_sec, X86_XMM2, x86_mem(REG(addr_lhs), 0));
        free_reg(addr_lhs);
        EMIT_BROADCAST(X86_XMM1, node->rhs);
    } else {
        EMIT_BROADCAST(X86_XMM2, node->lhs);
        if (rvec) {
            x86_movups_rm(cg_sec, X86_XMM1, x86_mem(REG(addr_rhs), 0));
            free_reg(addr_rhs);
        } else {
            EMIT_BROADCAST(X86_XMM1, node->rhs);
        }
    }
    x86_movaps(cg_sec, X86_XMM0, X86_XMM2); // xmm0 = lhs
    VReg dst = alloc_int128_addr();
    switch (node->kind) {
    case ND_ADD:
        if (flt) esz == 8 ? x86_addpd(cg_sec, X86_XMM0, X86_XMM1) : x86_addps(cg_sec, X86_XMM0, X86_XMM1);
        else if (esz == 8)
            x86_paddq(cg_sec, X86_XMM0, X86_XMM1);
        else if (esz == 4)
            x86_paddd(cg_sec, X86_XMM0, X86_XMM1);
        else if (esz == 2)
            x86_paddw(cg_sec, X86_XMM0, X86_XMM1);
        else
            x86_paddb(cg_sec, X86_XMM0, X86_XMM1);
        break;
    case ND_SUB:
        if (flt) esz == 8 ? x86_subpd(cg_sec, X86_XMM0, X86_XMM1) : x86_subps(cg_sec, X86_XMM0, X86_XMM1);
        else if (esz == 8)
            x86_psubq(cg_sec, X86_XMM0, X86_XMM1);
        else if (esz == 4)
            x86_psubd(cg_sec, X86_XMM0, X86_XMM1);
        else if (esz == 2)
            x86_psubw(cg_sec, X86_XMM0, X86_XMM1);
        else
            x86_psubb(cg_sec, X86_XMM0, X86_XMM1);
        break;
    case ND_MUL:
        if (!flt) error("vector_size: integer vector multiply not supported");
        esz == 8 ? x86_mulpd(cg_sec, X86_XMM0, X86_XMM1) : x86_mulps(cg_sec, X86_XMM0, X86_XMM1);
        break;
    case ND_DIV:
        if (!flt) error("vector_size: integer vector divide not supported");
        esz == 8 ? x86_divpd(cg_sec, X86_XMM0, X86_XMM1) : x86_divps(cg_sec, X86_XMM0, X86_XMM1);
        break;
    case ND_BITAND:
        if (flt) esz == 8 ? x86_andpd(cg_sec, X86_XMM0, X86_XMM1) : x86_andps(cg_sec, X86_XMM0, X86_XMM1);
        else
            x86_pand(cg_sec, X86_XMM0, X86_XMM1);
        break;
    case ND_BITOR:
        if (flt) esz == 8 ? x86_orpd(cg_sec, X86_XMM0, X86_XMM1) : x86_orps(cg_sec, X86_XMM0, X86_XMM1);
        else
            x86_por(cg_sec, X86_XMM0, X86_XMM1);
        break;
    case ND_BITXOR:
        if (flt) x86_xorps(cg_sec, X86_XMM0, X86_XMM1);
        else
            x86_pxor(cg_sec, X86_XMM0, X86_XMM1);
        break;
    case ND_LT:
        if (!flt) error("vector_size: integer vector compare not supported");
        x86_cmpps(cg_sec, X86_XMM0, X86_XMM1, 1); // xmm0 = lhs < rhs
        break;
    case ND_LE:
        if (!flt) error("vector_size: integer vector compare not supported");
        x86_cmpps(cg_sec, X86_XMM0, X86_XMM1, 2); // xmm0 = lhs <= rhs
        break;
    case ND_EQ:
        if (flt) x86_cmpps(cg_sec, X86_XMM0, X86_XMM1, 0);
        else
            x86_pcmpeqd(cg_sec, X86_XMM0, X86_XMM1);
        break;
    case ND_NE:
        if (!flt) error("vector_size: integer vector compare not supported");
        x86_cmpps(cg_sec, X86_XMM0, X86_XMM1, 4); // xmm0 = lhs != rhs
        break;
    default:
        error("vector_size: unsupported vector op %d", node->kind);
    }
    x86_movups_mr(cg_sec, x86_mem(REG(dst), 0), X86_XMM0);
    return dst;
#endif
}

// Single-arg vector builtins: __builtin_ia32_sqrtps/sqrtss/rsqrtps
#ifdef ARCH_ARM64
VReg gen_vector_unary_builtin(Node *node) {
    VReg a = gen_addr(node->args);
#ifdef ARCH_ARM64
    asm_ldr_q(cg_sec, ASM_Q0, a); // ldr q0, [x{a}]
    free_reg(a);
    VReg dst = alloc_int128_addr();
    const char *name = node->funcname ? node->funcname : (node->lhs && node->lhs->var ? node->lhs->var->name : NULL);
    if (name) {
        if (!strcmp(name, "__builtin_ia32_sqrtps"))
            asm_fsqrt_v4s(cg_sec, ASM_Q0, ASM_Q0); // fsqrt v0.4s, v0.4s
        else if (!strcmp(name, "__builtin_ia32_sqrtss")) {
            // Scalar fsqrt zeros upper 96 bits of V0 on ARM64.
            // Preserve lanes 1-3 via scratch register V2.
            secbuf_emit32le(cg_sec, 0x1E204002u); // fmov s2, s0  (copy lane 0 to s2, leaves V0 intact)
            secbuf_emit32le(cg_sec, 0x1E21C042u); // fsqrt s2, s2  (scalar sqrt, zeros V2 upper bits)
            secbuf_emit32le(cg_sec, 0x6E040440u); // mov.s v0[0], v2[0]  (insert sqrt result into lane 0)
        } else if (!strcmp(name, "__builtin_ia32_sqrtpd")) {
            asm_fsqrt_v2d(cg_sec, ASM_Q0, ASM_Q0); // fsqrt v0.2d, v0.2d
        } else if (!strcmp(name, "__builtin_ia32_sqrtsd")) {
            // Scalar double sqrt of lane 0; preserve lane 1 via scratch V2.
            secbuf_emit32le(cg_sec, 0x1E604002u); // fmov d2, d0  (copy lane 0 to d2)
            secbuf_emit32le(cg_sec, 0x1E61C042u); // fsqrt d2, d2  (scalar double sqrt)
            secbuf_emit32le(cg_sec, 0x6E080440u); // ins v0.d[0], v2.d[0]  (write result to lane 0)
        } else if (!strcmp(name, "__builtin_ia32_rsqrtps")) {
            // FRSQRTE alone gives ~8-bit precision; one Newton step gets ~16 bits
            // (matching Intel RSQRTPS precision).  Formula:
            //   e1 = e * (3.0 - a * e^2) / 2.0
            asm_fmov_q(cg_sec, ASM_Q3, ASM_Q0); // mov v3.16b, v0.16b  (save input a)
            asm_frsqrte_v4s(cg_sec, ASM_Q0, ASM_Q0); // v0 = frsqrte(a) estimate
            asm_fmul_v4s(cg_sec, ASM_Q1, ASM_Q0, ASM_Q0); // v1 = e^2
            asm_frsqrts_v4s(cg_sec, ASM_Q1, ASM_Q3, ASM_Q1); // v1 = (3 - a*e^2)/2
            asm_fmul_v4s(cg_sec, ASM_Q0, ASM_Q0, ASM_Q1); // v0 = e * v1  (refined)
        }
    }
    asm_str_q(cg_sec, ASM_Q0, dst); // str q0, [x{dst}]
    return dst;
#else
    x86_movups_rm(cg_sec, X86_XMM0, x86_mem(REG(a), 0));
    free_reg(a);
    VReg dst = alloc_int128_addr();
    const char *name = node->funcname ? node->funcname : (node->lhs && node->lhs->var ? node->lhs->var->name : NULL);
    if (name) {
        if (!strcmp(name, "__builtin_ia32_sqrtps"))
            x86_sqrtps(cg_sec, X86_XMM0, X86_XMM0);
        else if (!strcmp(name, "__builtin_ia32_sqrtss"))
            x86_sqrtss(cg_sec, X86_XMM0, X86_XMM0);
        else if (!strcmp(name, "__builtin_ia32_rsqrtps"))
            x86_rsqrtps(cg_sec, X86_XMM0, X86_XMM0);
        else if (!strcmp(name, "__builtin_ia32_sqrtpd"))
            x86_sqrtpd(cg_sec, X86_XMM0, X86_XMM0);
        else if (!strcmp(name, "__builtin_ia32_sqrtsd"))
            x86_sqrtsd(cg_sec, X86_XMM0, X86_XMM0);
    }
    x86_movups_mr(cg_sec, x86_mem(REG(dst), 0), X86_XMM0);
    return dst;
#endif
}
#endif /* ARCH_ARM64 */

// Two-arg vector builtins: __builtin_ia32_pshufb128 (x86 SSSE3 only —
// _mm_shuffle_epi8's real implementation; ARM64's NEON TBL equivalent
// isn't wired up, matching several other x86-only builtins in this file).
#ifdef ARCH_ARM64
VReg gen_vector_binary_builtin(Node *node) {
#ifdef ARCH_ARM64
    (void)node;
    return R_NONE;
#else
    Node *arg0 = node->args;
    Node *arg1 = arg0 ? arg0->next : NULL;
    VReg a = gen_addr(arg0);
    VReg b = gen_addr(arg1);
    x86_movups_rm(cg_sec, X86_XMM0, x86_mem(REG(a), 0));
    free_reg(a);
    x86_movups_rm(cg_sec, X86_XMM1, x86_mem(REG(b), 0));
    free_reg(b);
    x86_pshufb(cg_sec, X86_XMM0, X86_XMM1); // xmm0 = pshufb(xmm0, xmm1)
    VReg dst = alloc_int128_addr();
    x86_movups_mr(cg_sec, x86_mem(REG(dst), 0), X86_XMM0);
    return dst;
#endif
}
#endif /* ARCH_ARM64 */


#ifndef ARCH_ARM64
// ===== __builtin_ia32_* SIMD intrinsic codegen (x86-64) =====
// The real GCC headers (xmmintrin.h/emmintrin.h/...) implement the
// intrinsics as thin inline wrappers over __builtin_ia32_* calls. Each
// call is typed by name (see type.c's ia32_builtin_ret); here we emit
// the matching packed-SSE instruction. Vector operands/results live in
// 8- or 16-byte stack slots (vectors are TY_STRUCT in this compiler).
static bool ia32_suf(const char *n, const char *s) {
    size_t nl = strlen(n), sl = strlen(s);
    return nl >= sl && memcmp(n + nl - sl, s, sl) == 0;
}
static bool ia32_is_const(Node *arg) {
    if (!arg) return false;
    while (arg->kind == ND_CAST && arg->lhs)
        arg = arg->lhs;
    return arg->kind == ND_NUM;
}
static uint8_t ia32_imm8(Node *arg, const char *what) {
    if (!arg)
        error("__builtin_ia32_*: missing %s operand", what);
    if (!ia32_is_const(arg)) {
        // _MM_SHUFFLE(a,b,c,d) macro arithmetic folds to a constant here;
        // only genuinely runtime values fall through to the error.
        long long v = 0;
        if (!eval_const_expr(arg, &v))
            error_tok(arg->tok, "__builtin_ia32_*: %s must be an integer constant", what);
        return (uint8_t)v;
    }
    while (arg->kind == ND_CAST && arg->lhs)
        arg = arg->lhs;
    return (uint8_t)arg->val;
}
// Address of a by-value vector operand (vector values live in slots).
VReg ia32_vaddr(Node *arg) {
    VReg r = gen_addr(arg);
    if (r < 0) {
        r = gen(arg);
    }
    return r;
}
// Load a 8/16-byte vector operand into XMM2/XMM1 and copy XMM2 to XMM0.
static void ia32_load2(Node *a, Node *b, int bytes) {
    VReg da = ia32_vaddr(a);
    if (bytes == 8)
        x86_movq_rm(cg_sec, x86_mem(REG(da), 0), X86_XMM2);
    else
        x86_movups_rm(cg_sec, X86_XMM2, x86_mem(REG(da), 0));
    free_reg(da);
    VReg db = ia32_vaddr(b);
    if (bytes == 8)
        x86_movq_rm(cg_sec, x86_mem(REG(db), 0), X86_XMM1);
    else
        x86_movups_rm(cg_sec, X86_XMM1, x86_mem(REG(db), 0));
    free_reg(db);
    x86_movaps(cg_sec, X86_XMM0, X86_XMM2); // dst = arg1 (upper lanes from arg1)
}
static void ia32_load1(Node *a, int bytes) {
    VReg da = ia32_vaddr(a);
    if (bytes == 8)
        x86_movq_rm(cg_sec, x86_mem(REG(da), 0), X86_XMM0);
    else
        x86_movups_rm(cg_sec, X86_XMM0, x86_mem(REG(da), 0));
    free_reg(da);
}
static VReg ia32_store(int bytes) {
    VReg dst = alloc_int128_addr();
    if (bytes == 8)
        x86_movq_mr(cg_sec, x86_mem(REG(dst), 0), X86_XMM0);
    else if (bytes == 32) {
        alloc_int128_slot(); // second half of the 32-byte slot
        x86_vmovups_mr256(cg_sec, x86_mem(REG(dst), 0), X86_XMM0);
    } else
        x86_movups_mr(cg_sec, x86_mem(REG(dst), 0), X86_XMM0);
    return dst;
}
// Emit prefix + 0F op /r (reg/reg) on XMM0, XMM1.
static void ia32_emit2(int pfx, int op) {
    switch (pfx) {
    case 0x66: sse_rr_66(cg_sec, (uint8_t)op, X86_XMM0, X86_XMM1); break;
    case 0xf3: sse_rr_f3(cg_sec, (uint8_t)op, X86_XMM0, X86_XMM1); break;
    case 0xf2: sse_rr_f2(cg_sec, (uint8_t)op, X86_XMM0, X86_XMM1); break;
    default: sse_rr_np(cg_sec, (uint8_t)op, X86_XMM0, X86_XMM1); break;
    }
}
// int ALU root -> 66 0F opcode (shared MMX/SSE; size selects load/store width)
static int ia32_int_op(const char *root) {
    static const struct {
        const char *r;
        int op;
    } tab[] = {
        {"paddb", 0xfc},
        {"paddw", 0xfd},
        {"paddd", 0xfe},
        {"paddq", 0xd4},
        {"psubb", 0xf8},
        {"psubw", 0xf9},
        {"psubd", 0xfa},
        {"psubq", 0xfb},
        {"paddsb", 0xec},
        {"paddsw", 0xed},
        {"paddusb", 0xdc},
        {"paddusw", 0xdd},
        {"psubsb", 0xe8},
        {"psubsw", 0xe9},
        {"psubusb", 0xd8},
        {"psubusw", 0xd9},
        {"pand", 0xdb},
        {"pandn", 0xdf},
        {"por", 0xeb},
        {"pxor", 0xef},
        {"pcmpeqb", 0x74},
        {"pcmpeqw", 0x75},
        {"pcmpeqd", 0x76},
        {"pcmpgtb", 0x64},
        {"pcmpgtw", 0x65},
        {"pcmpgtd", 0x66},
        {"pmullw", 0xd5},
        {"pmulhw", 0xe5},
        {"pmulhuw", 0xe4},
        {"pmuludq", 0xf4},
        {"pmaddwd", 0xf5},
        {"pavgb", 0xe0},
        {"pavgw", 0xe3},
        {"psadbw", 0xf6},
        {"pmaxub", 0xde},
        {"pmaxsw", 0xee},
        {"pminub", 0xda},
        {"pminsw", 0xea},
        {"packsswb", 0x63},
        {"packssdw", 0x6b},
        {"packuswb", 0x67},
        {"punpcklbw", 0x60},
        {"punpcklwd", 0x61},
        {"punpckldq", 0x62},
        {"punpcklqdq", 0x6c},
        {"punpckhbw", 0x68},
        {"punpckhwd", 0x69},
        {"punpckhdq", 0x6a},
        {"punpckhqdq", 0x6d},
    };
    for (size_t i = 0; i < sizeof(tab) / sizeof(tab[0]); i++)
        if (!strcmp(tab[i].r, root))
            return tab[i].op;
    return -1;
}
// 66 0F 38 xx /r two-register ops (SSSE3/SSE4.x)
static int ia32_int38_op(const char *root) {
    static const struct {
        const char *r;
        int op;
    } tab[] = {
        {"pabsb", 0x1c},
        {"pabsw", 0x1d},
        {"pabsd", 0x1e},
        {"psignb", 0x08},
        {"psignw", 0x09},
        {"psignd", 0x0a},
        {"phaddw", 0x01},
        {"phaddd", 0x02},
        {"phaddsw", 0x03},
        {"phsubw", 0x05},
        {"phsubd", 0x06},
        {"phsubsw", 0x07},
        {"pmaddubsw", 0x04},
        {"pmulhrsw", 0x0b},
        // pblendvb/blendvps/blendvpd are 3-operand (implicit XMM0 mask)
        // ops the generic 2-operand ia32_load2()+ia32_emit38() path below
        // can't express (it never loads the mask into XMM0) -- handled by
        // their own dedicated case further down instead. Do NOT list them
        // here: this table is scanned first, so a matching entry would
        // silently steal the name into the wrong-arity codegen path.
        {"ptest", 0x17},
        {"pmovsxbw", 0x20},
        {"pmovsxbd", 0x21},
        {"pmovsxbq", 0x22},
        {"pmovsxwd", 0x23},
        {"pmovsxwq", 0x24},
        {"pmovsxdq", 0x25},
        {"pmuldq", 0x28},
        {"pcmpeqq", 0x29},
        {"movntdqa", 0x2a},
        {"packusdw", 0x2b},
        {"pmovzxbw", 0x30},
        {"pmovzxbd", 0x31},
        {"pmovzxbq", 0x32},
        {"pmovzxwd", 0x33},
        {"pmovzxwq", 0x34},
        {"pmovzxdq", 0x35},
        {"pcmpgtq", 0x37},
        {"pminsb", 0x38},
        {"pminsd", 0x39},
        {"pminuw", 0x3a},
        {"pminud", 0x3b},
        {"pmaxsb", 0x3c},
        {"pmaxsd", 0x3d},
        {"pmaxuw", 0x3e},
        {"pmaxud", 0x3f},
        {"pmulld", 0x40},
        {"phminposuw", 0x41},
        {"pshufb", 0x00},
    };
    for (size_t i = 0; i < sizeof(tab) / sizeof(tab[0]); i++)
        if (!strcmp(tab[i].r, root))
            return tab[i].op;
    return -1;
}
// 66 0F 3A xx /r ib (SSE4.1 immediate ops)
static int ia32_int3a_op(const char *root) {
    static const struct {
        const char *r;
        int op;
    } tab[] = {
        {"roundps", 0x08},
        {"roundpd", 0x09},
        {"roundss", 0x0a},
        {"roundsd", 0x0b},
        {"blendps", 0x0c},
        {"blendpd", 0x0d},
        {"pblendw", 0x0e},
        {"palignr", 0x0f},
        {"insertps", 0x21},
        {"dpps", 0x40},
        {"dppd", 0x41},
        {"mpsadbw", 0x42},
        {"pclmulqdq", 0x44},
        {"pcmpestrm", 0x60},
        {"pcmpestri", 0x61},
        {"pcmpistrm", 0x62},
        {"pcmpistri", 0x63},
    };
    for (size_t i = 0; i < sizeof(tab) / sizeof(tab[0]); i++)
        if (!strcmp(tab[i].r, root))
            return tab[i].op;
    return -1;
}
static void ia32_emit38(int op) {
    emit1(cg_sec, 0x66);
    maybe_rex(cg_sec, 0, (int)X86_XMM0, 0, (int)X86_XMM1);
    emit3(cg_sec, 0x0f, 0x38, (uint8_t)op);
    emit1(cg_sec, modrxmm(3, X86_XMM0, X86_XMM1));
}
static void ia32_emit3a(int op, uint8_t imm) {
    emit1(cg_sec, 0x66);
    maybe_rex(cg_sec, 0, (int)X86_XMM0, 0, (int)X86_XMM1);
    emit3(cg_sec, 0x0f, 0x3a, (uint8_t)op);
    emit1(cg_sec, modrxmm(3, X86_XMM0, X86_XMM1));
    emit1(cg_sec, imm);
}

#ifndef ARCH_ARM64
// Load a 32-byte vector operand into YMM<y> (vectors live in slots).
void avx_loadY(X86XmmReg y, Node *a) {
    VReg va = ia32_vaddr(a);
    x86_vmovups_rm256(cg_sec, y, x86_mem(REG(va), 0));
    free_reg(va);
}
// Address of a fresh 32-byte slot (two 16-byte int128 slots).
VReg avx_slot_addr(void) {
    VReg dst = alloc_int128_addr();
    alloc_int128_slot(); // second half of the 32-byte slot
    return dst;
}
// Store YMM0 to a fresh 32-byte slot and return its address.
VReg avx_store(void) {
    VReg dst = avx_slot_addr();
    x86_vmovups_mr256(cg_sec, x86_mem(REG(dst), 0), X86_XMM0);
    return dst;
}
#endif

VReg gen_ia32_builtin(Node *node) {
    const char *fn = node->funcname;
    // rcc's own bundled SIMD headers declare the five legacy builtins
    // (sqrtps/sqrtss/rsqrtps/sqrtpd/sqrtsd) as real functions, so those
    // calls carry their target in lhs->var->name instead of funcname.
    if (!fn && node->lhs && node->lhs->var && node->lhs->var->name)
        fn = node->lhs->var->name;
    if (!fn) return R_NONE;
    const char *n = fn + 15; // "__builtin_ia32_"
    Node *a1 = node->args, *a2 = a1 ? a1->next : NULL, *a3 = a2 ? a2->next : NULL;

    // ================= no-result ops =================
    if (!strcmp(n, "pause")) {
        x86_pause(cg_sec);
        return R_NONE;
    }
    if (!strcmp(n, "sfence")) {
        x86_sfence(cg_sec);
        return R_NONE;
    }
    if (!strcmp(n, "lfence")) {
        x86_lfence(cg_sec);
        return R_NONE;
    }
    if (!strcmp(n, "mfence")) {
        x86_mfence(cg_sec);
        return R_NONE;
    }
    if (!strcmp(n, "emms")) {
        x86_emms(cg_sec);
        return R_NONE;
    }
    if (!strcmp(n, "femms")) {
        x86_femms(cg_sec);
        return R_NONE;
    }
    if (!strcmp(n, "monitor")) {
        VReg p = gen(a1), e = gen(a2), h = gen(a3);
        x86_mov_rr(cg_sec, 8, X86_RAX, REG(p)); // rax = addr
        x86_mov_rr(cg_sec, 8, X86_RCX, REG(e)); // rcx = ext
        x86_mov_rr(cg_sec, 8, X86_RDX, REG(h)); // rdx = hints
        x86_monitor(cg_sec);
        free_reg(p);
        free_reg(e);
        free_reg(h);
        return R_NONE;
    }
    if (!strcmp(n, "mwait")) {
        VReg e = gen(a1), h = gen(a2);
        x86_mov_rr(cg_sec, 4, X86_RAX, REG(e)); // eax = ext
        x86_mov_rr(cg_sec, 4, X86_RCX, REG(h)); // ecx = hints
        x86_mwait(cg_sec);
        free_reg(e);
        free_reg(h);
        return R_NONE;
    }
    if (!strcmp(n, "rdtsc")) {
        // RDTSC writes the 64-bit timestamp counter split across
        // EDX:EAX (each 32-bit write implicitly zero-extends to 64
        // bits on x86-64); __builtin_ia32_rdtsc() returns it combined
        // as a single `unsigned long long`.
        x86_rdtsc(cg_sec);
        VReg r = alloc_reg();
        x86_mov_rr(cg_sec, 4, REG(r), X86_RDX); // mov edx, r (zero-extends)
        x86_shl_ri(cg_sec, 8, REG(r), 32); // shl r, #32
        VReg lo = alloc_reg();
        x86_mov_rr(cg_sec, 4, REG(lo), X86_RAX); // mov eax, lo (zero-extends)
        x86_or_rr(cg_sec, 8, REG(r), REG(lo)); // or lo, r
        free_reg(lo);
        return r;
    }
    if (!strcmp(n, "rdtscp")) {
        // __builtin_ia32_rdtscp(unsigned int *aux): RDTSCP additionally
        // writes the TSC_AUX MSR value to ECX; store that through the
        // pointer argument, return the combined EDX:EAX counter exactly
        // like plain rdtsc.
        x86_rdtscp(cg_sec);
        VReg r = alloc_reg();
        x86_mov_rr(cg_sec, 4, REG(r), X86_RDX);
        x86_shl_ri(cg_sec, 8, REG(r), 32);
        VReg lo = alloc_reg();
        x86_mov_rr(cg_sec, 4, REG(lo), X86_RAX);
        x86_or_rr(cg_sec, 8, REG(r), REG(lo));
        free_reg(lo);
        if (a1) {
            VReg p = gen(a1);
            VReg aux = alloc_reg();
            x86_mov_rr(cg_sec, 4, REG(aux), X86_RCX);
            x86_mov_mr(cg_sec, 4, x86_mem(REG(p), 0), REG(aux));
            free_reg(aux);
            free_reg(p);
        }
        return r;
    }
    if (!strcmp(n, "clflush")) {
        VReg p = gen(a1);
        x86_clflush(cg_sec, x86_mem(REG(p), 0));
        free_reg(p);
        return R_NONE;
    }
    if (!strcmp(n, "prefetch")) {
        VReg p = gen(a1);
        X86Mem m = x86_mem(REG(p), 0);
        if (ia32_is_const(a2)) {
            switch (ia32_imm8(a2, "locality") & 3) {
            case 0: x86_prefetchnta(cg_sec, m); break;
            case 1: x86_prefetcht0(cg_sec, m); break;
            case 2: x86_prefetcht1(cg_sec, m); break;
            default: x86_prefetcht2(cg_sec, m); break;
            }
        } else {
            // Runtime locality (xmmintrin.h's _mm_prefetch passes
            // `(__I & 0xC) >> 2`): dispatch among the four forms.
            int c = ++rcc_label_count;
            VReg loc = gen(a2);
            asm_cmp_imm(cg_sec, loc, 4, 0);
            {
                size_t o = asm_jcc_label(cg_sec, X86_E);
                asm_fixup_add(cg_sec, o, format(".L.pf.nta.%d", c), 1);
            }
            asm_cmp_imm(cg_sec, loc, 4, 1);
            {
                size_t o = asm_jcc_label(cg_sec, X86_E);
                asm_fixup_add(cg_sec, o, format(".L.pf.t0.%d", c), 1);
            }
            asm_cmp_imm(cg_sec, loc, 4, 2);
            {
                size_t o = asm_jcc_label(cg_sec, X86_E);
                asm_fixup_add(cg_sec, o, format(".L.pf.t1.%d", c), 1);
            }
            x86_prefetcht2(cg_sec, m);
            {
                size_t o = asm_jmp_label(cg_sec);
                asm_fixup_add(cg_sec, o, format(".L.pf.end.%d", c), 0);
            }
            cg_def_label(format(".L.pf.nta.%d", c));
            x86_prefetchnta(cg_sec, m);
            {
                size_t o = asm_jmp_label(cg_sec);
                asm_fixup_add(cg_sec, o, format(".L.pf.end.%d", c), 0);
            }
            cg_def_label(format(".L.pf.t0.%d", c));
            x86_prefetcht0(cg_sec, m);
            {
                size_t o = asm_jmp_label(cg_sec);
                asm_fixup_add(cg_sec, o, format(".L.pf.end.%d", c), 0);
            }
            cg_def_label(format(".L.pf.t1.%d", c));
            x86_prefetcht1(cg_sec, m);
            cg_def_label(format(".L.pf.end.%d", c));
            free_reg(loc);
        }
        free_reg(p);
        return R_NONE;
    }
    if (!strcmp(n, "ldmxcsr")) {
        VReg r = gen(a1);
        asm_sub_rsp_imm(cg_sec, 16);
        x86_mov_mr(cg_sec, 4, x86_mem(X86_RSP, 0), REG(r));
        x86_ldmxcsr_m(cg_sec, x86_mem(X86_RSP, 0));
        asm_add_rsp_imm(cg_sec, 16);
        free_reg(r);
        return R_NONE;
    }
    if (!strcmp(n, "stmxcsr")) {
        asm_sub_rsp_imm(cg_sec, 16);
        x86_stmxcsr_m(cg_sec, x86_mem(X86_RSP, 0));
        VReg r = alloc_reg();
        x86_mov_rm(cg_sec, 4, REG(r), x86_mem(X86_RSP, 0));
        asm_add_rsp_imm(cg_sec, 16);
        return r;
    }
    if (!strcmp(n, "movnti") || !strcmp(n, "movnti64")) {
        VReg p = gen(a1), v = gen(a2);
        x86_movnti_m(cg_sec, x86_mem(REG(p), 0), REG(v), !strcmp(n, "movnti") ? 4 : 8);
        free_reg(p);
        free_reg(v);
        return R_NONE;
    }
    if (!strcmp(n, "movntps") || !strcmp(n, "movntpd") || !strcmp(n, "movntdq")) {
        VReg p = gen(a1);
        VReg va = ia32_vaddr(a2);
        x86_movups_rm(cg_sec, X86_XMM0, x86_mem(REG(va), 0));
        free_reg(va);
        if (!strcmp(n, "movntps")) x86_movntps_m(cg_sec, x86_mem(REG(p), 0), X86_XMM0);
        else if (!strcmp(n, "movntpd"))
            x86_movntpd_m(cg_sec, x86_mem(REG(p), 0), X86_XMM0);
        else
            x86_movntdq_m(cg_sec, x86_mem(REG(p), 0), X86_XMM0);
        free_reg(p);
        return R_NONE;
    }
    if (!strcmp(n, "movntq")) {
        VReg p = gen(a1);
        VReg va = ia32_vaddr(a2);
        x86_movq_rm(cg_sec, x86_mem(REG(va), 0), X86_XMM0);
        free_reg(va);
        x86_movntq_m(cg_sec, x86_mem(REG(p), 0), X86_XMM0);
        free_reg(p);
        return R_NONE;
    }
    if (!strcmp(n, "maskmovdqu")) {
        VReg da = ia32_vaddr(a1);
        VReg db = ia32_vaddr(a2);
        x86_movups_rm(cg_sec, X86_XMM1, x86_mem(REG(da), 0));
        free_reg(da);
        x86_movups_rm(cg_sec, X86_XMM2, x86_mem(REG(db), 0));
        free_reg(db);
        x86_maskmovdqu(cg_sec, X86_XMM1, X86_XMM2); // writes to [rdi]
        return R_NONE;
    }
    // storehps/storelps: store high/low 64 bits of the vector to *ptr
    if (!strcmp(n, "storehps") || !strcmp(n, "storelps")) {
        VReg p = gen(a1);
        VReg va = ia32_vaddr(a2);
        x86_movups_rm(cg_sec, X86_XMM0, x86_mem(REG(va), 0));
        free_reg(va);
        if (!strcmp(n, "storehps"))
            x86_pshufd(cg_sec, X86_XMM0, X86_XMM0, 0xee); // high dwords -> low
        x86_movq_mr(cg_sec, x86_mem(REG(p), 0), X86_XMM0);
        free_reg(p);
        return R_NONE;
    }
    // loadhps/loadlps: A with high/low 64 bits replaced by *ptr
    if (!strcmp(n, "loadhps") || !strcmp(n, "loadlps")) {
        VReg va = ia32_vaddr(a1);
        x86_movups_rm(cg_sec, X86_XMM0, x86_mem(REG(va), 0));
        free_reg(va);
        VReg p = gen(a2);
        x86_movq_rm(cg_sec, x86_mem(REG(p), 0), X86_XMM1);
        free_reg(p);
        if (!strcmp(n, "loadhps"))
            x86_movlhps(cg_sec, X86_XMM0, X86_XMM1);
        else
            x86_movsd_rr(cg_sec, X86_XMM0, X86_XMM1);
        return ia32_store(16);
    }
    if (!strcmp(n, "movss") || !strcmp(n, "movsd")) {
        // movss/movsd: A with lane 0 replaced by B's lane 0
        ia32_load2(a1, a2, 16);
        if (!strcmp(n, "movss")) x86_movss_rr(cg_sec, X86_XMM0, X86_XMM1);
        else
            x86_movsd_rr(cg_sec, X86_XMM0, X86_XMM1);
        return ia32_store(16);
    }
    if (!strcmp(n, "movhlps") || !strcmp(n, "movlhps")) {
        ia32_load2(a1, a2, 16);
        if (!strcmp(n, "movhlps")) x86_movhlps(cg_sec, X86_XMM0, X86_XMM1);
        else
            x86_movlhps(cg_sec, X86_XMM0, X86_XMM1);
        return ia32_store(16);
    }
    if (!strcmp(n, "movsldup") || !strcmp(n, "movshdup")) {
        ia32_load1(a1, 16);
        if (!strcmp(n, "movsldup")) x86_movsldup(cg_sec, X86_XMM0, X86_XMM0);
        else
            x86_movshdup(cg_sec, X86_XMM0, X86_XMM0);
        return ia32_store(16);
    }
    if (!strcmp(n, "movq128")) {
        // movq xmm, xmm: low 64 bits, upper zero
        ia32_load1(a1, 8); // movq load zero-extends
        return ia32_store(16);
    }
    if (!strcmp(n, "movmskps") || !strcmp(n, "movmskpd")) {
        VReg va = ia32_vaddr(a1);
        x86_movups_rm(cg_sec, X86_XMM0, x86_mem(REG(va), 0));
        free_reg(va);
        VReg r = alloc_reg();
        if (!strcmp(n, "movmskps")) x86_movmskps(cg_sec, REG(r), X86_XMM0);
        else
            x86_movmskpd(cg_sec, REG(r), X86_XMM0);
        return r;
    }
    if (!strcmp(n, "pmovmskb") || !strcmp(n, "pmovmskb128")) {
        VReg va = ia32_vaddr(a1);
        int bytes = !strcmp(n, "pmovmskb") ? 8 : 16;
        if (bytes == 8) x86_movq_rm(cg_sec, x86_mem(REG(va), 0), X86_XMM0);
        else
            x86_movups_rm(cg_sec, X86_XMM0, x86_mem(REG(va), 0));
        free_reg(va);
        VReg r = alloc_reg();
        x86_pmovmskb(cg_sec, (X86XmmReg)REG(r), X86_XMM0); // GP dst in reg field
        return r;
    }
    // ================= F16C half-precision converts =================
    // VEX-only (no legacy SSE encoding exists), and the 128<->256
    // pairs are asymmetric: vcvtph2ps256 takes a 128-bit half-vector
    // source but produces a 256-bit float result, vcvtps2ph256 takes a
    // 256-bit float source but produces a 128-bit half-vector result
    // (the destination is always <=128 bits, per the ISA), so these
    // can't reuse ia32_load2's matched-width convention and are
    // dispatched here on the exact (un-suffix-stripped) name instead.
    if (!strcmp(n, "vcvtph2ps")) {
        ia32_load1(a1, 16);
        x86_vcvtph2ps(cg_sec, X86_XMM0, X86_XMM0);
        return ia32_store(16);
    }
    if (!strcmp(n, "vcvtph2ps256")) {
        ia32_load1(a1, 16); // source is a 128-bit v8hi (8 packed halfs)
        x86_vcvtph2ps256(cg_sec, X86_XMM0, X86_XMM0);
        return ia32_store(32); // result is a 256-bit v8sf
    }
    if (!strcmp(n, "vcvtps2ph")) {
        ia32_load1(a1, 16);
        x86_vcvtps2ph(cg_sec, X86_XMM0, X86_XMM0, ia32_imm8(a2, "imm"));
        return ia32_store(16);
    }
    if (!strcmp(n, "vcvtps2ph256")) {
        avx_loadY(X86_XMM1, a1); // source is a 256-bit v8sf
        x86_vcvtps2ph256(cg_sec, X86_XMM0, X86_XMM1, ia32_imm8(a2, "imm"));
        return ia32_store(16); // result is a 128-bit v8hi
    }


    // ================= AVX/AVX2: 256-bit names =================
    // All AVX/AVX2 intrinsics are `__builtin_ia32_<name>256`; the family
    // letter (ps/pd/b/w/d/q) stays in the root after stripping "256".
    // Fixed YMM2/YMM1 sources, YMM0 result (like the XMM path); YMM3 for
    // the blendv mask.
#ifndef ARCH_ARM64
    if (ia32_suf(n, "256")) {
        char root[64];
        size_t rl = strlen(n) - 3;
        memcpy(root, n, rl);
        root[rl] = 0;
        // int/void results and memory forms first (no vector result)
        if (!strcmp(root, "movmskps") || !strcmp(root, "movmskpd")) {
            avx_loadY(X86_XMM1, a1);
            VReg r = alloc_reg();
            if (!strcmp(root, "movmskps")) x86_vmovmskps(cg_sec, (X86XmmReg)REG(r), X86_XMM1);
            else
                x86_vmovmskpd(cg_sec, (X86XmmReg)REG(r), X86_XMM1);
            return r;
        }
        if (!strcmp(root, "pmovmskb")) {
            avx_loadY(X86_XMM1, a1);
            VReg r = alloc_reg();
            x86_vpmovmskb256(cg_sec, (X86XmmReg)REG(r), X86_XMM1);
            return r;
        }
        if (!strcmp(root, "ptestz") || !strcmp(root, "ptestc") || !strcmp(root, "ptestnzc") ||
            !strcmp(root, "vtestzps") || !strcmp(root, "vtestcps") || !strcmp(root, "vtestnzcps") ||
            !strcmp(root, "vtestzpd") || !strcmp(root, "vtestcpd") || !strcmp(root, "vtestnzcpd")) {
            avx_loadY(X86_XMM1, a1);
            avx_loadY(X86_XMM2, a2);
            if (!strncmp(root, "ptest", 5)) x86_vptest(cg_sec, X86_XMM1, X86_XMM2);
            else if (strstr(root, "ps"))
                x86_vtestps(cg_sec, X86_XMM1, X86_XMM2);
            else
                x86_vtestpd(cg_sec, X86_XMM1, X86_XMM2);
            VReg r = alloc_reg();
            bool isz = root[strlen(root) - 1] == 'z';
            bool isc = root[strlen(root) - 1] == 'c';
            if (isz) {
                asm_setcc(cg_sec, X86_RAX, X86_E);
                asm_movzx_phys(cg_sec, r, X86_RAX, 4, 1);
            } else if (isc) {
                asm_setcc(cg_sec, X86_RAX, X86_B);
                asm_movzx_phys(cg_sec, r, X86_RAX, 4, 1);
            } else {
                asm_setcc(cg_sec, X86_RAX, X86_E);
                asm_setcc(cg_sec, X86_RCX, X86_B);
                x86_or_rr(cg_sec, 1, X86_RAX, X86_RCX);
                x86_xor_ri(cg_sec, 1, X86_RAX, 1);
                asm_movzx_phys(cg_sec, r, X86_RAX, 4, 1);
            }
            return r;
        }
        // non-temporal stores: movntps/movntpd/movntdq (ptr, vec)
        if (!strcmp(root, "movntps") || !strcmp(root, "movntpd") || !strcmp(root, "movntdq")) {
            VReg p = gen(a1);
            avx_loadY(X86_XMM0, a2);
            if (!strcmp(root, "movntps")) x86_vmovntps_m256(cg_sec, x86_mem(REG(p), 0), X86_XMM0);
            else if (!strcmp(root, "movntpd"))
                x86_vmovntpd_m256(cg_sec, x86_mem(REG(p), 0), X86_XMM0);
            else
                x86_vmovntdq_m256(cg_sec, x86_mem(REG(p), 0), X86_XMM0);
            free_reg(p);
            return R_NONE;
        }
        // masked stores: maskstoreps/pd (ptr, data, mask), maskstored/q
        if (!strcmp(root, "maskstoreps") || !strcmp(root, "maskstorepd") ||
            !strcmp(root, "maskstored") || !strcmp(root, "maskstoreq")) {
            VReg p = gen(a1);
            avx_loadY(X86_XMM0, a2); // data
            avx_loadY(X86_XMM1, a3); // mask
            if (ia32_suf(root, "q")) x86_vpmaskmovq_mr(cg_sec, x86_mem(REG(p), 0), X86_XMM0, X86_XMM1);
            else
                x86_vpmaskmovd_mr(cg_sec, x86_mem(REG(p), 0), X86_XMM0, X86_XMM1);
            free_reg(p);
            return R_NONE;
        }
        // masked loads: maskloadps/pd (ptr, mask), maskloadd/q
        if (!strcmp(root, "maskloadps") || !strcmp(root, "maskloadpd") ||
            !strcmp(root, "maskloadd") || !strcmp(root, "maskloadq")) {
            VReg p = gen(a1);
            avx_loadY(X86_XMM1, a2); // mask
            if (ia32_suf(root, "q")) x86_vpmaskmovq_rm(cg_sec, X86_XMM0, X86_XMM1, x86_mem(REG(p), 0));
            else
                x86_vpmaskmovd_rm(cg_sec, X86_XMM0, X86_XMM1, x86_mem(REG(p), 0));
            free_reg(p);
            return avx_store();
        }
        // 2-op unary converts
        if (!strcmp(root, "cvtdq2ps") || !strcmp(root, "cvtps2dq") ||
            !strcmp(root, "cvttps2dq") || !strcmp(root, "cvtps2pd") ||
            !strcmp(root, "cvtpd2ps") || !strcmp(root, "cvtdq2pd") ||
            !strcmp(root, "cvtpd2dq") || !strcmp(root, "cvttpd2dq")) {
            avx_loadY(X86_XMM1, a1);
            if (!strcmp(root, "cvtdq2ps")) x86_vcvtdq2ps(cg_sec, X86_XMM0, X86_XMM1);
            else if (!strcmp(root, "cvtps2dq"))
                x86_vcvtps2dq(cg_sec, X86_XMM0, X86_XMM1);
            else if (!strcmp(root, "cvttps2dq"))
                x86_vcvttps2dq(cg_sec, X86_XMM0, X86_XMM1);
            else if (!strcmp(root, "cvtps2pd"))
                x86_vcvtps2pd(cg_sec, X86_XMM0, X86_XMM1);
            else if (!strcmp(root, "cvtpd2ps"))
                x86_vcvtpd2ps(cg_sec, X86_XMM0, X86_XMM1);
            else if (!strcmp(root, "cvtdq2pd"))
                x86_vcvtdq2pd(cg_sec, X86_XMM0, X86_XMM1);
            else if (!strcmp(root, "cvtpd2dq"))
                x86_vcvtpd2dq(cg_sec, X86_XMM0, X86_XMM1);
            else
                x86_vcvttpd2dq(cg_sec, X86_XMM0, X86_XMM1);
            return avx_store();
        }
        // pmovsx*/pmovzx*: sign/zero-extend from XMM (8/16 source bytes) to YMM
        if (!strncmp(root, "pmovsx", 6) || !strncmp(root, "pmovzx", 6)) {
            VReg va = ia32_vaddr(a1);
            x86_movups_rm(cg_sec, X86_XMM1, x86_mem(REG(va), 0)); // low 128 bits
            free_reg(va);
            if (!strcmp(root, "pmovsxbw")) x86_vpmovsxbw(cg_sec, X86_XMM0, X86_XMM1);
            else if (!strcmp(root, "pmovsxbd"))
                x86_vpmovsxbd(cg_sec, X86_XMM0, X86_XMM1);
            else if (!strcmp(root, "pmovsxbq"))
                x86_vpmovsxbq(cg_sec, X86_XMM0, X86_XMM1);
            else if (!strcmp(root, "pmovsxwd"))
                x86_vpmovsxwd(cg_sec, X86_XMM0, X86_XMM1);
            else if (!strcmp(root, "pmovsxwq"))
                x86_vpmovsxwq(cg_sec, X86_XMM0, X86_XMM1);
            else if (!strcmp(root, "pmovsxdq"))
                x86_vpmovsxdq(cg_sec, X86_XMM0, X86_XMM1);
            else if (!strcmp(root, "pmovzxbw"))
                x86_vpmovzxbw(cg_sec, X86_XMM0, X86_XMM1);
            else if (!strcmp(root, "pmovzxbd"))
                x86_vpmovzxbd(cg_sec, X86_XMM0, X86_XMM1);
            else if (!strcmp(root, "pmovzxbq"))
                x86_vpmovzxbq(cg_sec, X86_XMM0, X86_XMM1);
            else if (!strcmp(root, "pmovzxwd"))
                x86_vpmovzxwd(cg_sec, X86_XMM0, X86_XMM1);
            else if (!strcmp(root, "pmovzxwq"))
                x86_vpmovzxwq(cg_sec, X86_XMM0, X86_XMM1);
            else
                x86_vpmovzxdq(cg_sec, X86_XMM0, X86_XMM1);
            return avx_store();
        }
        // lddqu / movntdqa / vbroadcastf128: memory loads
        if (!strcmp(root, "lddqu")) {
            VReg p = gen(a1);
            x86_vlddqu256(cg_sec, X86_XMM0, x86_mem(REG(p), 0));
            free_reg(p);
            return avx_store();
        }
        if (!strcmp(root, "movntdqa")) {
            VReg p = gen(a1);
            x86_vmovntdqa256(cg_sec, X86_XMM0, x86_mem(REG(p), 0));
            free_reg(p);
            return avx_store();
        }
        if (!strcmp(root, "vbroadcastf128_pd") || !strcmp(root, "vbroadcastf128_ps")) {
            VReg p = gen(a1);
            x86_vbroadcastf128(cg_sec, X86_XMM0, x86_mem(REG(p), 0));
            free_reg(p);
            return avx_store();
        }
        // broadcasts: vbroadcastss/sd, pbroadcastb/w/d/q, vbroadcastsi256
        // (arg is a 16-byte vector; low element broadcast to all 8/4 lanes)
        if (!strcmp(root, "vbroadcastss") || !strcmp(root, "vbroadcastss_ps") ||
            !strcmp(root, "pbroadcastd") || !strcmp(root, "vbroadcastsd") ||
            !strcmp(root, "vbroadcastsd_pd") || !strcmp(root, "pbroadcastb") ||
            !strcmp(root, "pbroadcastw") || !strcmp(root, "pbroadcastq") ||
            !strcmp(root, "vbroadcastsi")) {
            VReg va = ia32_vaddr(a1);
            x86_movups_rm(cg_sec, X86_XMM0, x86_mem(REG(va), 0));
            free_reg(va);
            if (!strcmp(root, "vbroadcastss") || !strcmp(root, "vbroadcastss_ps"))
                x86_vbroadcastss(cg_sec, X86_XMM2, X86_XMM0, X86_XMM0);
            else if (!strcmp(root, "vbroadcastsd") || !strcmp(root, "vbroadcastsd_pd") || !strcmp(root, "vbroadcastsi"))
                x86_vbroadcastsd(cg_sec, X86_XMM2, X86_XMM0, X86_XMM0);
            else if (!strcmp(root, "pbroadcastb"))
                x86_vpbroadcastb(cg_sec, X86_XMM2, X86_XMM0, X86_XMM0);
            else if (!strcmp(root, "pbroadcastw"))
                x86_vpbroadcastw(cg_sec, X86_XMM2, X86_XMM0, X86_XMM0);
            else if (!strcmp(root, "pbroadcastq"))
                x86_vpbroadcastq(cg_sec, X86_XMM2, X86_XMM0, X86_XMM0);
            else
                x86_vpbroadcastd(cg_sec, X86_XMM2, X86_XMM0, X86_XMM0);
            VReg dst = avx_slot_addr();
            x86_vmovups_mr256(cg_sec, x86_mem(REG(dst), 0), X86_XMM2);
            return dst;
        }
        // 256->128 casts: ps_ps/pd_pd/si_si (truncation of the low half)
        if (!strcmp(root, "ps_ps") || !strcmp(root, "pd_pd") || !strcmp(root, "si_si")) {
            VReg va = ia32_vaddr(a1);
            x86_movups_rm(cg_sec, X86_XMM0, x86_mem(REG(va), 0)); // low 128 bits
            free_reg(va);
            return ia32_store(16);
        }
        // extract 128 from 256: extract128i / vextractf128 (16-byte result)
        if (!strcmp(root, "extract128i") || !strcmp(root, "vextractf128_pd") ||
            !strcmp(root, "vextractf128_ps") || !strcmp(root, "vextractf128_si")) {
            // vextractf128: ps=1C, pd=19, si/i128=39
            avx_loadY(X86_XMM1, a1);
            if (!strcmp(root, "extract128i") || !strcmp(root, "vextractf128_si"))
                x86_vextracti128(cg_sec, X86_XMM0, X86_XMM1, ia32_imm8(a2, "imm"));
            else if (!strcmp(root, "vextractf128_pd"))
                x86_vextractf128_pd(cg_sec, X86_XMM0, X86_XMM1, ia32_imm8(a2, "imm"));
            else
                x86_vextractf128(cg_sec, X86_XMM0, X86_XMM1, ia32_imm8(a2, "imm"));
            return ia32_store(16);
        }
        // insert 128 into 256: insert128i / vinsertf128 (32-byte result)
        if (!strcmp(root, "insert128i") || !strcmp(root, "vinsertf128_pd") ||
            !strcmp(root, "vinsertf128_ps") || !strcmp(root, "vinsertf128_si")) {
            // vinsertf128: ps=18, pd=19, si/i128=38
            avx_loadY(X86_XMM1, a1);
            VReg vb = ia32_vaddr(a2);
            x86_movups_rm(cg_sec, X86_XMM2, x86_mem(REG(vb), 0)); // 16-byte source
            free_reg(vb);
            if (!strcmp(root, "insert128i") || !strcmp(root, "vinsertf128_si"))
                x86_vinserti128(cg_sec, X86_XMM0, X86_XMM1, X86_XMM2, ia32_imm8(a3, "imm"));
            else if (!strcmp(root, "vinsertf128_pd"))
                x86_vinsertf128_pd(cg_sec, X86_XMM0, X86_XMM1, X86_XMM2, ia32_imm8(a3, "imm"));
            else
                x86_vinsertf128(cg_sec, X86_XMM0, X86_XMM1, X86_XMM2, ia32_imm8(a3, "imm"));
            return avx_store();
        }
        // unary 2-op permutes/moves: movddup/movshdup/movsldup,
        // vpermq/vpermpd (imm), vpermilps/pd (imm)
        if (!strcmp(root, "movddup") || !strcmp(root, "movshdup") || !strcmp(root, "movsldup")) {
            avx_loadY(X86_XMM1, a1);
            if (!strcmp(root, "movddup")) x86_vmovddup(cg_sec, X86_XMM0, X86_XMM1);
            else if (!strcmp(root, "movshdup"))
                x86_vmovshdup(cg_sec, X86_XMM0, X86_XMM1);
            else
                x86_vmovsldup(cg_sec, X86_XMM0, X86_XMM1);
            return avx_store();
        }
        if (!strcmp(root, "permdi") || !strcmp(root, "permdf") ||
            !strcmp(root, "vpermilps") || !strcmp(root, "vpermilpd")) {
            avx_loadY(X86_XMM1, a1);
            if (!strcmp(root, "permdi")) x86_vpermq(cg_sec, X86_XMM0, X86_XMM1, ia32_imm8(a2, "imm"));
            else if (!strcmp(root, "permdf"))
                x86_vpermpd(cg_sec, X86_XMM0, X86_XMM1, ia32_imm8(a2, "imm"));
            else if (!strcmp(root, "vpermilps"))
                x86_vpermilps_i(cg_sec, X86_XMM0, X86_XMM1, ia32_imm8(a2, "imm"));
            else
                x86_vpermilpd_i(cg_sec, X86_XMM0, X86_XMM1, ia32_imm8(a2, "imm"));
            return avx_store();
        }
        // 3-op permutes: permvarsi (vpermd), permvarsf (vpermps),
        // permti (vperm2i128), vpermilvarps/vpermilvarpd
        if (!strcmp(root, "permvarsi") || !strcmp(root, "permvarsf") ||
            !strcmp(root, "permti") || !strcmp(root, "vperm2f128_si") ||
            !strcmp(root, "vpermilvarps") || !strcmp(root, "vpermilvarpd")) {
            // vpermd/vpermps: dst=reg, INDICES=vvvv, table=rm — the builtin
            // is (table, indices), so a2 goes to vvvv and a1 to rm.
            if (!strcmp(root, "permvarsi") || !strcmp(root, "permvarsf")) {
                avx_loadY(X86_XMM1, a2); // indices
                avx_loadY(X86_XMM2, a1); // table
                if (!strcmp(root, "permvarsi")) x86_vpermd(cg_sec, X86_XMM0, X86_XMM1, X86_XMM2);
                else
                    x86_vpermps(cg_sec, X86_XMM0, X86_XMM1, X86_XMM2);
                return avx_store();
            }
            // vpermilvarps/vpermilvarpd: dst=reg, src=vvvv, indices=rm
            avx_loadY(X86_XMM1, a1);
            avx_loadY(X86_XMM2, a2);
            if (!strcmp(root, "permti") || !strcmp(root, "vperm2f128_si"))
                x86_vperm2i128(cg_sec, X86_XMM0, X86_XMM1, X86_XMM2, ia32_imm8(a3, "imm"));
            else if (!strcmp(root, "vpermilvarps"))
                x86_vpermilps(cg_sec, X86_XMM0, X86_XMM1, X86_XMM2);
            else
                x86_vpermilpd(cg_sec, X86_XMM0, X86_XMM1, X86_XMM2);
            return avx_store();
        }
        // unary 3-op imm shuffles: pshufd/pshufhw/pshuflw
        if (!strcmp(root, "pshufd") || !strcmp(root, "pshufhw") || !strcmp(root, "pshuflw")) {
            avx_loadY(X86_XMM1, a1);
            if (!strcmp(root, "pshufd")) x86_vpshufd(cg_sec, X86_XMM0, X86_XMM0, X86_XMM1, ia32_imm8(a2, "imm"));
            else if (!strcmp(root, "pshufhw"))
                x86_vpshufhw(cg_sec, X86_XMM0, X86_XMM0, X86_XMM1, ia32_imm8(a2, "imm"));
            else
                x86_vpshuflw(cg_sec, X86_XMM0, X86_XMM0, X86_XMM1, ia32_imm8(a2, "imm"));
            return avx_store();
        }
        // float compares with imm: cmpps/cmppd
        if (!strcmp(root, "cmpps") || !strcmp(root, "cmppd")) {
            avx_loadY(X86_XMM1, a1);
            avx_loadY(X86_XMM2, a2);
            if (!strcmp(root, "cmpps")) x86_vcmpps(cg_sec, X86_XMM0, X86_XMM1, X86_XMM2, ia32_imm8(a3, "imm"));
            else
                x86_vcmppd(cg_sec, X86_XMM0, X86_XMM1, X86_XMM2, ia32_imm8(a3, "imm"));
            return avx_store();
        }
        // roundps/roundpd/dpps/mpsadbw: 3-op imm
        if (!strcmp(root, "roundps") || !strcmp(root, "roundpd") ||
            !strcmp(root, "dpps") || !strcmp(root, "mpsadbw")) {
            // roundps/roundpd take (vec, imm); dpps/mpsadbw take (a, b, imm)
            bool unary_imm = !strcmp(root, "roundps") || !strcmp(root, "roundpd");
            avx_loadY(X86_XMM1, a1);
            if (!unary_imm) avx_loadY(X86_XMM2, a2);
            if (!strcmp(root, "roundps")) x86_vroundps(cg_sec, X86_XMM0, X86_XMM0, X86_XMM1, ia32_imm8(a2, "imm"));
            else if (!strcmp(root, "roundpd"))
                x86_vroundpd(cg_sec, X86_XMM0, X86_XMM0, X86_XMM1, ia32_imm8(a2, "imm"));
            else if (!strcmp(root, "dpps"))
                x86_vdpps(cg_sec, X86_XMM0, X86_XMM1, X86_XMM2, ia32_imm8(a3, "imm"));
            else
                x86_vmpsadbw(cg_sec, X86_XMM0, X86_XMM1, X86_XMM2, ia32_imm8(a3, "imm"));
            return avx_store();
        }
        // blendps/blendpd/pblendw/pblendd: 3-op imm
        if (!strcmp(root, "blendps") || !strcmp(root, "blendpd") ||
            !strcmp(root, "pblendw") || !strcmp(root, "pblendd")) {
            avx_loadY(X86_XMM1, a1);
            avx_loadY(X86_XMM2, a2);
            if (!strcmp(root, "blendps")) x86_vblendps(cg_sec, X86_XMM0, X86_XMM1, X86_XMM2, ia32_imm8(a3, "imm"));
            else if (!strcmp(root, "blendpd"))
                x86_vblendpd(cg_sec, X86_XMM0, X86_XMM1, X86_XMM2, ia32_imm8(a3, "imm"));
            else if (!strcmp(root, "pblendw"))
                x86_vpblendw(cg_sec, X86_XMM0, X86_XMM1, X86_XMM2, ia32_imm8(a3, "imm"));
            else
                x86_vpblendd(cg_sec, X86_XMM0, X86_XMM1, X86_XMM2, ia32_imm8(a3, "imm"));
            return avx_store();
        }
        // blendvps/blendvpd/pblendvb: mask in YMM3 (is4 field)
        if (!strcmp(root, "blendvps") || !strcmp(root, "blendvpd") || !strcmp(root, "pblendvb")) {
            avx_loadY(X86_XMM1, a1);
            avx_loadY(X86_XMM2, a2);
            avx_loadY(X86_XMM3, a3); // mask
            if (!strcmp(root, "blendvps")) x86_vblendvps(cg_sec, X86_XMM0, X86_XMM1, X86_XMM2, X86_XMM3);
            else if (!strcmp(root, "blendvpd"))
                x86_vblendvpd(cg_sec, X86_XMM0, X86_XMM1, X86_XMM2, X86_XMM3);
            else
                x86_vpblendvb(cg_sec, X86_XMM0, X86_XMM1, X86_XMM2, X86_XMM3);
            return avx_store();
        }
        // palignr/pshufb: 3-op (imm for palignr)
        if (!strcmp(root, "palignr")) {
            // the builtin's imm is in BITS (the header passes __N*8);
            // the instruction shifts by bytes.
            avx_loadY(X86_XMM1, a1);
            avx_loadY(X86_XMM2, a2);
            x86_vpalignr(cg_sec, X86_XMM0, X86_XMM1, X86_XMM2, ia32_imm8(a3, "imm") >> 3);
            return avx_store();
        }
        if (!strcmp(root, "pshufb")) {
            avx_loadY(X86_XMM1, a1);
            avx_loadY(X86_XMM2, a2);
            x86_vpshufb(cg_sec, X86_XMM0, X86_XMM1, X86_XMM2);
            return avx_store();
        }
        // shifts: pslldi/psllwi/psllqi/psrldi/psrlwi/psrlqi/psradi/psrawi
        // (imm), pslldqi/psrldqi (imm byte), plain names (variable count)
        {
            size_t sl = strlen(root);
            if (!strncmp(root, "psll", 4) || !strncmp(root, "psrl", 4) || !strncmp(root, "psra", 4)) {
                bool imm_form = root[sl - 1] == 'i';
                char base[12];
                size_t bl = sl - (imm_form ? 1 : 0);
                memcpy(base, root, bl);
                base[bl] = 0;
                if (!strcmp(base, "pslldq") || !strcmp(base, "psrldq")) {
                    // byte shift by imm8
                    avx_loadY(X86_XMM0, a1);
                    if (!strcmp(base, "pslldq")) x86_vpslldq_i(cg_sec, X86_XMM0, ia32_imm8(a2, "imm"));
                    else
                        x86_vpsrldq_i(cg_sec, X86_XMM0, ia32_imm8(a2, "imm"));
                    return avx_store();
                }
                if (imm_form && ia32_is_const(a2)) {
                    avx_loadY(X86_XMM0, a1);
                    if (!strcmp(base, "psllw")) x86_vpsllw_i(cg_sec, X86_XMM0, ia32_imm8(a2, "imm"));
                    else if (!strcmp(base, "pslld"))
                        x86_vpslld_i(cg_sec, X86_XMM0, ia32_imm8(a2, "imm"));
                    else if (!strcmp(base, "psllq"))
                        x86_vpsllq_i(cg_sec, X86_XMM0, ia32_imm8(a2, "imm"));
                    else if (!strcmp(base, "psrlw"))
                        x86_vpsrlw_i(cg_sec, X86_XMM0, ia32_imm8(a2, "imm"));
                    else if (!strcmp(base, "psrld"))
                        x86_vpsrld_i(cg_sec, X86_XMM0, ia32_imm8(a2, "imm"));
                    else if (!strcmp(base, "psrlq"))
                        x86_vpsrlq_i(cg_sec, X86_XMM0, ia32_imm8(a2, "imm"));
                    else if (!strcmp(base, "psraw"))
                        x86_vpsraw_i(cg_sec, X86_XMM0, ia32_imm8(a2, "imm"));
                    else
                        x86_vpsrad_i(cg_sec, X86_XMM0, ia32_imm8(a2, "imm"));
                    return avx_store();
                }
                // variable count: count in XMM2 (low 64 bits), shift YMM0 in place
                avx_loadY(X86_XMM0, a1);
                VReg cnt = gen(a2);
                asm_sub_rsp_imm(cg_sec, 16);
                x86_mov_mr(cg_sec, 8, x86_mem(X86_RSP, 0), REG(cnt));
                free_reg(cnt);
                x86_movq_rm(cg_sec, x86_mem(X86_RSP, 0), X86_XMM2);
                asm_add_rsp_imm(cg_sec, 16);
                if (!strcmp(base, "psllw")) x86_vpsllw_r(cg_sec, X86_XMM0, X86_XMM0, X86_XMM2);
                else if (!strcmp(base, "pslld"))
                    x86_vpslld_r(cg_sec, X86_XMM0, X86_XMM0, X86_XMM2);
                else if (!strcmp(base, "psllq"))
                    x86_vpsllq_r(cg_sec, X86_XMM0, X86_XMM0, X86_XMM2);
                else if (!strcmp(base, "psrlw"))
                    x86_vpsrlw_r(cg_sec, X86_XMM0, X86_XMM0, X86_XMM2);
                else if (!strcmp(base, "psrld"))
                    x86_vpsrld_r(cg_sec, X86_XMM0, X86_XMM0, X86_XMM2);
                else if (!strcmp(base, "psrlq"))
                    x86_vpsrlq_r(cg_sec, X86_XMM0, X86_XMM0, X86_XMM2);
                else if (!strcmp(base, "psraw"))
                    x86_vpsraw_r(cg_sec, X86_XMM0, X86_XMM0, X86_XMM2);
                else
                    x86_vpsrad_r(cg_sec, X86_XMM0, X86_XMM0, X86_XMM2);
                return avx_store();
            }
        }
        // int ALU: 0F map (pp=1) — paddb..packuswb, pcmpeq*, pcmpgt*, ...
        {
            static const struct {
                const char *r;
                void (*fn)(SecBuf *, X86XmmReg, X86XmmReg, X86XmmReg);
            } tab0f[] = {
                {"paddb", x86_vpaddb},
                {"paddw", x86_vpaddw},
                {"paddd", x86_vpaddd},
                {"paddq", x86_vpaddq},
                {"psubb", x86_vpsubb},
                {"psubw", x86_vpsubw},
                {"psubd", x86_vpsubd},
                {"psubq", x86_vpsubq},
                {"paddsb", x86_vpaddsb},
                {"paddsw", x86_vpaddsw},
                {"paddusb", x86_vpaddusb},
                {"paddusw", x86_vpaddusw},
                {"psubsb", x86_vpsubsb},
                {"psubsw", x86_vpsubsw},
                {"psubusb", x86_vpsubusb},
                {"psubusw", x86_vpsubusw},
                {"pand", x86_vpand},
                {"pandn", x86_vpandn},
                {"por", x86_vpor},
                {"pxor", x86_vpxor},
                {"andnotsi", x86_vpandn},
                {"pcmpeqb", x86_vpcmpeqb},
                {"pcmpeqw", x86_vpcmpeqw},
                {"pcmpeqd", x86_vpcmpeqd},
                {"pcmpgtb", x86_vpcmpgtb},
                {"pcmpgtw", x86_vpcmpgtw},
                {"pcmpgtd", x86_vpcmpgtd},
                {"pmullw", x86_vpmullw},
                {"pmulhw", x86_vpmulhw},
                {"pmulhuw", x86_vpmulhuw},
                {"pmuludq", x86_vpmuludq},
                {"pmaddwd", x86_vpmaddwd},
                {"pavgb", x86_vpavgb},
                {"pavgw", x86_vpavgw},
                {"psadbw", x86_vpsadbw},
                {"pmaxub", x86_vpmaxub},
                {"pmaxsw", x86_vpmaxsw},
                {"pminub", x86_vpminub},
                {"pminsw", x86_vpminsw},
                {"packsswb", x86_vpacksswb},
                {"packssdw", x86_vpackssdw},
                {"packuswb", x86_vpackuswb},
                {"punpcklbw", x86_vpunpcklbw},
                {"punpcklwd", x86_vpunpcklwd},
                {"punpckldq", x86_vpunpckldq},
                {"punpcklqdq", x86_vpunpcklqdq},
                {"punpckhbw", x86_vpunpckhbw},
                {"punpckhwd", x86_vpunpckhwd},
                {"punpckhdq", x86_vpunpckhdq},
                {"punpckhqdq", x86_vpunpckhqdq},
            };
            for (size_t i = 0; i < sizeof(tab0f) / sizeof(tab0f[0]); i++)
                if (!strcmp(tab0f[i].r, root)) {
                    avx_loadY(X86_XMM1, a1);
                    avx_loadY(X86_XMM2, a2);
                    tab0f[i].fn(cg_sec, X86_XMM0, X86_XMM1, X86_XMM2);
                    return avx_store();
                }
        }
        // int ALU: 0F38 map — abs/sign/hadd/hsub/pmaddubsw/pmulhrsw/pshufb/
        // pmulld/pmuldq/pcmpgtq/min/max/packusdw
        {
            static const struct {
                const char *r;
                void (*fn)(SecBuf *, X86XmmReg, X86XmmReg, X86XmmReg);
            } tab38[] = {
                {"psignb", x86_vpsignb},
                {"psignw", x86_vpsignw},
                {"psignd", x86_vpsignd},
                {"phaddw", x86_vphaddw},
                {"phaddd", x86_vphaddd},
                {"phaddsw", x86_vphaddsw},
                {"phsubw", x86_vphsubw},
                {"phsubd", x86_vphsubd},
                {"phsubsw", x86_vphsubsw},
                {"pmaddubsw", x86_vpmaddubsw},
                {"pmulhrsw", x86_vpmulhrsw},
                {"pshufb", x86_vpshufb},
                {"pmulld", x86_vpmulld},
                {"pmuldq", x86_vpmuldq},
                {"pcmpgtq", x86_vpcmpgtq},
                {"pminsb", x86_vpminsb},
                {"pminsd", x86_vpminsd},
                {"pminuw", x86_vpminuw},
                {"pminud", x86_vpminud},
                {"pmaxsb", x86_vpmaxsb},
                {"pmaxsd", x86_vpmaxsd},
                {"pmaxuw", x86_vpmaxuw},
                {"pmaxud", x86_vpmaxud},
                {"packusdw", x86_vpackusdw},
            };
            for (size_t i = 0; i < sizeof(tab38) / sizeof(tab38[0]); i++)
                if (!strcmp(tab38[i].r, root)) {
                    avx_loadY(X86_XMM1, a1);
                    avx_loadY(X86_XMM2, a2);
                    tab38[i].fn(cg_sec, X86_XMM0, X86_XMM1, X86_XMM2);
                    return avx_store();
                }
            // unary 0F38 ops: pabsb/pabsw/pabsd (single operand)
            if (!strcmp(root, "pabsb") || !strcmp(root, "pabsw") || !strcmp(root, "pabsd")) {
                avx_loadY(X86_XMM1, a1);
                if (!strcmp(root, "pabsb")) x86_vpabsb(cg_sec, X86_XMM0, X86_XMM0, X86_XMM1);
                else if (!strcmp(root, "pabsw"))
                    x86_vpabsw(cg_sec, X86_XMM0, X86_XMM0, X86_XMM1);
                else
                    x86_vpabsd(cg_sec, X86_XMM0, X86_XMM0, X86_XMM1);
                return avx_store();
            }
        }
        // float ALU: addsubps/pd, addps/pd, sub, mul, div, min, max, sqrt,
        // rcp, rsqrt, andn, and, or, xor, unpckl/h
        {
            int op = -1;
            bool unary = false;
            if (!strncmp(root, "addsub", 6)) op = 0xd0;
            else if (!strncmp(root, "hadd", 4))
                op = 0x7c;
            else if (!strncmp(root, "hsub", 4))
                op = 0x7d;
            else if (!strncmp(root, "add", 3))
                op = 0x58;
            else if (!strncmp(root, "sub", 3))
                op = 0x5c;
            else if (!strncmp(root, "mul", 3))
                op = 0x59;
            else if (!strncmp(root, "div", 3))
                op = 0x5e;
            else if (!strncmp(root, "min", 3))
                op = 0x5d;
            else if (!strncmp(root, "max", 3))
                op = 0x5f;
            else if (!strncmp(root, "sqrt", 4)) {
                op = 0x51;
                unary = true;
            } else if (!strncmp(root, "rcp", 3)) {
                op = 0x53;
                unary = true;
            } else if (!strncmp(root, "rsqrt", 5)) {
                op = 0x52;
                unary = true;
            } else if (!strncmp(root, "andn", 4))
                op = 0x55;
            else if (!strncmp(root, "and", 3))
                op = 0x54;
            else if (!strncmp(root, "or", 2))
                op = 0x56;
            else if (!strncmp(root, "xor", 3))
                op = 0x57;
            else if (!strncmp(root, "unpckl", 6))
                op = 0x14;
            else if (!strncmp(root, "unpckh", 6))
                op = 0x15;
            if (op >= 0 && (ia32_suf(root, "ps") || ia32_suf(root, "pd"))) {
                bool ispd = ia32_suf(root, "pd");
                avx_loadY(X86_XMM1, a1);
                if (!unary) avx_loadY(X86_XMM2, a2);
                void (*fn)(SecBuf *, X86XmmReg, X86XmmReg, X86XmmReg) = NULL;
                if (unary) {
                    switch (op) {
                    case 0x51: fn = ispd ? x86_vsqrtpd : x86_vsqrtps; break;
                    case 0x53: fn = x86_vrcpps; break;
                    default: fn = x86_vrsqrtps; break;
                    }
                    fn(cg_sec, X86_XMM0, X86_XMM0, X86_XMM1); // vvvv=1111
                } else {
                    switch (op) {
                    case 0xd0: fn = ispd ? x86_vaddsubpd : x86_vaddsubps; break;
                    case 0x7c: fn = ispd ? x86_vhaddpd : x86_vhaddps; break;
                    case 0x7d: fn = ispd ? x86_vhsubpd : x86_vhsubps; break;
                    case 0x58: fn = ispd ? x86_vaddpd : x86_vaddps; break;
                    case 0x5c: fn = ispd ? x86_vsubpd : x86_vsubps; break;
                    case 0x59: fn = ispd ? x86_vmulpd : x86_vmulps; break;
                    case 0x5e: fn = ispd ? x86_vdivpd : x86_vdivps; break;
                    case 0x5d: fn = ispd ? x86_vminpd : x86_vminps; break;
                    case 0x5f: fn = ispd ? x86_vmaxpd : x86_vmaxps; break;
                    case 0x55: fn = ispd ? x86_vandnpd : x86_vandnps; break;
                    case 0x54: fn = ispd ? x86_vandpd : x86_vandps; break;
                    case 0x56: fn = ispd ? x86_vorpd : x86_vorps; break;
                    case 0x57: fn = ispd ? x86_vxorpd : x86_vxorps; break;
                    case 0x14: fn = ispd ? x86_vunpcklpd : x86_vunpcklps; break;
                    default: fn = ispd ? x86_vunpckhpd : x86_vunpckhps; break;
                    }
                    fn(cg_sec, X86_XMM0, X86_XMM1, X86_XMM2);
                }
                return avx_store();
            }
        }
        error("__builtin_ia32_%s256: unsupported AVX/AVX2 intrinsic", root);
    }
#endif

    // ================= AVX-512: 512-bit mask-form builtins =================
    // The headers' `_mm512_*` wrappers lower most ops to plain C vector
    // arithmetic (handled by gen_vector64_x86); the rest go through
    // `__builtin_ia32_*512_mask` calls whose last arg is a vector mask.
    // The blake3 usage always passes (__v16si)-1, i.e. no masking -> aaa=0.
#ifndef ARCH_ARM64
    if (!strcmp(n, "shuf_i32x4_mask")) {
        avx512_loadZ(X86_XMM2, a1);
        avx512_loadZ(X86_XMM1, a2);
        x86_vshufi32x4(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1, ia32_imm8(a3, "imm"));
        VReg dst = avx512_slot_addr();
        x86_vmovups_mr512(cg_sec, x86_mem(REG(dst), 0), X86_XMM2);
        return dst;
    }
    if (!strcmp(n, "prord256_mask")) {
        VReg va = ia32_vaddr(a1);
        x86_movups_rm(cg_sec, X86_XMM0, x86_mem(REG(va), 0));
        free_reg(va);
        x86_vprord256_i(cg_sec, X86_XMM0, ia32_imm8(a2, "imm"));
        return ia32_store(32);
    }
    if (!strcmp(n, "prord128_mask")) {
        VReg va = ia32_vaddr(a1);
        x86_movups_rm(cg_sec, X86_XMM0, x86_mem(REG(va), 0));
        free_reg(va);
        x86_vprord128_i(cg_sec, X86_XMM0, ia32_imm8(a2, "imm"));
        return ia32_store(16);
    }
    if (!strcmp(n, "pmovqd256_mask")) {
        VReg va = ia32_vaddr(a1);
        x86_movups_rm(cg_sec, X86_XMM1, x86_mem(REG(va), 0));
        free_reg(va);
        x86_vpmovqd256(cg_sec, X86_XMM0, X86_XMM1);
        return ia32_store(16);
    }
    if (!strcmp(n, "storedqusi256_mask")) {
        // masked 256-bit store: (ptr, data, mask); -1 mask -> no masking
        VReg p = gen(a1);
        VReg va = ia32_vaddr(a2);
        x86_vmovups_rm256(cg_sec, X86_XMM0, x86_mem(REG(va), 0)); // 32-byte data operand
        free_reg(va);
        x86_vmovdqu32_mr256(cg_sec, x86_mem(REG(p), 0), X86_XMM0);
        free_reg(p);
        return R_NONE;
    }
    if (!strcmp(n, "extractf64x4_mask")) {
        avx512_loadZ(X86_XMM1, a1);
        x86_vextractf64x4(cg_sec, X86_XMM1, X86_XMM1, ia32_imm8(a2, "imm"));
        VReg dst = avx_slot_addr();
        x86_vmovups_mr256(cg_sec, x86_mem(REG(dst), 0), X86_XMM1);
        return dst;
    }
    if (ia32_suf(n, "512_mask")) {
        char root[64];
        size_t rl = strlen(n) - 8; // strip "512_mask"
        memcpy(root, n, rl);
        root[rl] = 0;
        if (!strcmp(root, "psrldi") || !strcmp(root, "psrlqi") || !strcmp(root, "prord")) {
            avx512_loadZ(X86_XMM1, a1);
            if (!strcmp(root, "psrldi")) x86_vpsrld512_i(cg_sec, X86_XMM1, ia32_imm8(a2, "imm"));
            else if (!strcmp(root, "psrlqi"))
                x86_vpsrlq512_i(cg_sec, X86_XMM1, ia32_imm8(a2, "imm"));
            else
                x86_vprord512_i(cg_sec, X86_XMM1, ia32_imm8(a2, "imm"));
            VReg dst = avx512_slot_addr();
            x86_vmovups_mr512(cg_sec, x86_mem(REG(dst), 0), X86_XMM1);
            return dst;
        }
        if (!strcmp(root, "pandnd")) {
            avx512_loadZ(X86_XMM2, a1);
            avx512_loadZ(X86_XMM1, a2);
            x86_vpandnd512(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1, 0);
            VReg dst = avx512_slot_addr();
            x86_vmovups_mr512(cg_sec, x86_mem(REG(dst), 0), X86_XMM2);
            return dst;
        }
        if (!strcmp(root, "punpckldq") || !strcmp(root, "punpckhdq") ||
            !strcmp(root, "punpcklqdq") || !strcmp(root, "punpckhqdq")) {
            avx512_loadZ(X86_XMM2, a1);
            avx512_loadZ(X86_XMM1, a2);
            if (!strcmp(root, "punpckldq")) x86_vpunpckldq512(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1, 0);
            else if (!strcmp(root, "punpckhdq"))
                x86_vpunpckhdq512(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1, 0);
            else if (!strcmp(root, "punpcklqdq"))
                x86_vpunpcklqdq512(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1, 0);
            else
                x86_vpunpckhqdq512(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1, 0);
            VReg dst = avx512_slot_addr();
            x86_vmovups_mr512(cg_sec, x86_mem(REG(dst), 0), X86_XMM2);
            return dst;
        }
        if (!strcmp(root, "shuf_i32x4")) {
            avx512_loadZ(X86_XMM2, a1);
            avx512_loadZ(X86_XMM1, a2);
            x86_vshufi32x4(cg_sec, X86_XMM2, X86_XMM2, X86_XMM1, ia32_imm8(a3, "imm"));
            VReg dst = avx512_slot_addr();
            x86_vmovups_mr512(cg_sec, x86_mem(REG(dst), 0), X86_XMM2);
            return dst;
        }
        if (!strcmp(root, "pmovqd")) {
            avx512_loadZ(X86_XMM1, a1);
            x86_vpmovqd512(cg_sec, X86_XMM0, X86_XMM1);
            return ia32_store(32);
        }
        if (!strcmp(root, "ucmpd")) {
            avx512_loadZ(X86_XMM1, a1);
            avx512_loadZ(X86_XMM2, a2);
            x86_vpcmpud512(cg_sec, (X86XmmReg)1, X86_XMM1, X86_XMM2, ia32_imm8(a3, "imm")); // k1
            VReg r = alloc_reg();
            x86_kmovw_r32_k1(cg_sec, REG(r));
            return r;
        }
        error("__builtin_ia32_%s512_mask: unsupported AVX-512 intrinsic", root);
    }
    if (ia32_suf(n, "512")) {
        char root[64];
        size_t rl = strlen(n) - 3;
        memcpy(root, n, rl);
        root[rl] = 0;
        error("__builtin_ia32_%s512: unsupported AVX-512 intrinsic", root);
    }
#endif

    // ================= crc32 =================
    if (!strncmp(n, "crc32", 5)) {
        // crc32 r32, r/m8/16/32/64: the operand SIZE comes from the name
        // suffix (qi=byte, hi=word, si=dword, di=qword), never from the
        // argument's type (the headers pass unsigned char/values, but
        // direct calls pass plain int literals).
        int vsz = !strcmp(n, "crc32qi") ? 1 : !strcmp(n, "crc32hi") ? 2
            : !strcmp(n, "crc32di")                                 ? 8
                                                                    : 4;
        VReg c0 = gen(a1);
        VReg v = gen(a2);
        VReg r = alloc_reg();
        x86_mov_rr(cg_sec, 4, REG(r), REG(c0)); // seed with the incoming CRC
        if (vsz == 1) x86_crc32qi(cg_sec, REG(r), REG(v));
        else
            x86_crc32si(cg_sec, REG(r), REG(v), vsz);
        free_reg(c0);
        free_reg(v);
        return r;
    }

    // ================= move-mask / compare-flag int results =================
    if (!strncmp(n, "ptest", 5)) {
        ia32_load2(a1, a2, 16);
        ia32_emit38(0x17); // ptest xmm0, xmm1: ZF = (xmm0 & xmm1)==0, CF = (~xmm0 & xmm1)==0
        VReg r = alloc_reg();
        if (n[5] == 'z') {
            asm_setcc(cg_sec, X86_RAX, X86_E); // ZF: (a & b) == 0
            asm_movzx_phys(cg_sec, r, X86_RAX, 4, 1);
        } else if (n[5] == 'c') {
            asm_setcc(cg_sec, X86_RAX, X86_B); // CF: (~a & b) == 0
            asm_movzx_phys(cg_sec, r, X86_RAX, 4, 1);
        } else {
            // nzc: !(ZF || CF)
            asm_setcc(cg_sec, X86_RAX, X86_E);
            asm_setcc(cg_sec, X86_RCX, X86_B);
            x86_or_rr(cg_sec, 1, X86_RAX, X86_RCX);
            x86_xor_ri(cg_sec, 1, X86_RAX, 1);
            asm_movzx_phys(cg_sec, r, X86_RAX, 4, 1);
        }
        return r;
    }
    // comi*/ucomi*: compare two floats, return flag-derived int
    if (!strncmp(n, "comi", 4) || !strncmp(n, "ucomi", 5)) {
        ia32_load2(a1, a2, 16);
        if (strncmp(n, "ucomi", 5) == 0) {
            if (strstr(n, "sd")) x86_ucomisd(cg_sec, X86_XMM0, X86_XMM1);
            else
                x86_ucomiss(cg_sec, X86_XMM0, X86_XMM1);
        } else {
            if (strstr(n, "sd")) x86_comisd(cg_sec, X86_XMM0, X86_XMM1);
            else
                x86_comiss(cg_sec, X86_XMM0, X86_XMM1);
        }
        VReg r = alloc_reg();
        if (ia32_suf(n, "eq")) asm_setcc(cg_sec, X86_RAX, X86_E);
        else if (ia32_suf(n, "neq"))
            asm_setcc(cg_sec, X86_RAX, X86_NE);
        else if (ia32_suf(n, "lt"))
            asm_setcc(cg_sec, X86_RAX, X86_B);
        else if (ia32_suf(n, "le"))
            asm_setcc(cg_sec, X86_RAX, X86_BE);
        else if (ia32_suf(n, "gt"))
            asm_setcc(cg_sec, X86_RAX, X86_A);
        else
            asm_setcc(cg_sec, X86_RAX, X86_AE); // ge
        asm_movzx_phys(cg_sec, r, X86_RAX, 4, 1);
        return r;
    }

    // ================= converts =================
    if (!strncmp(n, "cvt", 3)) {
        // scalar int -> float: cvtsi2ss/sd (2 args: vec, int)
        if (!strcmp(n, "cvtsi2ss") || !strcmp(n, "cvtsi642ss") ||
            !strcmp(n, "cvtsi2sd") || !strcmp(n, "cvtsi642sd")) {
            VReg va = ia32_vaddr(a1);
            x86_movups_rm(cg_sec, X86_XMM0, x86_mem(REG(va), 0));
            free_reg(va);
            VReg iv = gen(a2);
            int isz = a2->ty ? (int)a2->ty->size : 4;
            if (isz > 4) isz = 8;
            if (ia32_suf(n, "ss")) x86_cvtsi2ss(cg_sec, isz, X86_XMM0, REG(iv));
            else
                x86_cvtsi2sd(cg_sec, isz, X86_XMM0, REG(iv));
            free_reg(iv);
            return ia32_store(16);
        }
        // float -> int scalar: cvtss2si/cvtsd2si (truncate forms exist too)
        if (!strcmp(n, "cvtss2si") || !strcmp(n, "cvttss2si") ||
            !strcmp(n, "cvtsd2si") || !strcmp(n, "cvttsd2si") ||
            !strcmp(n, "cvtss2si64") || !strcmp(n, "cvttss2si64") ||
            !strcmp(n, "cvtsd2si64") || !strcmp(n, "cvttsd2si64")) {
            bool is64 = ia32_suf(n, "64");
            bool is_sd = strstr(n, "sd2si") != NULL;
            VReg va = ia32_vaddr(a1);
            if (is_sd) x86_movsd_rm(cg_sec, X86_XMM0, x86_mem(REG(va), 0));
            else
                x86_movss_rm(cg_sec, X86_XMM0, x86_mem(REG(va), 0));
            free_reg(va);
            VReg r = alloc_reg();
            bool trunc = !strncmp(n, "cvtt", 4);
            if (trunc) {
                if (is_sd) x86_cvttsd2si(cg_sec, is64 ? 8 : 4, REG(r), X86_XMM0);
                else
                    x86_cvttss2si(cg_sec, is64 ? 8 : 4, REG(r), X86_XMM0);
            } else {
                if (is_sd) x86_cvtsd2si(cg_sec, is64 ? 8 : 4, REG(r), X86_XMM0);
                else
                    x86_cvtss2si(cg_sec, is64 ? 8 : 4, REG(r), X86_XMM0);
            }
            return r;
        }
        // packed float <-> int / width conversions
        if (!strcmp(n, "cvtps2dq") || !strcmp(n, "cvttps2dq") ||
            !strcmp(n, "cvtdq2ps") || !strcmp(n, "cvtps2pd") ||
            !strcmp(n, "cvtpd2ps") || !strcmp(n, "cvtdq2pd") ||
            !strcmp(n, "cvtpd2dq") || !strcmp(n, "cvttpd2dq")) {
            ia32_load1(a1, 16);
            if (!strcmp(n, "cvtps2dq")) sse_rr_66(cg_sec, 0x5b, X86_XMM0, X86_XMM0);
            else if (!strcmp(n, "cvttps2dq"))
                sse_rr_f3(cg_sec, 0x5b, X86_XMM0, X86_XMM0);
            else if (!strcmp(n, "cvtdq2ps"))
                sse_rr_np(cg_sec, 0x5b, X86_XMM0, X86_XMM0);
            else if (!strcmp(n, "cvtps2pd"))
                sse_rr_np(cg_sec, 0x5a, X86_XMM0, X86_XMM0);
            else if (!strcmp(n, "cvtpd2ps"))
                sse_rr_66(cg_sec, 0x5a, X86_XMM0, X86_XMM0);
            else if (!strcmp(n, "cvtdq2pd"))
                sse_rr_f3(cg_sec, 0xe6, X86_XMM0, X86_XMM0);
            else if (!strcmp(n, "cvtpd2dq"))
                sse_rr_f2(cg_sec, 0xe6, X86_XMM0, X86_XMM0);
            else
                sse_rr_66(cg_sec, 0xe6, X86_XMM0, X86_XMM0); // cvttpd2dq
            return ia32_store(16);
        }
        // scalar float conversions: cvtss2sd / cvtsd2ss (2-arg: A, B)
        if (!strcmp(n, "cvtss2sd") || !strcmp(n, "cvtsd2ss")) {
            ia32_load2(a1, a2, 16); // XMM0 = A (dst, upper preserved), XMM1 = B
            if (!strcmp(n, "cvtss2sd")) sse_rr_f3(cg_sec, 0x5a, X86_XMM0, X86_XMM1);
            else
                sse_rr_f2(cg_sec, 0x5a, X86_XMM0, X86_XMM1);
            return ia32_store(16);
        }
        // MMX converts. cvtpi2ps(A, B): dst = A with low 64 = conv(B[0..1]).
        // cvtpi2pd(A): dst = {conv(A[0]), conv(A[1])} — BOTH lanes written,
        // no dst operand (the header passes a single __m64 argument).
        if (!strcmp(n, "cvtpi2ps") || !strcmp(n, "cvtpi2pd")) {
            if (!strcmp(n, "cvtpi2pd")) {
                VReg vb = ia32_vaddr(a1); // the MMX source is the only arg
                x86_movq_rm(cg_sec, x86_mem(REG(vb), 0), X86_XMM1);
                free_reg(vb);
                x86_cvtpi2pd(cg_sec, X86_XMM0, X86_XMM1);
                return ia32_store(16);
            }
            VReg va = ia32_vaddr(a1);
            x86_movups_rm(cg_sec, X86_XMM0, x86_mem(REG(va), 0));
            free_reg(va);
            VReg vb = ia32_vaddr(a2); // 8-byte MMX operand
            x86_movq_rm(cg_sec, x86_mem(REG(vb), 0), X86_XMM1);
            free_reg(vb);
            x86_cvtpi2ps(cg_sec, X86_XMM0, X86_XMM1);
            return ia32_store(16);
        }
        if (!strcmp(n, "cvtps2pi") || !strcmp(n, "cvttps2pi") ||
            !strcmp(n, "cvtpd2pi") || !strcmp(n, "cvttpd2pi")) {
            VReg va = ia32_vaddr(a1);
            x86_movups_rm(cg_sec, X86_XMM0, x86_mem(REG(va), 0));
            free_reg(va);
            if (!strcmp(n, "cvtps2pi")) x86_cvtps2pi(cg_sec, X86_XMM0, X86_XMM0);
            else if (!strcmp(n, "cvttps2pi"))
                x86_cvttps2pi(cg_sec, X86_XMM0, X86_XMM0);
            else if (!strcmp(n, "cvtpd2pi"))
                x86_cvtpd2pi(cg_sec, X86_XMM0, X86_XMM0);
            else
                x86_cvttpd2pi(cg_sec, X86_XMM0, X86_XMM0);
            return ia32_store(8);
        }
    }

    // ================= shuffles / byte permutes with immediate =================
    if (!strcmp(n, "shufps") || !strcmp(n, "shufpd")) {
        ia32_load2(a1, a2, 16);
        uint8_t imm = ia32_imm8(a3, "imm");
        if (!strcmp(n, "shufps")) x86_shufps(cg_sec, X86_XMM0, X86_XMM1, imm);
        else
            x86_shufpd(cg_sec, X86_XMM0, X86_XMM1, imm);
        return ia32_store(16);
    }
    if (!strcmp(n, "pshufd") || !strcmp(n, "pshuflw") || !strcmp(n, "pshufhw") ||
        !strcmp(n, "pshufw")) {
        int bytes = !strcmp(n, "pshufd") || !strcmp(n, "pshuflw") || !strcmp(n, "pshufhw") ? 16 : 8;
        ia32_load1(a1, bytes);
        uint8_t imm = ia32_imm8(a2, "imm");
        if (!strcmp(n, "pshufd")) x86_pshufd(cg_sec, X86_XMM0, X86_XMM0, imm);
        else if (!strcmp(n, "pshuflw"))
            x86_pshuflw(cg_sec, X86_XMM0, X86_XMM0, imm);
        else if (!strcmp(n, "pshufhw"))
            x86_pshufhw(cg_sec, X86_XMM0, X86_XMM0, imm);
        else
            x86_pshufw(cg_sec, X86_XMM0, X86_XMM0, imm);
        return ia32_store(bytes);
    }
    if (!strcmp(n, "palignr") || !strcmp(n, "palignr128")) {
        int bytes = !strcmp(n, "palignr") ? 8 : 16;
        ia32_load2(a1, a2, bytes);
        x86_palignr(cg_sec, X86_XMM0, X86_XMM1, ia32_imm8(a3, "imm"));
        return ia32_store(bytes);
    }

    // ================= shifts =================
    {
        const char *sh = NULL;
        if (!strncmp(n, "psllw", 5)) sh = "psllw";
        else if (!strncmp(n, "pslld", 5))
            sh = "pslld";
        else if (!strncmp(n, "psllq", 5))
            sh = "psllq";
        else if (!strncmp(n, "psrlw", 5))
            sh = "psrlw";
        else if (!strncmp(n, "psrld", 5))
            sh = "psrld";
        else if (!strncmp(n, "psrlq", 5))
            sh = "psrlq";
        else if (!strncmp(n, "psraw", 5))
            sh = "psraw";
        else if (!strncmp(n, "psrad", 5))
            sh = "psrad";
        if (sh) {
            bool is128 = ia32_suf(n, "128") || strstr(n, "128") != NULL;
            int bytes = is128 ? 16 : 8;
            bool imm_form = ia32_suf(n, "i") || ia32_suf(n, "i128") || n[strlen(n) - 1] == 'i';
            // strip trailing "128"/"i" to find the root shift family
            char root[16];
            size_t rl = strlen(n);
            while (rl > 0 && (n[rl - 1] == '1' || n[rl - 1] == '2' || n[rl - 1] == '8' || n[rl - 1] == 'i'))
                rl--;
            memcpy(root, n, rl);
            root[rl] = 0;
            // dq shifts: pslldqi128/psrldqi128 -> group14 with ext 7/3
            if (!strcmp(root, "pslldq") || !strcmp(root, "psrldq")) {
                ia32_load1(a1, 16);
                if (!strcmp(root, "pslldq")) x86_pslldq(cg_sec, X86_XMM0, ia32_imm8(a2, "imm"));
                else
                    x86_psrldq(cg_sec, X86_XMM0, ia32_imm8(a2, "imm"));
                return ia32_store(16);
            }
            if (imm_form) {
                // The count is a compile-time constant: immediate form
                // (group 12/13/14). A runtime int count (GCC's
                // __builtin_ia32_psllwi takes both, mmintrin.h's
                // _mm_slli_pi16 passes a plain int) falls back to the
                // register-count form below.
                if (ia32_is_const(a2)) {
                    ia32_load1(a1, bytes);
                    uint8_t imm = ia32_imm8(a2, "imm");
                    // group 12 (words, 66 0F 71), 13 (dwords, 72), 14 (qwords, 73)
                    int grp = !strncmp(sh, "psllw", 5) || !strncmp(sh, "psrlw", 5) || !strncmp(sh, "psraw", 5) ? 0x71
                        : !strncmp(sh, "psllq", 5) || !strncmp(sh, "psrlq", 5)                                 ? 0x73
                                                                                                               : 0x72;
                    int ext = !strncmp(sh, "psll", 4) ? 6
                        : !strncmp(sh, "psrl", 4)     ? 2
                                                      : 4; // psra
                    emit1(cg_sec, 0x66);
                    maybe_rex(cg_sec, 0, 0, 0, (int)X86_XMM0);
                    emit3(cg_sec, 0x0f, (uint8_t)grp, (uint8_t)((3 << 6) | (ext << 3) | ((int)X86_XMM0 & 7)));
                    emit1(cg_sec, imm);
                    return ia32_store(bytes);
                }
                // runtime count: load it into XMM1 (low bits), register form
                VReg cnt = gen(a2);
                asm_sub_rsp_imm(cg_sec, 16);
                x86_mov_mr(cg_sec, 8, x86_mem(X86_RSP, 0), REG(cnt));
                free_reg(cnt);
                x86_movq_rm(cg_sec, x86_mem(X86_RSP, 0), X86_XMM1);
                asm_add_rsp_imm(cg_sec, 16);
                ia32_load1(a1, bytes);
                if (!strcmp(root, "psllw")) x86_psllw_r(cg_sec, X86_XMM0, X86_XMM1);
                else if (!strcmp(root, "pslld"))
                    x86_pslld_r(cg_sec, X86_XMM0, X86_XMM1);
                else if (!strcmp(root, "psllq"))
                    x86_psllq_r(cg_sec, X86_XMM0, X86_XMM1);
                else if (!strcmp(root, "psrlw"))
                    x86_psrlw_r(cg_sec, X86_XMM0, X86_XMM1);
                else if (!strcmp(root, "psrld"))
                    x86_psrld_r(cg_sec, X86_XMM0, X86_XMM1);
                else if (!strcmp(root, "psrlq"))
                    x86_psrlq_r(cg_sec, X86_XMM0, X86_XMM1);
                else if (!strcmp(root, "psraw"))
                    x86_psraw_r(cg_sec, X86_XMM0, X86_XMM1);
                else
                    x86_psrad_r(cg_sec, X86_XMM0, X86_XMM1);
                return ia32_store(bytes);
            }
            // register-count form: shift by XMM1's low bits
            ia32_load2(a1, a2, bytes);
            if (!strcmp(root, "pslldq") || !strcmp(root, "psrldq")) {
                ia32_load1(a1, 16);
                if (!strcmp(root, "pslldq")) x86_pslldq(cg_sec, X86_XMM0, ia32_imm8(a2, "imm"));
                else
                    x86_psrldq(cg_sec, X86_XMM0, ia32_imm8(a2, "imm"));
                return ia32_store(16);
            }
            return ia32_store(bytes);
        }
    }

    // ================= float ALU / compare (ss/sd/ps/pd) =================
    if (ia32_suf(n, "ss") || ia32_suf(n, "sd") || ia32_suf(n, "ps") || ia32_suf(n, "pd")) {
        int op = -1, pfx = 0;
        bool unary = false;
        // addsub*/hadd*/hsub* MUST be checked before the plain add/sub
        // (addsubps starts with "add" and would match 0x58 first).
        if (!strncmp(n, "addsub", 6)) op = 0xd0;
        else if (!strncmp(n, "hadd", 4))
            op = 0x7c;
        else if (!strncmp(n, "hsub", 4))
            op = 0x7d;
        else if (!strncmp(n, "add", 3))
            op = 0x58;
        else if (!strncmp(n, "sub", 3))
            op = 0x5c;
        else if (!strncmp(n, "mul", 3))
            op = 0x59;
        else if (!strncmp(n, "div", 3))
            op = 0x5e;
        else if (!strncmp(n, "min", 3))
            op = 0x5d;
        else if (!strncmp(n, "max", 3))
            op = 0x5f;
        else if (!strncmp(n, "sqrt", 4)) {
            op = 0x51;
            unary = true;
        } else if (!strncmp(n, "rcp", 3)) {
            op = 0x53;
            unary = true;
        } else if (!strncmp(n, "rsqrt", 5)) {
            op = 0x52;
            unary = true;
        } else if (!strncmp(n, "andn", 4))
            op = 0x55;
        else if (!strncmp(n, "and", 3))
            op = 0x54;
        else if (!strncmp(n, "or", 2))
            op = 0x56;
        else if (!strncmp(n, "xor", 3))
            op = 0x57;
        else if (!strncmp(n, "unpckl", 6))
            op = 0x14;
        else if (!strncmp(n, "unpckh", 6))
            op = 0x15;
        if (op >= 0) {
            if (ia32_suf(n, "ss") || ia32_suf(n, "ps")) {
                // addsubps/haddps/hsubps take the F2 prefix (not none)
                if (op == 0xd0 || op == 0x7c || op == 0x7d)
                    pfx = ia32_suf(n, "ss") ? 0xf3 : 0xf2;
                else
                    pfx = ia32_suf(n, "ss") ? 0xf3 : 0x00;
            } else {
                pfx = ia32_suf(n, "sd") ? 0xf2 : 0x66;
            }
            if (unary) {
                ia32_load1(a1, 16);
                // unary: the source is XMM0 itself (sqrtps xmm0, xmm0)
                switch (pfx) {
                case 0x66: sse_rr_66(cg_sec, (uint8_t)op, X86_XMM0, X86_XMM0); break;
                case 0xf3: sse_rr_f3(cg_sec, (uint8_t)op, X86_XMM0, X86_XMM0); break;
                case 0xf2: sse_rr_f2(cg_sec, (uint8_t)op, X86_XMM0, X86_XMM0); break;
                default: sse_rr_np(cg_sec, (uint8_t)op, X86_XMM0, X86_XMM0); break;
                }
                return ia32_store(16);
            }
            ia32_load2(a1, a2, 16);
            ia32_emit2(pfx, op);
            return ia32_store(16);
        }
        // cmpXX{ss,sd,ps,pd}: two-arg compare with fixed predicate
        int cmpimm = -1;
        if (!strncmp(n, "cmpeq", 5)) cmpimm = 0;
        else if (!strncmp(n, "cmplt", 5))
            cmpimm = 1;
        else if (!strncmp(n, "cmple", 5))
            cmpimm = 2;
        else if (!strncmp(n, "cmpunord", 8))
            cmpimm = 3;
        else if (!strncmp(n, "cmpneq", 6))
            cmpimm = 4;
        else if (!strncmp(n, "cmpnlt", 6))
            cmpimm = 5;
        else if (!strncmp(n, "cmpnle", 6))
            cmpimm = 6;
        else if (!strncmp(n, "cmpord", 6))
            cmpimm = 7;
        else if (!strncmp(n, "cmpge", 5))
            cmpimm = 5; // A >= B  ==  !(A < B)
        else if (!strncmp(n, "cmpgt", 5))
            cmpimm = 1; // A > B   ==  B < A
        else if (!strncmp(n, "cmpnge", 6))
            cmpimm = 1; // !(A>=B) ==  A < B
        else if (!strncmp(n, "cmpngt", 6))
            cmpimm = 5; // !(A>B)  ==  !(B<A)
        if (cmpimm >= 0) {
            bool swap = !strncmp(n, "cmpgt", 5) || !strncmp(n, "cmpngt", 6);
            if (swap) {
                ia32_load2(a2, a1, 16); // swap operands
            } else {
                ia32_load2(a1, a2, 16);
            }
            int pfx = ia32_suf(n, "ss") ? 0xf3 : ia32_suf(n, "sd") ? 0xf2
                : ia32_suf(n, "pd")                                ? 0x66
                                                                   : 0x00;
            if (pfx) emit1(cg_sec, (uint8_t)pfx);
            maybe_rex(cg_sec, 0, (int)X86_XMM0, 0, (int)X86_XMM1);
            emit3(cg_sec, 0x0f, 0xc2, modrxmm(3, X86_XMM0, X86_XMM1));
            emit1(cg_sec, (uint8_t)cmpimm);
            return ia32_store(16);
        }
        // cmpps/cmppd/cmpss/cmpsd: immediate predicate form
        if (!strcmp(n, "cmpps") || !strcmp(n, "cmppd") || !strcmp(n, "cmpss") || !strcmp(n, "cmpsd")) {
            ia32_load2(a1, a2, 16);
            int pfx = !strcmp(n, "cmpss") ? 0xf3 : !strcmp(n, "cmpsd") ? 0xf2
                : !strcmp(n, "cmppd")                                  ? 0x66
                                                                       : 0x00;
            if (ia32_is_const(a3)) {
                if (pfx) emit1(cg_sec, (uint8_t)pfx);
                maybe_rex(cg_sec, 0, (int)X86_XMM0, 0, (int)X86_XMM1);
                emit3(cg_sec, 0x0f, 0xc2, modrxmm(3, X86_XMM0, X86_XMM1));
                emit1(cg_sec, ia32_imm8(a3, "imm"));
                return ia32_store(16);
            }
            // Runtime predicate (xmmintrin.h's _mm_cmp_ps takes an int P;
            // GCC requires it constant, so this only ever fires when the
            // header's own inline body is compiled as the local copy):
            // dispatch among the 8 predicates.
            {
                int c = ++rcc_label_count;
                VReg pr = gen(a3);
                for (int pred = 0; pred < 8; pred++) {
                    if (pred > 0) {
                        asm_cmp_imm(cg_sec, pr, 4, pred);
                        size_t o = asm_jcc_label(cg_sec, X86_NE);
                        asm_fixup_add(cg_sec, o, format(".L.cmp.%d.next.%d", c, pred), 1);
                    }
                    if (pfx) emit1(cg_sec, (uint8_t)pfx);
                    maybe_rex(cg_sec, 0, (int)X86_XMM0, 0, (int)X86_XMM1);
                    emit3(cg_sec, 0x0f, 0xc2, modrxmm(3, X86_XMM0, X86_XMM1));
                    emit1(cg_sec, (uint8_t)pred);
                    {
                        size_t o = asm_jmp_label(cg_sec);
                        asm_fixup_add(cg_sec, o, format(".L.cmp.%d.end.%d", c, pred), 0);
                    }
                    if (pred < 7) cg_def_label(format(".L.cmp.%d.next.%d", c, pred));
                }
                cg_def_label(format(".L.cmp.%d.end.%d", c, 0));
                free_reg(pr);
                return ia32_store(16);
            }
        }
    }


    // ================= integer vector two-register ops =================
    {
        bool is128 = ia32_suf(n, "128");
        int end = (int)strlen(n) - (is128 ? 3 : 0);
        char root[24];
        size_t rl = (size_t)end;
        if (rl > 23) rl = 23;
        memcpy(root, n, rl);
        root[rl] = 0;
        int op = ia32_int_op(root);
        int bytes = is128 ? 16 : 8;
        if (op >= 0) {
            ia32_load2(a1, a2, bytes);
            ia32_emit2(0x66, op);
            return ia32_store(bytes);
        }
        // 66 0F 38 ops (SSSE3/SSE4.x). The MMX-era roots have 8-byte
        // (bare) and 16-byte (128-suffixed) forms; the SSE4-only roots
        // (blendv*, ptest, pmovsx/zx, pmuldq, pcmpeqq, packusdw,
        // pcmpgtq, pminsd.., pmulld, phminposuw, movntdqa) are always
        // 16 bytes even without the suffix.
        int bytes38 = 16;
        if (!strncmp(root, "pshufb", 6) || !strncmp(root, "pabs", 4) ||
            !strncmp(root, "psign", 5) || !strncmp(root, "phadd", 5) ||
            !strncmp(root, "phsub", 5) || !strcmp(root, "pmaddubsw") ||
            !strcmp(root, "pmulhrsw"))
            bytes38 = is128 ? 16 : 8;
        op = ia32_int38_op(root);
        if (op >= 0) {
            if (!strcmp(root, "ptest")) {
                ia32_load2(a1, a2, 16);
                ia32_emit38(op);
                VReg r = alloc_reg();
                asm_setcc(cg_sec, X86_RAX, X86_E);
                asm_movzx_phys(cg_sec, r, X86_RAX, 4, 1);
                return r;
            }
            if (!strcmp(root, "movntdqa")) {
                VReg p = gen(a1);
                x86_movntdqa_rm(cg_sec, x86_mem(REG(p), 0), X86_XMM0);
                free_reg(p);
                return ia32_store(16);
            }
            // Unary 0F38 ops (pabs*, pmovsx/zx*, phminposuw): the source
            // is XMM0 itself, not a second argument.
            bool un38 = !strncmp(root, "pabs", 4) ||
                !strncmp(root, "pmovsx", 6) || !strncmp(root, "pmovzx", 6) ||
                !strcmp(root, "phminposuw");
            if (un38) {
                ia32_load1(a1, bytes38);
                emit1(cg_sec, 0x66);
                maybe_rex(cg_sec, 0, (int)X86_XMM0, 0, (int)X86_XMM0);
                emit3(cg_sec, 0x0f, 0x38, (uint8_t)op);
                emit1(cg_sec, modrxmm(3, X86_XMM0, X86_XMM0));
                return ia32_store(bytes38);
            }
            ia32_load2(a1, a2, bytes38);
            ia32_emit38(op);
            return ia32_store(bytes38);
        }
        // 66 0F 3A imm ops (SSE4.x). roundps/roundpd are UNARY: their
        // imm is the second argument (xmmintrin.h: _mm_round_ps(A, mode)).
        // All are 16-byte except the bare MMX palignr (palignr128: 16).
        int bytes3a = (!strcmp(root, "palignr") && !is128) ? 8 : 16;
        op = ia32_int3a_op(root);
        if (op >= 0) {
            if (!strcmp(root, "roundps") || !strcmp(root, "roundpd")) {
                ia32_load1(a1, bytes3a);
                emit1(cg_sec, 0x66);
                maybe_rex(cg_sec, 0, (int)X86_XMM0, 0, (int)X86_XMM0);
                emit3(cg_sec, 0x0f, 0x3a, (uint8_t)op);
                emit1(cg_sec, modrxmm(3, X86_XMM0, X86_XMM0));
                emit1(cg_sec, ia32_imm8(a2, "imm"));
                return ia32_store(bytes3a);
            }
            ia32_load2(a1, a2, bytes3a);
            ia32_emit3a(op, ia32_imm8(a3, "imm"));
            return ia32_store(bytes3a);
        }
    }

    // ================= misc vector results =================
    if (!strcmp(n, "pshufb") || !strcmp(n, "pshufb128")) {
        int bytes = !strcmp(n, "pshufb") ? 8 : 16;
        ia32_load2(a1, a2, bytes);
        x86_pshufb(cg_sec, X86_XMM0, X86_XMM1);
        return ia32_store(bytes);
    }
    if (!strcmp(n, "lddqu")) {
        VReg p = gen(a1);
        x86_lddqu_rm(cg_sec, x86_mem(REG(p), 0), X86_XMM0);
        free_reg(p);
        return ia32_store(16);
    }
    if (!strcmp(n, "blendvps") || !strcmp(n, "blendvpd") || !strcmp(n, "pblendvb") || !strcmp(n, "pblendvb128")) {
        // implicit xmm0 mask: dst[i] = (xmm0[i] & 0x80) ? src2[i] : src1[i].
        // Instruction operand encoding: reg field = arg1, rm field = arg2.
        VReg da = ia32_vaddr(a1);
        x86_movups_rm(cg_sec, X86_XMM1, x86_mem(REG(da), 0));
        free_reg(da);
        VReg db = ia32_vaddr(a2);
        x86_movups_rm(cg_sec, X86_XMM2, x86_mem(REG(db), 0));
        free_reg(db);
        VReg dm = ia32_vaddr(a3);
        x86_movups_rm(cg_sec, X86_XMM0, x86_mem(REG(dm), 0));
        free_reg(dm);
        // PBLENDVB/BLENDVPS/BLENDVPD's ModRM.reg operand is BOTH the
        // destination and the first source (dst[i] = XMM0[i] & 0x80 ?
        // src2[i] : reg[i]), so the blended result lands in XMM1, not
        // XMM0 -- ia32_store() always reads XMM0, so copy the result
        // there before storing (XMM0's mask value is dead after this).
        if (!strcmp(n, "blendvps")) {
            x86_blendvps(cg_sec, X86_XMM1, X86_XMM2);
            x86_movaps(cg_sec, X86_XMM0, X86_XMM1);
        } else if (!strcmp(n, "blendvpd")) {
            x86_blendvpd(cg_sec, X86_XMM1, X86_XMM2);
            x86_movaps(cg_sec, X86_XMM0, X86_XMM1);
        } else {
            x86_pblendvb(cg_sec, X86_XMM1, X86_XMM2);
            x86_movaps(cg_sec, X86_XMM0, X86_XMM1);
        }
        return ia32_store(16);
    }
    if (!strcmp(n, "vec_ext_v4sf") || !strcmp(n, "vec_ext_v4si") ||
        !strcmp(n, "vec_ext_v2di") || !strcmp(n, "vec_ext_v16qi") ||
        !strcmp(n, "vec_ext_v8hi") || !strcmp(n, "vec_ext_v2si") ||
        !strcmp(n, "vec_ext_v4hi") || !strcmp(n, "vec_ext_v8qi")) {
        // extract element at index a2 (constant) from vector a1
        VReg va = ia32_vaddr(a1);
        uint64_t idx = (uint64_t)ia32_imm8(a2, "index");
        Type *ety = a1->ty ? a1->ty->base : NULL;
        int esz = ety ? (int)ety->size : 4;
        VReg r = alloc_reg();
        x86_mov_rm(cg_sec, esz <= 4 ? 4 : 8, REG(r), x86_mem(REG(va), (int)(idx * esz)));
        free_reg(va);
        return r;
    }
    if (!strncmp(n, "vec_set_v", 9)) {
        // __builtin_ia32_vec_set_*: insert scalar a2 into vector a1 at lane a3.
        // Copy the original vector to the result slot, then overwrite the
        // selected lane with the scalar. Element width is the result type's
        // base element size (v2di -> 8, v4si -> 4, ...).
        Type *ety = node->ty ? node->ty->base : NULL;
        int esz = ety ? (int)ety->size : 4;
        uint64_t idx = (uint64_t)ia32_imm8(a3, "index");
        VReg dst = alloc_int128_addr();
        VReg va = ia32_vaddr(a1);
        x86_movups_rm(cg_sec, X86_XMM0, x86_mem(REG(va), 0));
        x86_movups_mr(cg_sec, x86_mem(REG(dst), 0), X86_XMM0);
        free_reg(va);
        VReg v = gen(a2);
        x86_mov_mr(cg_sec, esz, x86_mem(REG(dst), (int)(idx * esz)), REG(v));
        free_reg(v);
        return dst;
    }
    if (!strncmp(n, "vec_init_v", 10)) {
        // build a vector from scalar args: store each to the slot. The
        // element width comes from the RESULT vector type (v4hi -> 2
        // bytes), never from the args (which are plain int literals).
        Type *ety = node->ty ? node->ty->base : NULL;
        int esz = ety ? (int)ety->size : 4;
        VReg dst = alloc_int128_addr();
        int i = 0;
        for (Node *p = a1; p; p = p->next, i++) {
            VReg v = gen(p);
            x86_mov_mr(cg_sec, esz, x86_mem(REG(dst), i * esz), REG(v));
            free_reg(v);
        }
        return dst;
    }
    // unhandled intrinsic: clear diagnostic instead of emitting garbage
    error_tok(node->tok, "__builtin_ia32_%s: intrinsic not yet implemented", n);
    return R_NONE;
}


#endif /* !ARCH_ARM64 */
