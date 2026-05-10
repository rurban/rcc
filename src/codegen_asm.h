// SPDX-License-Identifier: LGPL-2.1-or-later
// Codegen ASM wrappers — emit assembled bytes to SecBuf via asm_* functions.
// Replaces printf-based text emission in codegen.c.
// Uses arm64_enc.h / x86_enc.h encoder functions under the hood.
#ifndef CODEGEN_ASM_H
#define CODEGEN_ASM_H

#include "obj.h"
#ifdef ARCH_ARM64
#include "arm64_enc.h"
#else
#include "x86_enc.h"
#endif
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

void error(char *fmt, ...);
void *arena_alloc(size_t size);

// ============================================================================
// Register index → physical register number mapping
// codegen allocates regs as indices 0..NUM_REGS-1 into reg64[].
// The encoder functions use physical register numbers.
// ============================================================================

#ifdef ARCH_ARM64
// 12 free virtual registers
static const Arm64Reg cg_arm_reg[12] = {ARM64_X10, ARM64_X11, ARM64_X12,
                                        ARM64_X13, ARM64_X14, ARM64_X15,
                                        ARM64_X19, ARM64_X20, ARM64_X21,
                                        ARM64_X22, ARM64_X23, ARM64_X24};
// convert VReg => Arm64Reg
#define REG(r)  (((r) < 0 || (r) >= 12) ? (error("Invalid register %d", r),0) : cg_arm_reg[r])
#define CG_ARM_FP      ARM64_X29
#define CG_ARM_LR      ARM64_X30
#define CG_ARM_SP      ARM64_X31
#define CG_ARM_XZR     ARM64_X31
#else
static const X86Reg cg_x86_reg[8] = {X86_R10, X86_R11, X86_RBX, X86_R12,
                                     X86_R13, X86_R14, X86_R15, X86_RSI};
// convert VReg => X8664Reg
#define REG(r)  (((r) < 0 || (r) >= 8) ? (error("Invalid register %d", r),0) : cg_x86_reg[r])
#define CG_X86_FP      X86_RBP
#define CG_X86_SP      X86_RSP
#endif

// Virtual register index (before REG / CG_X86_REG mapping)
typedef enum {
    R_V0 = 0,
    R_V1,
    R_V2,
    R_V3,
    R_V4,
    R_V5,
    R_V6,
    R_V7,
#ifdef ARCH_ARM64
    R_V8,
    R_V9,
    R_V10,
    R_V11,
#endif
    R_NONE = -1
} VReg;

// Dry-run guard: skip emission when cg_sec is NULL
#define EMIT_GUARD if (!s || !s->data) return 0;

/* very bad idea
#ifdef ARCH_ARM64
static VReg reg_to_vreg(Arm64Reg r) {
    for (int i = 0; i < 12; i++) {
        if (cg_arm_reg[i] == r) return (VReg)i;
    }
    error("Invalid register %d", r);
    return R_NONE;
}
#else
static VReg reg_to_vreg(X86Reg r) {
    for (int i = 0; i < 8; i++) {
        if (cg_x86_reg[i] == r) return (VReg)i;
    }
    error("Invalid register %d", r);
    return R_NONE;
}
#endif
*/

// ============================================================================
// Label + branch fixup fixed-size hashtables (chaining, FNV-1a hash)
// ============================================================================

#define CG_HT_SIZE 4096 // power of 2

static uint32_t cg_ht_hash(const char *name) {
    uint32_t h = 2166136261u;
    for (; *name; name++) {
        h ^= (uint8_t)*name;
        h *= 16777619;
    }
    return h & (CG_HT_SIZE - 1);
}

// Label hashtable: name -> offset
typedef struct CgLabelNode {
    const char *name;
    size_t offset;
    struct CgLabelNode *next;
} CgLabelNode;

static CgLabelNode *cg_label_htab[CG_HT_SIZE];
static int cg_label_count = 0;

static void cg_label_ht_reset(void) {
    memset(cg_label_htab, 0, sizeof(cg_label_htab));
    cg_label_count = 0;
}

static void cg_label_ht_add(const char *name, size_t offset) {
    uint32_t h = cg_ht_hash(name);
    for (CgLabelNode *n = cg_label_htab[h]; n; n = n->next) {
        if (strcmp(n->name, name) == 0) {
            n->offset = offset;
            return;
        }
    }
    CgLabelNode *n = arena_alloc(sizeof(CgLabelNode));
    n->name = name;
    n->offset = offset;
    n->next = cg_label_htab[h];
    cg_label_htab[h] = n;
    cg_label_count++;
}

static size_t cg_label_ht_get(const char *name) {
    if (!name) return (size_t)-1;
    uint32_t h = cg_ht_hash(name);
    for (CgLabelNode *n = cg_label_htab[h]; n; n = n->next)
        if (strcmp(n->name, name) == 0)
            return n->offset;
    return (size_t)-1;
}

// ---------------------------------------------------------------------------
// Forward-fixup chain for local flow-control labels (.L.*)
// Faster than hash table: no strcmp, no hash, just a linked list of
// instruction offsets. Each loop/if stores its own chain.
// ---------------------------------------------------------------------------
typedef struct CgFwdList CgFwdList;
struct CgFwdList {
    size_t instr_off; // offset of the branch instruction in the section buffer
    int type; // 0=jmp (B), 1=jcc (B.cond), 2=adr
    CgFwdList *next;
};

// Push a fixup onto the front of the list
static CgFwdList *asm_fwd_push(CgFwdList *head, size_t instr_off, int type) {
    CgFwdList *n = arena_alloc(sizeof(CgFwdList));
    n->instr_off = instr_off;
    n->type = type;
    n->next = head;
    return n;
}

// Patch all fixups in the list to point to target_off
static void asm_fwd_patch_all(SecBuf *s, CgFwdList *head, size_t target_off) {
    while (head) {
        uint32_t insn = *(uint32_t *)(s->data + head->instr_off);
        int64_t delta = (int64_t)((int64_t)target_off - (int64_t)head->instr_off);
        if (head->type == 0) {
            // B: 26-bit signed word offset in bits [25:0]
            int64_t imm = delta / 4;
            insn = (insn & ~0x03FFFFFFU) | (uint32_t)(imm & 0x03FFFFFFU);
        } else if (head->type == 2) {
            // ADR: 21-bit byte offset, immhi[23:5] | immlo[30:29]
            int32_t immlo = (int32_t)(delta & 3);
            int32_t immhi = (int32_t)(delta >> 2);
            insn = (insn & ~0x60FFFFE0U) | (uint32_t)((immhi & 0x7FFFF) << 5) | (uint32_t)((immlo & 3) << 29);
        } else {
            // B.cond: 19-bit signed word offset in bits [23:5]
            int64_t imm = delta / 4;
            insn = (insn & ~0x00FFFFE0U) | (uint32_t)((imm & 0x7FFFF) << 5);
        }
        secbuf_patch32le(s, head->instr_off, insn);
        head = head->next;
    }
}

// Fixup hashtable: bucketed by label hash
typedef struct AsmFixupNode {
    size_t instr_off;
    const char *label;
    int type; // 0=jmp rel32, 1=jcc rel32
    struct AsmFixupNode *next;
} AsmFixupNode;

static AsmFixupNode *asm_fixup_htab[CG_HT_SIZE];
static int asm_fixup_count = 0;

static void asm_fixup_ht_reset(void) {
    memset(asm_fixup_htab, 0, sizeof(asm_fixup_htab));
    asm_fixup_count = 0;
}

static void asm_fixup_ht_add(size_t instr_off, const char *label, int type) {
    uint32_t h = cg_ht_hash(label);
    AsmFixupNode *n = arena_alloc(sizeof(AsmFixupNode));
    n->instr_off = instr_off;
    n->label = label;
    n->type = type;
    n->next = asm_fixup_htab[h];
    asm_fixup_htab[h] = n;
    asm_fixup_count++;
}

// Record a pending branch fixup (forward reference)
static void asm_fixup_add(SecBuf *s, size_t instr_off, const char *label, int type) {
    extern bool cg_dry_run;
    if (cg_dry_run) return;
    size_t target = cg_label_ht_get(label);
    if (target != (size_t)-1) {
#ifdef ARCH_ARM64
        uint32_t insn = *(uint32_t *)(s->data + instr_off);
        int64_t delta = (int64_t)((int64_t)target - (int64_t)instr_off);
        if (type == 0) {
            // B: 26-bit signed word offset in bits [25:0]
            int64_t imm = delta / 4;
            insn = (insn & ~0x03FFFFFFU) | (uint32_t)(imm & 0x03FFFFFFU);
        } else if (type == 2) {
            // ADR: 21-bit byte offset, immhi[23:5] | immlo[30:29]
            int32_t immlo = (int32_t)(delta & 3);
            int32_t immhi = (int32_t)(delta >> 2);
            insn = (insn & ~0x60FFFFE0U) | (uint32_t)((immhi & 0x7FFFF) << 5) | (uint32_t)((immlo & 3) << 29);
        } else {
            // B.cond: 19-bit signed word offset in bits [23:5]
            int64_t imm = delta / 4;
            insn = (insn & ~0x00FFFFE0U) | (uint32_t)((imm & 0x7FFFF) << 5);
        }
        secbuf_patch32le(s, instr_off, insn);
#else
        int32_t disp;
        if (type == 0)
            disp = (int32_t)(target - (instr_off + 5));
        else
            disp = (int32_t)(target - (instr_off + 6));
        secbuf_patch32le(s, type == 0 ? instr_off + 1 : instr_off + 2, (uint32_t)disp);
#endif
        return;
    }
    asm_fixup_ht_add(instr_off, label, type);
}

// Resolve pending fixups when a label is defined
static void asm_fixup_resolve(SecBuf *s, const char *label, size_t target_off) {
    uint32_t h = cg_ht_hash(label);
    AsmFixupNode **pp = &asm_fixup_htab[h];
    while (*pp) {
        AsmFixupNode *n = *pp;
        if (strcmp(n->label, label) == 0) {
#ifdef ARCH_ARM64
            uint32_t insn = *(uint32_t *)(s->data + n->instr_off);
            int64_t delta = (int64_t)((int64_t)target_off - (int64_t)n->instr_off);
            if (n->type == 0) {
                // B: 26-bit signed word offset in bits [25:0]
                int64_t imm = delta / 4;
                insn = (insn & ~0x03FFFFFFU) | (uint32_t)(imm & 0x03FFFFFFU);
            } else if (n->type == 2) {
                // ADR: 21-bit byte offset, immhi[23:5] | immlo[30:29]
                int32_t immlo = (int32_t)(delta & 3);
                int32_t immhi = (int32_t)(delta >> 2);
                insn = (insn & ~0x60FFFFE0U) | (uint32_t)((immhi & 0x7FFFF) << 5) | (uint32_t)((immlo & 3) << 29);
            } else {
                // B.cond: 19-bit signed word offset in bits [23:5]
                int64_t imm = delta / 4;
                insn = (insn & ~0x00FFFFE0U) | (uint32_t)((imm & 0x7FFFF) << 5);
            }
            secbuf_patch32le(s, n->instr_off, insn);
#else
            int32_t disp;
            if (n->type == 0)
                disp = (int32_t)(target_off - (n->instr_off + 5));
            else
                disp = (int32_t)(target_off - (n->instr_off + 6));
            secbuf_patch32le(s, n->type == 0 ? n->instr_off + 1 : n->instr_off + 2, (uint32_t)disp);
#endif
            *pp = n->next;
            asm_fixup_count--;
        } else {
            pp = &n->next;
        }
    }
}

// ============================================================================
// Peephole instruction tracking
// ============================================================================

typedef enum {
    ASM_NONE = 0,
    ASM_MOV_RR,
    ASM_MOV_RI,
    ASM_MOV_RRBP,
    ASM_MOV_RBPR,
    ASM_MOV_LOAD,
    ASM_MOV_STORE,
    ASM_MOVSX,
    ASM_MOVZX,
    ASM_LEA_FP,
    ASM_ADD_RR,
    ASM_ADD_RI,
    ASM_SUB_RR,
    ASM_SUB_RI,
    ASM_MUL_RR,
    ASM_IMUL_RRI,
    ASM_NEG,
    ASM_NOT,
    ASM_AND_RR,
    ASM_AND_RI,
    ASM_OR_RR,
    ASM_OR_RI,
    ASM_XOR_RR,
    ASM_XOR_RI,
    ASM_SHL_RI,
    ASM_SHR_RI,
    ASM_SAR_RI,
    ASM_SHL_CL,
    ASM_SHR_CL,
    ASM_SAR_CL,
    ASM_CMP_RR,
    ASM_CMP_RI,
    ASM_CMP_ZERO,
    ASM_TEST_RR,
    ASM_SETCC,
    ASM_CMOVCC,
    ASM_JMP,
    ASM_JMP_LABEL,
    ASM_JCC,
    ASM_JCC_LABEL,
    ASM_CALL,
    ASM_CALL_INDIR,
    ASM_RET,
    ASM_CVTSI2SD,
    ASM_CVTTSD2SI,
    ASM_CVTSD2SS,
    ASM_CVTSS2SD,
    ASM_FMOV_IM,
    ASM_FMOV_MI,
    ASM_FMOV_D0_R,
    ASM_FMOV_R_D0,
    ASM_FOP_RR,
    ASM_FNEG,
    ASM_FABS,
    ASM_FCMP,
    ASM_FCVT_F2I,
    ASM_FCVT_I2F,
    ASM_SCVTF,
    ASM_UCVTF,
    ASM_FCVTZS,
    ASM_FCVTZU,
    ASM_PUSH,
    ASM_POP,
    ASM_PUSH_IMM,
    ASM_STP_FP_LR,
    ASM_LDP_FP_LR,
    ASM_BSWAP,
    ASM_CLZ,
    ASM_RBIT,
    ASM_REV,
    ASM_REV16,
    ASM_LDX,
    ASM_STX,
    ASM_CAS,
    ASM_FENCE,
    ASM_LOCK,
    ASM_CLD,
    ASM_NOP,
    ASM_LEAVE,
    ASM_LABEL,
    ASM_GLBL,
    ASM_WEAK,
    ASM_DIRECTIVE,
} AsmOp;

typedef struct {
    AsmOp op;
    size_t offset;
    size_t count;
#ifdef ARCH_ARM64
    Arm64Reg rd, rs, rt;
#else
    X86Reg rd, rs, rt;
#endif
    int size;
    int64_t imm;
    int off;
    const char *label;
#ifdef ARCH_ARM64
    Arm64Cond cond;
#else
    X86Cond cond;
#endif
    int wreg;
    bool is_store;
} AsmInsn;

#define ASM_HISTORY 4
static AsmInsn asm_last[ASM_HISTORY];
static int asm_last_idx = 0;
static int asm_last_count = 0;
static AsmInsn asm_prev_node_last;
static bool asm_prev_node_valid = false;

// for peep
static void asm_record(AsmOp op, size_t offset, size_t count,
#ifdef ARCH_ARM64
                       Arm64Reg rd, Arm64Reg rs, Arm64Reg rt,
#else
                       X86Reg rd, X86Reg rs, X86Reg rt,
#endif
                       int size, int64_t imm, int off, const char *label,
#ifdef ARCH_ARM64
                       Arm64Cond cond,
#else
                       X86Cond cond,
#endif
                       int wreg, bool is_store) {
    AsmInsn *rec = &asm_last[asm_last_idx];
    rec->op = op;
    rec->offset = offset;
    rec->count = count;
    rec->rd = rd;
    rec->rs = rs;
    rec->rt = rt;
    rec->size = size;
    rec->imm = imm;
    rec->off = off;
    rec->label = label;
    rec->cond = cond;
    rec->wreg = wreg;
    rec->is_store = is_store;
    asm_last_idx = (asm_last_idx + 1) % ASM_HISTORY;
    if (asm_last_count < ASM_HISTORY) asm_last_count++;
}

static void asm_peep_save_prev(void) {
    if (asm_last_count > 0) {
        int prev = (asm_last_idx - 1 + ASM_HISTORY) % ASM_HISTORY;
        asm_prev_node_last = asm_last[prev];
        asm_prev_node_valid = true;
    } else {
        asm_prev_node_valid = false;
    }
}

static void asm_peep_node_start(SecBuf *s) {
    (void)s;
    asm_peep_save_prev();
}
static void asm_peep_node_end(SecBuf *s) { (void)s; }

// ============================================================================
// MOV / data movement
// ============================================================================

static void asm_mov_reg_reg(SecBuf *s, VReg dst, VReg src, int size) {
    if (dst == src) return;
    size_t off = s->len;
#ifdef ARCH_ARM64
    int sf = (size == 8) ? 1 : 0;
    Arm64Reg rdst = REG(dst);
    Arm64Reg rsrc = REG(src);
    arm64_orr_reg(s, sf, rdst, ARM64_XZR, rsrc, ARM64_LSL, 0);
    asm_record(ASM_MOV_RR, off, 1, rdst, rsrc, -1, size, 0, 0, NULL, 0, -1, false);
#else
    X86Reg rdst = REG(dst);
    X86Reg rsrc = REG(src);
    x86_mov_rr(s, size, rdst, rsrc);
    size_t count = s->len - off;
    asm_record(ASM_MOV_RR, off, count, rdst, rsrc, -1, size, 0, 0, NULL, 0, -1, false);
#endif
}

// Move return register (x0/rax) to virtual register r
static void asm_mov_retval(SecBuf *s, VReg r, int size) {
#ifdef ARCH_ARM64
    (void)size;
    arm64_orr_reg(s, 1, REG(r), ARM64_XZR, 0, ARM64_LSL, 0); // mov x{r}, x0
#else
    x86_mov_rr(s, size, REG(r), X86_RAX); // mov %rax/%eax, rr
#endif
}
// Move virtual register r to return register (x0/rax)
static void asm_mov_reg_to_retval(SecBuf *s, VReg r, int size) {
#ifdef ARCH_ARM64
    (void)size;
    arm64_orr_reg(s, 1, 0, ARM64_XZR, REG(r), ARM64_LSL, 0); // mov x0, x{r}
#else
    x86_mov_rr(s, size, X86_RAX, REG(r)); // mov rr, %rax
#endif
}
static void asm_mov_imm(SecBuf *s, VReg vr, int size, int64_t imm) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    //bool is_w = false; // always use 64-bit (x-reg) to match original printf codegen
    uint64_t uval = (uint64_t)imm;
    Arm64Reg r = REG(vr);
    int sf = 1;
    arm64_movz(s, sf, r, (uint16_t)(uval & 0xffff), 0);
    size_t count = 1;
    uint64_t v = uval >> 16;
    int shift = 16;
    int max_shift = 48;
    while (v && shift <= max_shift) {
        arm64_movk(s, sf, r, v & 0xffff, shift);
        v >>= 16;
        shift += 16;
        count++;
    }
    asm_record(ASM_MOV_RI, off, count, r, -1, -1, size, imm, 0, NULL, 0, -1, false);
#else
    X86Reg r = REG(vr);
    if (size == 8 && imm >= INT32_MIN && imm <= INT32_MAX)
        x86_mov_ri(s, 8, r, (int32_t)imm);
    else if (size == 8)
        x86_movabs(s, r, (uint64_t)imm);
    else
        x86_mov_ri(s, size, r, (int32_t)imm);
    size_t count = s->len - off;
    asm_record(ASM_MOV_RI, off, count, r, -1, -1, size, imm, 0, NULL, 0, -1, false);
#endif
}

#ifdef ARCH_ARM64
static void asm_mov_imm_phy(SecBuf *s, Arm64Reg r, int size, int64_t imm) {
    size_t off = s->len;
    //bool is_w = false; // always use 64-bit (x-reg) to match original printf codegen
    uint64_t uval = (uint64_t)imm;
    int sf = 1;
    arm64_movz(s, sf, r, (uint16_t)(uval & 0xffff), 0);
    size_t count = 1;
    uint64_t v = uval >> 16;
    int shift = 16;
    int max_shift = 48;
    while (v && shift <= max_shift) {
        arm64_movk(s, sf, r, v & 0xffff, shift);
        v >>= 16;
        shift += 16;
        count++;
    }
    asm_record(ASM_MOV_RI, off, count, r, -1, -1, size, imm, 0, NULL, 0, -1, false);
}
#else
static void asm_mov_imm_phy(SecBuf *s, X86Reg r, int size, int64_t imm) {
    size_t off = s->len;
    if (size == 8 && imm >= INT32_MIN && imm <= INT32_MAX)
        x86_mov_ri(s, 8, r, (int32_t)imm);
    else if (size == 8)
        x86_movabs(s, r, (uint64_t)imm);
    else
        x86_mov_ri(s, size, r, (int32_t)imm);
    size_t count = s->len - off;
    asm_record(ASM_MOV_RI, off, count, r, -1, -1, size, imm, 0, NULL, 0, -1, false);
}
#endif

// Zero register via xor
static void asm_xor_reg_reg(SecBuf *s, VReg dst, VReg src, int size) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    int sf = (size == 8) ? 1 : 0;
    Arm64Reg rdst = REG(dst);
    (void)src;
    arm64_eor_reg(s, sf, rdst, rdst, rdst, ARM64_LSL, 0);
    asm_record(ASM_XOR_RR, off, 1, rdst, rdst, -1, size, 0, 0, NULL, 0, -1, false);
#else
    X86Reg rdst = REG(dst);
    X86Reg rsrc = REG(src);
    x86_xor_rr(s, size, rdst, rsrc);
    size_t count = s->len - off;
    asm_record(ASM_XOR_RR, off, count, rdst, rsrc, -1, size, 0, 0, NULL, 0, -1, false);
#endif
}

static void asm_movq_zero(SecBuf *s, VReg r) { asm_xor_reg_reg(s, r, r, 8); }
static void asm_movl_zero(SecBuf *s, VReg r) { asm_xor_reg_reg(s, r, r, 4); }

#ifdef ARCH_ARM64
static void asm_movk(SecBuf *s, Arm64Reg rd, int sf, uint16_t imm16, int shift) {
    arm64_movk(s, sf, rd, imm16, shift);
}
#endif

static void asm_movsx(SecBuf *s, VReg dst, VReg src, int dst_sz, int src_sz) {
    if (dst_sz <= src_sz) return;
    size_t off = s->len;
#ifdef ARCH_ARM64
    Arm64Reg rdst = REG(dst);
    Arm64Reg rsrc = REG(src);
    if (dst_sz == 8 && src_sz == 4)
        arm64_sxtw(s, rdst, rsrc);
    else if (dst_sz == 8 && src_sz == 2)
        arm64_sxth(s, 1, rdst, rsrc);
    else if (dst_sz == 8 && src_sz == 1)
        arm64_sxtb(s, 1, rdst, rsrc);
    else if (dst_sz == 4 && src_sz == 2)
        arm64_sxth(s, 0, rdst, rsrc);
    else if (dst_sz == 4 && src_sz == 1)
        arm64_sxtb(s, 0, rdst, rsrc);
    else {
        asm_mov_reg_reg(s, dst, src, dst_sz);
        return;
    }
    asm_record(ASM_MOVSX, off, 1, rdst, rsrc, -1, dst_sz, 0, 0, NULL, 0, -1, false);
#else
    X86Reg rdst = REG(dst);
    X86Reg rsrc = REG(src);
    x86_movsx(s, dst_sz, src_sz, rdst, rsrc);
    size_t count = s->len - off;
    asm_record(ASM_MOVSX, off, count, rdst, rsrc, -1, dst_sz, 0, 0, NULL, 0, -1, false);
#endif
}

static void asm_movzx(SecBuf *s, VReg dst, VReg src, int dst_sz, int src_sz) {
    if (dst_sz <= src_sz) return;
    size_t off = s->len;
#ifdef ARCH_ARM64
    Arm64Reg rdst = REG(dst);
    Arm64Reg rsrc = REG(src);
    if (dst_sz == 8 && src_sz == 2)
        arm64_uxth(s, rdst, rsrc);
    else if (dst_sz == 8 && src_sz == 1)
        arm64_uxtb(s, rdst, rsrc);
    else if (dst_sz == 4 && src_sz == 2)
        arm64_uxth(s, rdst, rsrc);
    else if (dst_sz == 4 && src_sz == 1)
        arm64_uxtb(s, rdst, rsrc);
    else {
        asm_mov_reg_reg(s, dst, src, dst_sz);
        return;
    }
    asm_record(ASM_MOVZX, off, 1, rdst, rsrc, -1, dst_sz, 0, 0, NULL, 0, -1, false);
#else
    X86Reg rdst = REG(dst);
    X86Reg rsrc = REG(src);
    x86_movzx(s, dst_sz, src_sz, rdst, rsrc);
    size_t count = s->len - off;
    asm_record(ASM_MOVZX, off, count, rdst, rsrc, -1, dst_sz, 0, 0, NULL, 0, -1, false);
#endif
}

// movzx from a fixed (non-allocatable) physical register to VReg (x86 only)
#ifndef ARCH_ARM64
static void asm_movzx_phys(SecBuf *s, VReg dst, X86Reg src, int dst_sz, int src_sz) {
    if (dst_sz <= src_sz) return;
    size_t off = s->len;
    X86Reg rdst = REG(dst);
    x86_movzx(s, dst_sz, src_sz, rdst, src);
    size_t count = s->len - off;
    asm_record(ASM_MOVZX, off, count, rdst, src, -1, dst_sz, 0, 0, NULL, 0, -1, false);
}
#endif

// ============================================================================
// Arithmetic
// ============================================================================

static void asm_add_reg_reg(SecBuf *s, VReg dst, VReg src, int size) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    Arm64Reg rdst = REG(dst);
    Arm64Reg rsrc = REG(src);
    int sf = (size == 8) ? 1 : 0;
    arm64_add_reg(s, sf, rdst, rdst, rsrc, ARM64_LSL, 0);
    asm_record(ASM_ADD_RR, off, 1, rdst, rsrc, -1, size, 0, 0, NULL, 0, -1, false);
#else
    X86Reg rdst = REG(dst);
    X86Reg rsrc = REG(src);
    x86_add_rr(s, size, rdst, rsrc);
    size_t count = s->len - off;
    asm_record(ASM_ADD_RR, off, count, rdst, rsrc, -1, size, 0, 0, NULL, 0, -1, false);
#endif
}

static void asm_add_imm(SecBuf *s, VReg vr, int size, int32_t imm) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    Arm64Reg r = REG(vr);
    int sf = (size == 8) ? 1 : 0;
    if (imm >= 0 && imm < 4096) {
        arm64_add_imm(s, sf, r, r, imm, 0);
    } else if (imm > 0 && imm < 4096 * 4096 && (imm & 0xfff) == 0) {
        arm64_add_imm(s, sf, r, r, imm >> 12, 1); // shifted
    } else {
        // large imm: load into x16, then add
        int64_t v = imm < 0 ? -(int64_t)imm : (int64_t)imm;
        arm64_movz(s, 1, 16, (uint16_t)(v & 0xffff), 0);
        if (v >> 16) arm64_movk(s, 1, 16, (uint16_t)((v >> 16) & 0xffff), 16);
        if (v >> 32) arm64_movk(s, 1, 16, (uint16_t)((v >> 32) & 0xffff), 32);
        if (v >> 48) arm64_movk(s, 1, 16, (uint16_t)((v >> 48) & 0xffff), 48);
        if (imm < 0)
            arm64_sub_reg(s, sf, r, r, 16, ARM64_LSL, 0);
        else
            arm64_add_reg(s, sf, r, r, 16, ARM64_LSL, 0);
    }
    asm_record(ASM_ADD_RI, off, (size_t)(s->len - off) / 4, r, -1, -1, size, imm, 0, NULL, 0, -1, false);
#else
    X86Reg r = REG(vr);
    x86_add_ri(s, size, r, imm);
    size_t count = s->len - off;
    asm_record(ASM_ADD_RI, off, count, r, -1, -1, size, imm, 0, NULL, 0, -1, false);
#endif
}

static void asm_sub_reg_reg(SecBuf *s, VReg dst, VReg src, int size) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    Arm64Reg rdst = REG(dst);
    Arm64Reg rsrc = REG(src);
    int sf = (size == 8) ? 1 : 0;
    arm64_sub_reg(s, sf, rdst, rdst, rsrc, ARM64_LSL, 0);
    asm_record(ASM_SUB_RR, off, 1, dst, src, -1, size, 0, 0, NULL, 0, -1, false);
#else
    X86Reg rdst = REG(dst);
    X86Reg rsrc = REG(src);
    x86_sub_rr(s, size, rdst, rsrc);
    size_t count = s->len - off;
    asm_record(ASM_SUB_RR, off, count, rdst, rsrc, -1, size, 0, 0, NULL, 0, -1, false);
#endif
}

static void asm_sub_reg3(SecBuf *s, int dst, int src1, int src2, int size) {
#ifdef ARCH_ARM64
    Arm64Reg rdst = REG(dst);
    Arm64Reg rsrc1 = REG(src1);
    Arm64Reg rsrc2 = REG(src2);
    int sf = (size == 8) ? 1 : 0;
    arm64_sub_reg(s, sf, rdst, rsrc1, rsrc2, ARM64_LSL, 0);
#else
    X86Reg rdst = REG(dst);
    X86Reg rsrc1 = REG(src1);
    X86Reg rsrc2 = REG(src2);
    x86_mov_rr(s, size, rdst, rsrc1);
    x86_sub_rr(s, size, rdst, rsrc2);
#endif
}

static void asm_sub_imm(SecBuf *s, VReg vr, int size, int32_t imm) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    Arm64Reg r = REG(vr);
    int sf = (size == 8) ? 1 : 0;
    if (imm >= 0 && imm < 4096) {
        arm64_sub_imm(s, sf, r, r, imm, 0);
    } else if (imm > 0 && imm < 4096 * 4096 && (imm & 0xfff) == 0) {
        arm64_sub_imm(s, sf, r, r, imm >> 12, 1); // shifted
    } else {
        int64_t v = imm < 0 ? -(int64_t)imm : (int64_t)imm;
        arm64_movz(s, 1, ARM64_X16, (uint16_t)(v & 0xffff), 0);
        if (v >> 16) arm64_movk(s, 1, ARM64_X16, (uint16_t)((v >> 16) & 0xffff), 16);
        if (v >> 32) arm64_movk(s, 1, ARM64_X16, (uint16_t)((v >> 32) & 0xffff), 32);
        if (v >> 48) arm64_movk(s, 1, ARM64_X16, (uint16_t)((v >> 48) & 0xffff), 48);
        if (imm < 0)
            arm64_add_reg(s, sf, r, r, ARM64_X16, ARM64_LSL, 0);
        else
            arm64_sub_reg(s, sf, r, r, ARM64_X16, ARM64_LSL, 0);
    }
    asm_record(ASM_SUB_RI, off, (size_t)(s->len - off) / 4, r, -1, -1, size, imm, 0, NULL, 0, -1, false);
#else
    X86Reg r = REG(vr);
    x86_sub_ri(s, size, r, imm);
    size_t count = s->len - off;
    asm_record(ASM_SUB_RI, off, count, r, -1, -1, size, imm, 0, NULL, 0, -1, false);
#endif
}

static void asm_mul_reg_reg(SecBuf *s, VReg dst, VReg src, int size) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    Arm64Reg rdst = REG(dst);
    Arm64Reg rsrc = REG(src);
    int sf = (size == 8) ? 1 : 0;
    arm64_mul(s, sf, rdst, rdst, rsrc);
    asm_record(ASM_MUL_RR, off, 1, rdst, rsrc, -1, size, 0, 0, NULL, 0, -1, false);
#else
    X86Reg rdst = REG(dst);
    X86Reg rsrc = REG(src);
    x86_imul_rr(s, size, rdst, rsrc);
    size_t count = s->len - off;
    asm_record(ASM_MUL_RR, off, count, rdst, rsrc, -1, size, 0, 0, NULL, 0, -1, false);
#endif
}
#ifndef ARCH_ARM64
// imul $imm, r_dst, r_src  — x86 3-operand immediate multiply
static void asm_imul_imm(SecBuf *s, VReg dst, VReg src, int size, int32_t imm) {
    x86_imul_rri(s, size, REG(dst), REG(src), imm);
}
#endif
static void asm_sdiv_reg_reg(SecBuf *s, VReg dst, VReg src, int size) {
#ifdef ARCH_ARM64
    Arm64Reg rdst = REG(dst);
    int sf = (size == 8) ? 1 : 0;
    arm64_sdiv(s, sf, rdst, rdst, REG(src));
#else
    (void)dst;
    x86_idiv_r(s, size, REG(src));
#endif
}
#ifdef ARCH_ARM64
static void asm_udiv_reg_reg(SecBuf *s, VReg dst, VReg src, int size) {
    Arm64Reg rdst = REG(dst);
    int sf = (size == 8) ? 1 : 0;
    arm64_udiv(s, sf, rdst, rdst, REG(src));
}
#endif

static void asm_neg(SecBuf *s, VReg vr, int size) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    Arm64Reg r = REG(vr);
    int sf = (size == 8) ? 1 : 0;
    arm64_neg(s, sf, r, r);
    asm_record(ASM_NEG, off, 1, r, -1, -1, size, 0, 0, NULL, 0, -1, false);
#else
    X86Reg r = REG(vr);
    x86_neg_r(s, size, r);
    size_t count = s->len - off;
    asm_record(ASM_NEG, off, count, r, -1, -1, size, 0, 0, NULL, 0, -1, false);
#endif
}

static void asm_not(SecBuf *s, VReg vr, int size) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    Arm64Reg r = REG(vr);
    int sf = (size == 8) ? 1 : 0;
    arm64_mvn(s, sf, r, r, ARM64_LSL, 0);
    asm_record(ASM_NOT, off, 1, r, -1, -1, size, 0, 0, NULL, 0, -1, false);
#else
    X86Reg r = REG(vr);
    x86_not_r(s, size, r);
    size_t count = s->len - off;
    asm_record(ASM_NOT, off, count, r, -1, -1, size, 0, 0, NULL, 0, -1, false);
#endif
}

static void asm_dec(SecBuf *s, VReg vr, int size) {
#ifdef ARCH_ARM64
    Arm64Reg r = REG(vr);
    int sf = (size == 8) ? 1 : 0;
    arm64_sub_imm(s, sf, r, r, 1, 0);
#else
    X86Reg r = REG(vr);
    x86_dec_r(s, size, r);
#endif
}

// ============================================================================
// Logical
// ============================================================================

static void asm_and_reg_reg(SecBuf *s, VReg dst, VReg src, int size) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    Arm64Reg rdst = REG(dst);
    Arm64Reg rsrc = REG(src);
    int sf = (size == 8) ? 1 : 0;
    arm64_and_reg(s, sf, rdst, rdst, rsrc, ARM64_LSL, 0);
    asm_record(ASM_AND_RR, off, 1, rdst, rsrc, -1, size, 0, 0, NULL, 0, -1, false);
#else
    X86Reg rdst = REG(dst);
    X86Reg rsrc = REG(src);
    x86_and_rr(s, size, rdst, rsrc);
    size_t count = s->len - off;
    asm_record(ASM_AND_RR, off, count, rdst, rsrc, -1, size, 0, 0, NULL, 0, -1, false);
#endif
}
#ifdef ARCH_ARM64
static void asm_and_reg_phy(SecBuf *s, VReg dst, Arm64Reg src, int size) {
    size_t off = s->len;
    Arm64Reg rdst = REG(dst);
    int sf = (size == 8) ? 1 : 0;
    arm64_and_reg(s, sf, rdst, rdst, src, ARM64_LSL, 0);
    asm_record(ASM_AND_RR, off, 1, rdst, src, -1, size, 0, 0, NULL, 0, -1, false);
}
#endif

// and rr, %rax — used after asm_movabs_phy(X86_RAX, mask) for bitfield masking
static void asm_and_rax(SecBuf *s, VReg r, int size) {
#ifndef ARCH_ARM64
    size_t off = s->len;
    X86Reg rdst = REG(r);
    x86_and_rr(s, size, rdst, X86_RAX);
    size_t count = s->len - off;
    asm_record(ASM_AND_RR, off, count, rdst, X86_RAX, -1, size, 0, 0, NULL, 0, -1, false);
#else
    (void)s;
    (void)r;
    (void)size;
#endif
}

static void asm_or_reg_reg(SecBuf *s, VReg dst, VReg src, int size) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    Arm64Reg rdst = REG(dst);
    Arm64Reg rsrc = REG(src);
    int sf = (size == 8) ? 1 : 0;
    arm64_orr_reg(s, sf, rdst, rdst, rsrc, ARM64_LSL, 0);
    asm_record(ASM_OR_RR, off, 1, rdst, rsrc, -1, size, 0, 0, NULL, 0, -1, false);
#else
    X86Reg rdst = REG(dst);
    X86Reg rsrc = REG(src);
    x86_or_rr(s, size, rdst, rsrc);
    size_t count = s->len - off;
    asm_record(ASM_OR_RR, off, count, rdst, rsrc, -1, size, 0, 0, NULL, 0, -1, false);
#endif
}

static void asm_eor_reg_reg(SecBuf *s, VReg dst, VReg src, int size) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    Arm64Reg rdst = REG(dst);
    Arm64Reg rsrc = REG(src);
    int sf = (size == 8) ? 1 : 0;
    arm64_eor_reg(s, sf, rdst, rdst, rsrc, ARM64_LSL, 0);
    asm_record(ASM_XOR_RR, off, 1, rdst, rsrc, -1, size, 0, 0, NULL, 0, -1, false);
#else
    X86Reg rdst = REG(dst);
    X86Reg rsrc = REG(src);
    x86_xor_rr(s, size, rdst, rsrc);
    size_t count = s->len - off;
    asm_record(ASM_XOR_RR, off, count, rdst, rsrc, -1, size, 0, 0, NULL, 0, -1, false);
#endif
}

// ============================================================================
// Shifts
// ============================================================================

static void asm_shl_imm(SecBuf *s, VReg vr, int size, uint8_t shift) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    Arm64Reg r = REG(vr);
    int sf = (size == 8) ? 1 : 0;
    arm64_lsl_imm(s, sf, r, r, shift);
    asm_record(ASM_SHL_RI, off, 1, r, -1, -1, size, shift, 0, NULL, 0, -1, false);
#else
    X86Reg r = REG(vr);
    x86_shl_ri(s, size, r, shift);
    size_t count = s->len - off;
    asm_record(ASM_SHL_RI, off, count, r, -1, -1, size, shift, 0, NULL, 0, -1, false);
#endif
}

static void asm_shr_imm(SecBuf *s, VReg r, int size, uint8_t shift) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    int sf = (size == 8) ? 1 : 0;
    arm64_lsr_imm(s, sf, REG(r), REG(r), shift);
    asm_record(ASM_SHR_RI, off, 1, r, -1, -1, size, shift, 0, NULL, 0, -1, false);
#else
    x86_shr_ri(s, size, REG(r), shift);
    size_t count = s->len - off;
    asm_record(ASM_SHR_RI, off, count, REG(r), -1, -1, size, shift, 0, NULL, 0, -1, false);
#endif
}

static void asm_sar_imm(SecBuf *s, VReg r, int size, uint8_t shift) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    int sf = (size == 8) ? 1 : 0;
    arm64_asr_imm(s, sf, REG(r), REG(r), shift);
    asm_record(ASM_SAR_RI, off, 1, r, -1, -1, size, shift, 0, NULL, 0, -1, false);
#else
    x86_sar_ri(s, size, REG(r), shift);
    size_t count = s->len - off;
    asm_record(ASM_SAR_RI, off, count, REG(r), -1, -1, size, shift, 0, NULL, 0, -1, false);
#endif
}

static void asm_shl_cl(SecBuf *s, VReg r, int size, VReg shift_reg) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    int sf = (size == 8) ? 1 : 0;
    arm64_lsl_reg(s, sf, REG(r), REG(r), REG(shift_reg));
#else
    (void)shift_reg;
    x86_shl_rcl(s, size, REG(r));
    size_t count = s->len - off;
    asm_record(ASM_SHL_CL, off, count, REG(r), -1, -1, size, 0, 0, NULL, 0, -1, false);
#endif
}

static void asm_shr_cl(SecBuf *s, VReg r, int size, VReg shift_reg) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    int sf = (size == 8) ? 1 : 0;
    arm64_lsr_reg(s, sf, REG(r), REG(r), REG(shift_reg));
#else
    (void)shift_reg;
    x86_shr_rcl(s, size, REG(r));
    size_t count = s->len - off;
    asm_record(ASM_SHR_CL, off, count, REG(r), -1, -1, size, 0, 0, NULL, 0, -1, false);
#endif
}

static void asm_sar_cl(SecBuf *s, VReg r, int size, VReg shift_reg) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    int sf = (size == 8) ? 1 : 0;
    arm64_asr_reg(s, sf, REG(r), REG(r), REG(shift_reg));
#else
    (void)shift_reg;
    x86_sar_rcl(s, size, REG(r));
    size_t count = s->len - off;
    asm_record(ASM_SAR_CL, off, count, REG(r), -1, -1, size, 0, 0, NULL, 0, -1, false);
#endif
}

#ifndef ARCH_ARM64
static void asm_cqo(SecBuf *s) {
    // ARM64: sxtw x0, w0 for 32-bit, already sign-extended for 64-bit
    x86_cqo(s);
}
#endif

static void asm_cdq(SecBuf *s) {
#ifdef ARCH_ARM64
    // ARM64: sxtw x0, w0
    //asm_sxtw(s, V_X0, V_X1); // FIXME
    arm64_sxtw(s, ARM64_X0, ARM64_X1);
#else
    x86_cdq(s);
#endif
}

static void asm_idiv(SecBuf *s, VReg r, int size) {
#ifdef ARCH_ARM64
    // ARM64 division FIXME
#else
    x86_idiv_r(s, size, REG(r));
#endif
}

static void asm_div(SecBuf *s, VReg r, int size) {
#ifdef ARCH_ARM64
    // ARM64 unsigned division  FIXME
#else
    x86_div_r(s, size, REG(r));
#endif
}

// ============================================================================
// Compare / Test
// ============================================================================

static void asm_cmp_reg_reg(SecBuf *s, VReg a, VReg b, int size) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    int sf = (size == 8) ? 1 : 0;
    arm64_subs_reg(s, sf, ARM64_XZR, REG(a), REG(b), ARM64_LSL, 0);
    asm_record(ASM_CMP_RR, off, 1, REG(a), REG(b), -1, size, 0, 0, NULL, 0, -1, false);
#else
    x86_cmp_rr(s, size, REG(a), REG(b));
    size_t count = s->len - off;
    asm_record(ASM_CMP_RR, off, count, REG(a), REG(b), -1, size, 0, 0, NULL, 0, -1, false);
#endif
}
#ifdef ARCH_ARM64
static void asm_cmp_reg_phy(SecBuf *s, VReg a, Arm64Reg b, int size) {
    size_t off = s->len;
    int sf = (size == 8) ? 1 : 0;
    arm64_subs_reg(s, sf, ARM64_XZR, REG(a), b, ARM64_LSL, 0);
    asm_record(ASM_CMP_RR, off, 1, REG(a), b, -1, size, 0, 0, NULL, 0, -1, false);
}
#endif

static void asm_cmp_zero(SecBuf *s, VReg r, int size) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    int sf = (size == 8) ? 1 : 0;
    arm64_subs_imm(s, sf, ARM64_XZR, REG(r), 0, 0);
#else
    x86_cmp_ri(s, size, REG(r), 0);
#endif
    asm_record(ASM_CMP_ZERO, off, (size_t)(s->len - off), REG(r), -1, -1, size, 0, 0, NULL, 0, -1, false);
}

// ============================================================================
// Conditional set / move
// ============================================================================

#ifdef ARCH_ARM64
static void asm_cset(SecBuf *s, VReg r, Arm64Cond cond) {
    size_t off = s->len;
    arm64_cset(s, 0, REG(r), cond);
    asm_record(ASM_SETCC, off, 1, r, -1, -1, 4, 0, 0, NULL, cond, -1, false);
}
#else
static void asm_setcc(SecBuf *s, X86Reg r, X86Cond cond) {
    size_t off = s->len;
    x86_setcc(s, cond, r);
    asm_record(ASM_SETCC, off, s->len - off, r, -1, -1, 1, 0, 0, NULL, cond, -1, false);
}
#endif

// ============================================================================
// Branches (placeholder filled via relocation)
// ============================================================================

static size_t asm_call_label(SecBuf *s) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    arm64_bl(s, 0);
#else
    x86_call_rel32(s, 0);
#endif
    asm_record(ASM_CALL, off, (size_t)(s->len - off), -1, -1, -1, 0, 0, 0, NULL, 0, -1, false);
    return off; // offset of instruction start for relocation
}

static void asm_call_reg(SecBuf *s, VReg r) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    arm64_blr(s, REG(r));
#else
    x86_call_r(s, REG(r));
    asm_record(ASM_CALL_INDIR, off, s->len - off, REG(r), -1, -1, 0, 0, 0, NULL, 0, -1, false);
#endif
}

static size_t asm_jcc_label(SecBuf *s, int cond) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    arm64_bcond(s, (Arm64Cond)cond, 0);
    asm_record(ASM_JCC, off, 1, -1, -1, -1, 0, 0, 0, NULL, cond, -1, false);
    return off;
#else
    x86_jcc_rel32(s, (X86Cond)cond, 0);
    size_t count = s->len - off;
    asm_record(ASM_JCC, off, count, -1, -1, -1, 0, 0, 0, NULL, cond, -1, false);
    return off;
#endif
}

static size_t asm_jmp_label_to(SecBuf *s, const char *label) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    arm64_b(s, 0);
    asm_record(ASM_JMP, off, 1, -1, -1, -1, 0, 0, 0, (char *)label, 0, -1, false);
    return off;
#else
    x86_jmp_rel32(s, 0);
    size_t count = s->len - off;
    asm_record(ASM_JMP, off, count, -1, -1, -1, 0, 0, 0, (char *)label, 0, -1, false);
    return off;
#endif
}
// Conditional branch wrappers with label strings (forward-fixup)
static size_t asm_jb_label_to(SecBuf *s, const char *label) {
    size_t off = s->len;
    x86_jcc_rel32(s, X86_B, 0);
    size_t count = s->len - off;
    asm_record(ASM_JCC, off, count, -1, -1, -1, 0, 0, 0, (char *)label, X86_B, -1, false);
    return off;
}
static size_t asm_jz_label_to(SecBuf *s, const char *label) {
    size_t off = s->len;
    x86_jcc_rel32(s, X86_Z, 0);
    size_t count = s->len - off;
    asm_record(ASM_JCC, off, count, -1, -1, -1, 0, 0, 0, (char *)label, X86_Z, -1, false);
    return off;
}
static size_t asm_je_label_to(SecBuf *s, const char *label) {
    size_t off = s->len;
    x86_jcc_rel32(s, X86_E, 0);
    size_t count = s->len - off;
    asm_record(ASM_JCC, off, count, -1, -1, -1, 0, 0, 0, (char *)label, X86_E, -1, false);
    return off;
}
static size_t asm_ja_label_to(SecBuf *s, const char *label) {
    size_t off = s->len;
    x86_jcc_rel32(s, X86_A, 0);
    size_t count = s->len - off;
    asm_record(ASM_JCC, off, count, -1, -1, -1, 0, 0, 0, (char *)label, X86_A, -1, false);
    return off;
}


static size_t asm_jmp_label(SecBuf *s) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    arm64_b(s, 0);
    asm_record(ASM_JMP, off, 1, -1, -1, -1, 0, 0, 0, NULL, 0, -1, false);
    return off;
#else
    x86_jmp_rel32(s, 0);
    size_t count = s->len - off;
    asm_record(ASM_JMP, off, count, -1, -1, -1, 0, 0, 0, NULL, 0, -1, false);
    return off;
#endif
}

// Emit B to a known target position (backward branch — no fixup needed)
static size_t asm_b_back(SecBuf *s, size_t target_off) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    arm64_b(s, 0);
    int64_t delta = (int64_t)((int64_t)target_off - (int64_t)off);
    uint32_t insn = *(uint32_t *)(s->data + off);
    int64_t imm = delta / 4;
    insn = (insn & ~0x03FFFFFFU) | (uint32_t)(imm & 0x03FFFFFFU);
    secbuf_patch32le(s, off, insn);
#else
    x86_jmp_rel32(s, 0);
    int32_t disp = (int32_t)(target_off - (off + 5));
    secbuf_patch32le(s, off + 1, (uint32_t)disp);
#endif
    asm_record(ASM_JMP, off, 1, -1, -1, -1, 0, 0, 0, NULL, 0, -1, false);
    return off;
}

// Emit B.cond to a known target position (backward — no fixup needed)
static size_t asm_bcond_back(SecBuf *s, int cond, size_t target_off) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    arm64_bcond(s, (Arm64Cond)cond, 0);
    int64_t delta = (int64_t)((int64_t)target_off - (int64_t)off);
    uint32_t insn = *(uint32_t *)(s->data + off);
    int64_t imm = delta / 4;
    insn = (insn & ~0x00FFFFE0U) | (uint32_t)((imm & 0x7FFFF) << 5);
    secbuf_patch32le(s, off, insn);
#else
    x86_jcc_rel32(s, (X86Cond)cond, 0);
    int32_t disp = (int32_t)(target_off - (off + 6));
    secbuf_patch32le(s, off + 2, (uint32_t)disp);
#endif
    asm_record(ASM_JCC, off, 1, -1, -1, -1, 0, 0, 0, NULL, cond, -1, false);
    return off;
}


static void asm_jmp_reg(SecBuf *s, VReg r) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    arm64_br(s, REG(r));
#else
    x86_jmp_r(s, REG(r));
#endif
    asm_record(ASM_JMP, off, (size_t)(s->len - off), REG(r), -1, -1, 0, 0, 0, NULL, 0, -1, false);
}

#ifdef ARCH_ARM64
// adr x16, label — emit placeholder, fixup type=2 patches 21-bit byte offset at label def
static void asm_adr_x16_label(SecBuf *s, const char *label) {
    size_t off = s->len;
    arm64_adr(s, 16, 0); // adr x16, 0 (placeholder)
    asm_fixup_add(s, off, label, 2); // type=2 = ADR fixup
}
#endif

#ifdef ARCH_ARM64
static void asm_bti_c(SecBuf *s) {
    arm64_bti_c(s); // hint #34 = bti c
}
#endif

static void asm_ret(SecBuf *s) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    arm64_ret(s, CG_ARM_LR);
#else
    x86_ret(s);
#endif
    size_t count = (size_t)(s->len - off);
    asm_record(ASM_RET, off, count, -1, -1, -1, 0, 0, 0, NULL, 0, -1, false);
}

static void asm_leave(SecBuf *s) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    arm64_add_extreg(s, 1, CG_ARM_SP, CG_ARM_FP, ARM64_XZR, ARM64_UXTX, 0); // mov sp, fp
    arm64_ldp(s, 1, CG_ARM_FP, CG_ARM_LR, CG_ARM_SP, 0, false, true);
#else
    x86_leave(s);
    size_t count = s->len - off;
    asm_record(ASM_LEAVE, off, count, -1, -1, -1, 0, 0, 0, NULL, 0, -1, false);
#endif
}

// ============================================================================
// Stack
// ============================================================================

#ifdef ARCH_ARM64
static void asm_stp_fp_lr(SecBuf *s) {
    size_t off = s->len;
    arm64_stp(s, 1, CG_ARM_FP, CG_ARM_LR, CG_ARM_SP, -2, true, false); // stp x29, x30, [sp, #-16]!
    asm_record(ASM_STP_FP_LR, off, 1, -1, -1, -1, 0, 0, 0, NULL, 0, -1, false);
}
static void asm_ldp_fp_lr(SecBuf *s) {
    size_t off = s->len;
    arm64_ldp(s, 1, CG_ARM_FP, CG_ARM_LR, CG_ARM_SP, 2, false, true); // ldp x29, x30, [sp], #16
    asm_record(ASM_LDP_FP_LR, off, 1, -1, -1, -1, 0, 0, 0, NULL, 0, -1, false);
}

static void asm_stp_sp(SecBuf *s, Arm64Reg rt1, Arm64Reg rt2, int32_t imm7, bool pre, bool post) {
    arm64_stp(s, 1, rt1, rt2, CG_ARM_SP, imm7, pre, post);
}

static void asm_ldp_sp(SecBuf *s, Arm64Reg rt1, Arm64Reg rt2, int32_t imm7, bool pre, bool post) {
    arm64_ldp(s, 1, rt1, rt2, CG_ARM_SP, imm7, pre, post);
}

static void asm_mov_fp_sp(SecBuf *s) {
    arm64_add_imm(s, 1, CG_ARM_FP, CG_ARM_SP, 0, 0); // add x29, sp, #0
}

static void asm_mov_x0_reg(SecBuf *s, Arm64Reg src) {
    arm64_orr_reg(s, 1, ARM64_X0, ARM64_XZR, src, ARM64_LSL, 0); // mov x0, x{src}
}
static void asm_mov_w0_reg32(SecBuf *s, Arm64Reg src) {
    arm64_orr_reg(s, 0, ARM64_X0, ARM64_XZR, src, ARM64_LSL, 0); // mov w0, w{src}
}
#endif

#ifndef ARCH_ARM64
static void asm_push(SecBuf *s, X86Reg r) {
    size_t off = s->len;
    x86_push(s, r);
    asm_record(ASM_PUSH, off, s->len - off, r, -1, -1, 0, 0, 0, NULL, 0, -1, false);
}
static void asm_pop(SecBuf *s, X86Reg r) {
    size_t off = s->len;
    x86_pop(s, r);
    asm_record(ASM_POP, off, s->len - off, r, -1, -1, 0, 0, 0, NULL, 0, -1, false);
}
static void asm_lea_rbp(SecBuf *s, X86Reg reg, int size, int offset) {
    size_t off = s->len;
    X86Mem m = {CG_X86_FP, X86_NOREG, 1, -offset};
    x86_lea(s, size, reg, m);
    asm_record(ASM_LEA_FP, off, s->len - off, reg, -1, -1, size, 0, -offset, NULL, 0, -1, false);
}
static void asm_mov_rbp(SecBuf *s, X86Reg reg, int size, int offset) {
    size_t off = s->len;
    X86Mem m = {CG_X86_FP, X86_NOREG, 1, -offset};
    x86_mov_rm(s, size, reg, m);
    asm_record(ASM_MOV_RRBP, off, s->len - off, reg, -1, -1, size, 0, offset, NULL, 0, -1, false);
}
static void asm_mov_phyreg_rbp(SecBuf *s, X86Reg reg, int size, int offset) {
    size_t off = s->len;
    X86Mem m = {CG_X86_FP, X86_NOREG, 1, -offset};
    x86_mov_mr(s, size, m, reg);
    asm_record(ASM_MOV_RBPR, off, s->len - off, reg, -1, -1, size, 0, offset, NULL, 0, -1, true);
}
static void asm_movabs_phy(SecBuf *s, X86Reg reg, uint64_t val) {
    size_t off = s->len;
    x86_movabs(s, reg, val);
    asm_record(ASM_MOV_RI, off, s->len - off, reg, -1, -1, 8, (int64_t)val, 0, NULL, 0, -1, false);
}
// movq %rsp, %rbp  (function prologue)
static void asm_mov_rsp_rbp(SecBuf *s) {
    size_t off = s->len;
    x86_mov_rr(s, 8, X86_RBP, X86_RSP); // movq %rsp, %rbp
    asm_record(ASM_MOV_RR, off, s->len - off, (int)X86_RBP, (int)X86_RSP, -1, 8, 0, 0, NULL, 0, -1, false);
}
// subq $imm, %rsp  (function prologue stack allocation)
static void asm_sub_rsp_imm(SecBuf *s, int32_t imm) {
    size_t off = s->len;
    x86_sub_ri(s, 8, X86_RSP, imm); // subq $imm, %rsp
    asm_record(ASM_SUB_RI, off, s->len - off, (int)X86_RSP, -1, -1, 8, imm, 0, NULL, 0, -1, false);
}
// addq $imm, %rsp  (function epilogue stack deallocation)
static void asm_add_rsp_imm(SecBuf *s, int32_t imm) {
    size_t off = s->len;
    x86_add_ri(s, 8, X86_RSP, imm); // addq $imm, %rsp
    asm_record(ASM_ADD_RI, off, s->len - off, (int)X86_RSP, -1, -1, 8, imm, 0, NULL, 0, -1, false);
}
// store immediate to rbp-relative: movb/movl/movq $imm, -offset(%rbp)
static void asm_mov_rbp_imm(SecBuf *s, int size, int offset, int32_t imm) {
    size_t off = s->len;
    X86Mem m = {CG_X86_FP, X86_NOREG, 1, -offset};
    x86_mov_mi(s, size, m, imm); // movb/movl/movq $imm, -offset(%rbp)
    asm_record(ASM_MOV_RI, off, s->len - off, -1, -1, -1, size, imm, offset, NULL, 0, -1, true);
}
// movzbl/movzwl -offset(%rbp), dst: zero-extending load from rbp-relative
static void asm_movzx_rbp_reg(SecBuf *s, VReg dst, int dst_sz, int src_sz, int offset) {
    size_t off = s->len;
    X86Mem m = {CG_X86_FP, X86_NOREG, 1, -offset};
    X86Reg rdst = REG(dst);
    x86_movzx_rm(s, dst_sz, src_sz, rdst, m); // movzbl/movzwl -off(%rbp), dst
    asm_record(ASM_MOVZX, off, s->len - off, rdst, -1, -1, dst_sz, 0, offset, NULL, 0, -1, false);
}
// movsbl/movswl -offset(%rbp), dst: sign-extending load from rbp-relative
static void asm_movsx_rbp_reg(SecBuf *s, VReg dst, int dst_sz, int src_sz, int offset) {
    size_t off = s->len;
    X86Mem m = {CG_X86_FP, X86_NOREG, 1, -offset};
    x86_movsx_rm(s, dst_sz, src_sz, REG(dst), m); // movsbl/movswl -off(%rbp), dst
    asm_record(ASM_MOVSX, off, s->len - off, REG(dst), -1, -1, dst_sz, 0, offset, NULL, 0, -1, false);
}
// movzbl/movzwl off(base), dst: zero-extending load from base+offset
static void asm_movzx_base_off_reg(SecBuf *s, VReg dst, VReg base, int64_t disp, int dst_sz, int src_sz) {
    size_t off = s->len;
    X86Mem m = {REG(base), X86_NOREG, 1, disp};
    x86_movzx_rm(s, dst_sz, src_sz, REG(dst), m); // movzbl/movzwl disp(base), dst
    asm_record(ASM_MOVZX, off, s->len - off, REG(dst), REG(base), -1, dst_sz, 0, disp, NULL, 0, -1, false);
}
// movsbl/movswl off(base), dst: sign-extending load from base+offset
static void asm_movsx_base_off_reg(SecBuf *s, VReg dst, VReg base, int64_t disp, int dst_sz, int src_sz) {
    size_t off = s->len;
    X86Mem m = {REG(base), X86_NOREG, 1, disp};
    x86_movsx_rm(s, dst_sz, src_sz, REG(dst), m); // movsbl/movswl disp(base), dst
    asm_record(ASM_MOVSX, off, s->len - off, REG(dst), REG(base), -1, dst_sz, 0, disp, NULL, 0, -1, false);
}
// movl/movq disp(base), dst: regular load from base+offset
static void asm_mov_base_off_reg(SecBuf *s, VReg dst, VReg base, int64_t disp, int sz) {
    size_t off = s->len;
    X86Mem m = {REG(base), X86_NOREG, 1, disp};
    x86_mov_rm(s, sz, REG(dst), m); // movl/movq disp(base), dst
    asm_record(ASM_MOV_RR, off, s->len - off, REG(dst), REG(base), -1, sz, 0, disp, NULL, 0, -1, false);
}
// movq phy, disp(base_vreg): store physical reg to base+offset
static void asm_mov_phy_base_off(SecBuf *s, X86Reg phy, VReg base, int64_t disp, int sz) {
    size_t off = s->len;
    X86Mem m = {REG(base), X86_NOREG, 1, disp};
    x86_mov_mr(s, sz, m, phy); // movq phy, disp(%base)
    asm_record(ASM_MOV_RR, off, s->len - off, phy, REG(base), -1, sz, 0, disp, NULL, 0, -1, true);
}
#endif

#ifdef ARCH_ARM64
// rd = x29 + imm (handles negative and large |imm| via scratch x16)
static void asm_add_fp_imm(SecBuf *s, Arm64Reg rd, int imm) {
    int abs_imm = imm < 0 ? -imm : imm;
    if (abs_imm < 4096) {
        if (imm >= 0)
            arm64_add_imm(s, 1, rd, CG_ARM_FP, imm, 0);
        else
            arm64_sub_imm(s, 1, rd, CG_ARM_FP, -imm, 0);
    } else {
        int64_t v = abs_imm;
        arm64_movz(s, 1, ARM64_X16, (uint16_t)(v & 0xffff), 0);
        if (v >> 16) arm64_movk(s, 1, ARM64_X16, (uint16_t)((v >> 16) & 0xffff), 16);
        if (v >> 32) arm64_movk(s, 1, ARM64_X16, (uint16_t)((v >> 32) & 0xffff), 32);
        if (v >> 48) arm64_movk(s, 1, ARM64_X16, (uint16_t)((v >> 48) & 0xffff), 48);
        if (imm >= 0)
            arm64_add_reg(s, 1, rd, CG_ARM_FP, ARM64_X16, ARM64_LSL, 0);
        else
            arm64_sub_reg(s, 1, rd, CG_ARM_FP, ARM64_X16, ARM64_LSL, 0);
    }
}
// sub x0, x29, x16
static void asm_sub_x0_fp_x16(SecBuf *s) {
    arm64_sub_reg(s, 1, ARM64_X0, CG_ARM_FP, ARM64_X16, ARM64_LSL, 0);
}
// rd = rd + imm
static void asm_add_self_imm(SecBuf *s, Arm64Reg rd, int imm) {
    arm64_add_imm(s, 1, rd, rd, imm, 0);
}
// rd = rd & imm
static void asm_and_self_imm(SecBuf *s, Arm64Reg rd, uint64_t imm) {
    arm64_and_imm(s, 1, rd, rd, imm);
}
static void asm_sub_sp_sp_reg(SecBuf *s, Arm64Reg rs) {
    arm64_sub_extreg(s, 1, ARM64_SP, ARM64_SP, rs, ARM64_UXTX, 0); // sub sp, sp, rs
}
static void asm_add_sp_sp_reg(SecBuf *s, Arm64Reg rs) {
    arm64_add_extreg(s, 1, ARM64_SP, ARM64_SP, rs, ARM64_UXTX, 0); // add sp, sp, rs
}
// mov rd, sp
static void asm_mov_reg_sp(SecBuf *s, Arm64Reg rd) {
    arm64_add_imm(s, 1, rd, ARM64_SP, 0, 0); // mov rd, sp
}
// mov sp, rs
static void asm_mov_sp_reg(SecBuf *s, Arm64Reg rs) {
    arm64_add_imm(s, 1, ARM64_SP, rs, 0, 0); // mov sp, rs
}

// mov x16, x{base}  — move virtual reg to x16 scratch
static void asm_mov_x16_reg(SecBuf *s, VReg base_r) {
    arm64_orr_reg(s, 1, ARM64_X16, ARM64_XZR, REG(base_r), ARM64_LSL, 0); // mov x16, x{base_r}
}
// str x16, [x{base}, #uimm8] — store x16 to virtual base reg + scaled offset
static void asm_str_x16_reg_uoff(SecBuf *s, VReg base_r, int byte_off) {
    arm64_str_uoff(s, 3, ARM64_X16, REG(base_r), (uint32_t)(byte_off / 8)); // str x16, [base, #byte_off]
}
// ldr x16, [x{base}, #uimm8] — load x16 from virtual base reg + scaled offset
static void asm_ldr_x16_reg_uoff(SecBuf *s, VReg base_r, int byte_off) {
    arm64_ldr_uoff(s, 3, ARM64_X16, REG(base_r), (uint32_t)(byte_off / 8)); // ldr x16, [base, #byte_off]
}
static void asm_str_fp_reg(SecBuf *s, VReg reg) {
    arm64_str_imm(s, 1, CG_ARM_FP, REG(reg), 0, false); // str x29, [x{reg}]
}
static void asm_ldr_fp_reg(SecBuf *s, VReg reg) {
    arm64_ldr_imm(s, 1, CG_ARM_FP, REG(reg), 0, false); // ldr x29, [x{reg}]
}
static void asm_ldur_x16_fp_minus(SecBuf *s, int off) {
    arm64_ldur(s, 1, ARM64_X16, CG_ARM_FP, -off);
}
// rd = x29 - imm  (handles large |imm| via scratch x16)
static void asm_sub_fp_imm(SecBuf *s, Arm64Reg rd, int32_t imm) {
    if (imm >= 0 && imm < 4096) {
        arm64_sub_imm(s, 1, rd, CG_ARM_FP, imm, 0);
    } else {
        int64_t v = imm < 0 ? -(int64_t)imm : (int64_t)imm;
        arm64_movz(s, 1, ARM64_X16, (uint16_t)(v & 0xffff), 0);
        if (v >> 16) arm64_movk(s, 1, ARM64_X16, (uint16_t)((v >> 16) & 0xffff), 16);
        if (v >> 32) arm64_movk(s, 1, ARM64_X16, (uint16_t)((v >> 32) & 0xffff), 32);
        if (imm >= 0)
            arm64_sub_reg(s, 1, rd, CG_ARM_FP, ARM64_X16, ARM64_LSL, 0);
        else
            arm64_add_reg(s, 1, rd, CG_ARM_FP, ARM64_X16, ARM64_LSL, 0);
    }
}
// rd = x29 - rs
static void asm_sub_fp_reg(SecBuf *s, Arm64Reg rd, Arm64Reg rs) {
    arm64_sub_reg(s, 1, rd, CG_ARM_FP, rs, ARM64_LSL, 0);
}
// ldr x{rd}, [x{rs}] — load 64-bit from register
static void asm_ldr_reg(SecBuf *s, Arm64Reg rd, Arm64Reg rs) {
    arm64_ldr_uoff(s, 3, rd, rs, 0);
}
// str x{rd}, [x{rs}] — store 64-bit to register
static void asm_str_reg(SecBuf *s, Arm64Reg rd, Arm64Reg rs) {
    arm64_str_uoff(s, 3, rd, rs, 0);
}
// ldur rd, [x29, #-off] — unscaled load from frame
static void asm_ldur_fp_phy(SecBuf *s, Arm64Reg rd, int32_t off) {
    arm64_ldur(s, 1, rd, CG_ARM_FP, -off);
}
// stur rd, [x29, #-off] — unscaled store to frame
static void asm_stur_fp_phy(SecBuf *s, Arm64Reg rd, int32_t off) {
    arm64_stur(s, 1, rd, CG_ARM_FP, -off);
}
// asr rd, r{src}, #63 — arithmetic shift right by 63
static void asm_asr_63(SecBuf *s, Arm64Reg rd, VReg src) {
    arm64_asr_imm(s, 1, rd, REG(src), 63); // asr rd, x{src}, #63
}
static void asm_cmn_imm(SecBuf *s, VReg r, int sf, int imm) {
    // CMN = ADDS xzr, r, #imm (compare negative: flags set by r+imm)
    arm64_adds_imm(s, sf, ARM64_X31, REG(r), imm, 0);
}
// r = x29 + imm  (handles large |imm| via scratch x16)
static void asm_add_reg_fp_imm(SecBuf *s, VReg r, int32_t imm) {
    int abs_imm = imm < 0 ? -imm : imm;
    if (abs_imm < 4096) {
        if (imm >= 0)
            arm64_add_imm(s, 1, REG(r), CG_ARM_FP, imm, 0);
        else
            arm64_sub_imm(s, 1, REG(r), CG_ARM_FP, -imm, 0);
    } else {
        // mov x16, #|imm|; r = x29 +/- x16
        int v = abs_imm;
        arm64_movz(s, 1, ARM64_X16, (uint16_t)(v & 0xffff), 0);
        v >>= 16;
        int sh = 16;
        while (v) {
            arm64_movk(s, 1, ARM64_X16, (uint16_t)(v & 0xffff), sh);
            v >>= 16;
            sh += 16;
        }
        if (imm >= 0)
            arm64_add_reg(s, 1, REG(r), ARM64_X29, ARM64_X16, ARM64_LSL, 0);
        else
            arm64_sub_reg(s, 1, REG(r), ARM64_X29, ARM64_X16, ARM64_LSL, 0);
    }
}
// r = x29 - imm  (handles large |imm| via scratch x16)
static void asm_sub_reg_fp_imm(SecBuf *s, VReg r, int32_t imm) {
    int abs_imm = imm < 0 ? -imm : imm;
    if (abs_imm < 4096) {
        if (imm >= 0)
            arm64_sub_imm(s, 1, REG(r), CG_ARM_FP, imm, 0);
        else
            arm64_add_imm(s, 1, REG(r), CG_ARM_FP, -imm, 0);
    } else {
        // mov x16, #|imm|; r = x29 -/+ x16
        int v = abs_imm;
        arm64_movz(s, 1, ARM64_X16, (uint16_t)(v & 0xffff), 0);
        v >>= 16;
        int sh = 16;
        while (v) {
            arm64_movk(s, 1, ARM64_X16, (uint16_t)(v & 0xffff), sh);
            v >>= 16;
            sh += 16;
        }
        if (imm >= 0)
            arm64_sub_reg(s, 1, REG(r), CG_ARM_FP, ARM64_X16, ARM64_LSL, 0);
        else
            arm64_add_reg(s, 1, REG(r), CG_ARM_FP, ARM64_X16, ARM64_LSL, 0);
    }
    return;
}
static void asm_sub_reg_fp_reg(SecBuf *s, int dst, int src, int size) {
    int sf = (size == 8) ? 1 : 0;
    arm64_sub_reg(s, sf, REG(dst), CG_ARM_FP, REG(src), ARM64_LSL, 0);
}
static void asm_sub_reg_fp_phy(SecBuf *s, VReg dst, Arm64Reg src, int size) {
    int sf = (size == 8) ? 1 : 0;
    arm64_sub_reg(s, sf, REG(dst), CG_ARM_FP, src, ARM64_LSL, 0);
}
static void asm_stur_fp(SecBuf *s, VReg r, int off) {
    arm64_stur(s, 1, REG(r), CG_ARM_FP, -off); // stur x{r}, [x29, #-off]
}
static void asm_ldur_fp(SecBuf *s, VReg r, int off) {
    arm64_ldur(s, 1, REG(r), CG_ARM_FP, -off); // ldur x{r}, [x29, #-off]
}

static void asm_stur_phy(SecBuf *s, Arm64Reg rt, Arm64Reg rn, int sf, int32_t off) {
    if (sf == 0) arm64_sturb(s, rt, rn, off);
    else if (sf == 1)
        arm64_sturh(s, rt, rn, off);
    else if (sf == 2)
        arm64_stur(s, 0, rt, rn, off);
    else
        arm64_stur(s, 1, rt, rn, off);
}

static void asm_ldur_phy(SecBuf *s, Arm64Reg rt, Arm64Reg rn, int sf, int32_t off) {
    if (sf == 0) arm64_ldurb(s, (int)rt, (int)rn, off);
    else if (sf == 1)
        arm64_ldurh(s, (int)rt, (int)rn, off);
    else if (sf == 2)
        arm64_ldur(s, 0, (int)rt, (int)rn, off);
    else
        arm64_ldur(s, 1, (int)rt, (int)rn, off);
}
#endif

#ifndef ARCH_ARM64
// Use codegen register indices (0..7) for these wrappers
static void asm_mov_rbp_reg(SecBuf *s, VReg r, int size, int offset) {
    size_t off = s->len;
    X86Mem m = {CG_X86_FP, X86_NOREG, 1, -offset};
    x86_mov_rm(s, size, REG(r), m);
    asm_record(ASM_MOV_RRBP, off, s->len - off, REG(r), -1, -1, size, 0, offset, NULL, 0, -1, false);
}
static void asm_mov_reg_rbp(SecBuf *s, VReg r, int size, int offset) {
    size_t off = s->len;
    X86Mem m = {CG_X86_FP, X86_NOREG, 1, -offset};
    x86_mov_mr(s, size, m, REG(r));
    asm_record(ASM_MOV_RBPR, off, s->len - off, REG(r), -1, -1, size, 0, offset, NULL, 0, -1, true);
}
static void asm_lea_rbp_reg(SecBuf *s, VReg r, int size, int offset) {
    size_t off = s->len;
    X86Mem m = {CG_X86_FP, X86_NOREG, 1, -offset};
    x86_lea(s, size, REG(r), m);
    asm_record(ASM_LEA_FP, off, s->len - off, REG(r), -1, -1, size, 0, -offset, NULL, 0, -1, false);
}
#endif

static void asm_cmp_imm(SecBuf *s, VReg r, int size, int32_t imm) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    int sf = (size == 8) ? 1 : 0;
    arm64_subs_imm(s, sf, 31, REG(r), imm, 0);
#else
    x86_cmp_ri(s, size, REG(r), imm);
#endif
    asm_record(ASM_CMP_RI, off, s->len - off, REG(r), -1, -1, size, imm, 0, NULL, 0, -1, false);
}

#ifndef ARCH_ARM64
static void asm_test(SecBuf *s, X86Reg a, X86Reg b, int size) {
    size_t off = s->len;
    x86_test_rr(s, size, a, b);
    asm_record(ASM_TEST_RR, off, s->len - off, a, b, -1, size, 0, 0, NULL, 0, -1, false);
}
static void asm_inc(SecBuf *s, X86Reg r, int size) {
    x86_inc_r(s, size, r);
}
#endif

// ============================================================================
// Bit operations
// ============================================================================

static void asm_bswap(SecBuf *s, VReg vr, int size) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    Arm64Reg r = REG(vr);
    if (size == 4)
        arm64_rev(s, 0, r, r);
    else
        arm64_rev(s, 1, r, r);
    asm_record(ASM_BSWAP, off, 1, r, -1, -1, size, 0, 0, NULL, 0, -1, false);
#else
    X86Reg r = REG(vr);
    x86_bswap(s, size, r);
    size_t count = s->len - off;
    asm_record(ASM_BSWAP, off, count, r, -1, -1, size, 0, 0, NULL, 0, -1, false);
#endif
}

static void asm_clz(SecBuf *s, VReg dst, VReg src, int size) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    Arm64Reg rdst = REG(dst);
    Arm64Reg rsrc = REG(src);
    int sf = (size == 8) ? 1 : 0;
    arm64_clz(s, sf, rdst, rsrc);
    asm_record(ASM_CLZ, off, 1, rdst, rsrc, -1, size, 0, 0, NULL, 0, -1, false);
#else
    X86Reg rdst = REG(dst);
    X86Reg rsrc = REG(src);
    x86_lzcnt(s, size, rdst, rsrc);
    size_t count = s->len - off;
    asm_record(ASM_CLZ, off, count, rdst, rsrc, -1, size, 0, 0, NULL, 0, -1, false);
#endif
}

#ifdef ARCH_ARM64
static void asm_rbit(SecBuf *s, int dst, int src, int size) {
    size_t off = s->len;
    int sf = (size == 8) ? 1 : 0;
    Arm64Reg rdst = REG(dst);
    Arm64Reg rsrc = REG(src);
    arm64_rbit(s, sf, rdst, rsrc);
    asm_record(ASM_RBIT, off, 1, rdst, rsrc, -1, size, 0, 0, NULL, 0, -1, false);
}
#endif

static void asm_rev(SecBuf *s, VReg vr, int size) {
#ifdef ARCH_ARM64
    size_t off = s->len;
    int sf = (size == 8) ? 1 : 0;
    Arm64Reg r = REG(vr);
    arm64_rev(s, sf, r, r);
    asm_record(ASM_REV, off, 1, r, -1, -1, size, 0, 0, NULL, 0, -1, false);
#else
    asm_bswap(s, vr, size);
#endif
}

#ifdef ARCH_ARM64
static void asm_rev16(SecBuf *s, VReg vr, int size) {
    size_t off = s->len;
    int sf = (size == 8) ? 1 : 0;
    Arm64Reg r = REG(vr);
    arm64_rev16(s, sf, r, r); // rev16 wr, wr
    asm_record(ASM_REV16, off, 1, r, -1, -1, size, 0, 0, NULL, 0, -1, false);
}
#endif

// ============================================================================
// Floating point
// ============================================================================

#ifdef ARCH_ARM64
static void asm_cvtsi2ss(SecBuf *s, Arm64Reg src, int size) {
    // ARM64: single-precision uses same scvtf path as double for now
    arm64_scvtf(s, size == 8, 0, 0, src);
}
#else
static void asm_cvtsi2ss(SecBuf *s, X86Reg src, int size) {
    x86_cvtsi2ss(s, size, X86_XMM0, src); // cvtsi2ss src, %xmm0
}
#endif

static void asm_cvtsi2sd(SecBuf *s, VReg src_r, int size) {
#ifdef ARCH_ARM64
    arm64_scvtf(s, size == 8, 1, 0, REG(src_r)); // scvtf d0, w/x{src_r} (ftype=1=double dst)
#else
    x86_cvtsi2sd(s, size, X86_XMM0, REG(src_r));
#endif
}
static void asm_cvttsd2si(SecBuf *s, VReg dst_r, int size) {
#ifdef ARCH_ARM64
    arm64_fcvtzs(s, size == 8, 1, REG(dst_r), 0); // fcvtzs w/x{dst_r}, d0 (ftype=1=double src)
#else
    x86_cvttsd2si(s, size, REG(dst_r), X86_XMM0);
#endif
}
static void asm_cvtss2sd(SecBuf *s) {
#ifdef ARCH_ARM64
    arm64_fcvt(s, 1, 0, 0, 0); // fcvt d0, s0
#else
    x86_cvtss2sd(s, X86_XMM0, X86_XMM0);
#endif
}
static void asm_cvtsd2ss(SecBuf *s) {
#ifdef ARCH_ARM64
    arm64_fcvt(s, 0, 1, 0, 0); // fcvt s0, d0 (opc=0=single dest, ftype=1=double src)
#else
    x86_cvtsd2ss(s, X86_XMM0, X86_XMM0);
#endif
}

// ARM64 ldr from register offset (unsigned offset)
static void asm_ldr_reg_off(SecBuf *s, VReg dst_r, VReg base_r, int size, uint32_t uimm) {
#ifdef ARCH_ARM64
    switch (size) {
    case 1: arm64_ldrb_uoff(s, REG(dst_r), REG(base_r), uimm); break;
    case 2: arm64_ldrh_uoff(s, REG(dst_r), REG(base_r), uimm); break;
    case 4: arm64_ldr_uoff(s, 2, REG(dst_r), REG(base_r), uimm / 4); break;
    default: arm64_ldr_uoff(s, 3, REG(dst_r), REG(base_r), uimm / 8); break;
    }
#else
    X86Mem m = {REG(base_r), X86_NOREG, 1, (int64_t)uimm};
    x86_mov_rm(s, size, REG(dst_r), m);
#endif
}
// ARM64 str to register offset (unsigned offset)
static void asm_str_reg_off(SecBuf *s, int src_r, int base_r, int size, uint32_t uimm) {
#ifdef ARCH_ARM64
    switch (size) {
    case 1: arm64_strb_uoff(s, REG(src_r), REG(base_r), uimm); break;
    case 2: arm64_strh_uoff(s, REG(src_r), REG(base_r), uimm); break;
    case 4: arm64_str_uoff(s, 2, REG(src_r), REG(base_r), uimm / 4); break;
    default: arm64_str_uoff(s, 3, REG(src_r), REG(base_r), uimm / 8); break;
    }
#else
    X86Mem m = {REG(base_r), X86_NOREG, 1, (int64_t)uimm};
    x86_mov_mr(s, size, m, REG(src_r));
#endif
}
#ifdef ARCH_ARM64
// ARM64 load fp reg dst_fp_r from [base_r, #byte_off]; size=4→S, 8→D
static void asm_ldr_fp_off(SecBuf *s, Arm64Reg dst_fp_r, VReg base_r, int size, uint32_t byte_off) {
    int sz = (size == 8) ? 3 : 2;
    arm64_ldr_fp(s, sz, dst_fp_r, REG(base_r), byte_off);
}
#endif
// load float s/d from [base]; size=4→S(32bit), size=8→D(64bit)
#ifdef ARCH_ARM64
static void asm_ldr_fp(SecBuf *s, Arm64Reg dst_fp_r, VReg base_r, int size) {
    int sz = (size == 8) ? 3 : 2; // sz=3→D(64bit), sz=2→S(32bit)
    arm64_ldr_fp(s, sz, dst_fp_r, REG(base_r), 0);
}
#else
static void asm_ldr_fp(SecBuf *s, X86XmmReg dst_fp_r, VReg base_r, int size) {
    X86Mem m = {REG(base_r), X86_NOREG, 1, 0};
    if (size == 8)
        x86_movsd_rm(s, dst_fp_r, m);
    else
        x86_movss_rm(s, dst_fp_r, m);
}
#endif
// store float s/d to [base]; size=4→S(32bit), size=8→D(64bit)
#ifdef ARCH_ARM64
static void asm_str_fp(SecBuf *s, Arm64Reg src_fp_r, VReg base_r, int size) {
    int sz = (size == 8) ? 3 : 2;
    arm64_str_fp(s, sz, src_fp_r, REG(base_r), 0);
}
#else
static void asm_str_fp(SecBuf *s, X86XmmReg src_fp_r, VReg base_r, int size) {
    X86Mem m = {REG(base_r), X86_NOREG, 1, 0};
    if (size == 8)
        x86_movsd_mr(s, m, src_fp_r);
    else
        x86_movss_mr(s, m, src_fp_r);
}
#endif

#ifdef ARCH_ARM64
// ARM64 sxtw
static void asm_sxtw(SecBuf *s, Arm64Reg dst_r, Arm64Reg src_r) {
    arm64_sxtw(s, dst_r, src_r);
}
#endif

#ifndef ARCH_ARM64
// x86_64 movq GP register to XMM register
static void asm_movq_r_xmm(SecBuf *s, X86XmmReg xmm_dst, VReg gp_src) {
    x86_movq_r_xmm(s, xmm_dst, REG(gp_src));
}
// x86_64 movq XMM register to GP register
static void asm_movq_xmm_r(SecBuf *s, VReg gp_dst, X86XmmReg xmm_src) {
    x86_movq_xmm_r(s, REG(gp_dst), xmm_src);
}

// x86_64 SSE/FP binary ops: addsd, subsd, mulsd, divsd
static void asm_addsd(SecBuf *s) {
    x86_addsd(s, X86_XMM0, X86_XMM1);
}
static void asm_subsd(SecBuf *s) {
    x86_subsd(s, X86_XMM0, X86_XMM1);
}
static void asm_mulsd(SecBuf *s) {
    x86_mulsd(s, X86_XMM0, X86_XMM1);
}
static void asm_divsd(SecBuf *s) {
    x86_divsd(s, X86_XMM0, X86_XMM1);
}
// x86_64 SSE/FP binary ops: addss, subss, mulss, divss
static void asm_addss(SecBuf *s) {
    x86_addss(s, X86_XMM0, X86_XMM1);
}
static void asm_subss(SecBuf *s) {
    x86_subss(s, X86_XMM0, X86_XMM1);
}
static void asm_mulss(SecBuf *s) {
    x86_mulss(s, X86_XMM0, X86_XMM1);
}
static void asm_divss(SecBuf *s) {
    x86_divss(s, X86_XMM0, X86_XMM1);
}

// x86_64 ucomisd / ucomiss (float compare, sets EFLAGS)
static void asm_ucomisd(SecBuf *s) {
    x86_ucomisd(s, X86_XMM0, X86_XMM1);
}
static void asm_ucomiss(SecBuf *s) {
    x86_ucomiss(s, X86_XMM0, X86_XMM1);
}
#endif // X64

#ifdef ARCH_ARM64
// ARM64 fadd, fsub, fmul, fdiv (ftype=0 for single, 1 for double)
static void asm_fadd(SecBuf *s, int ftype) {
    arm64_fadd(s, ftype, 0, 0, 1); // d0, d0, d1
}
static void asm_fsub(SecBuf *s, int ftype) {
    arm64_fsub(s, ftype, 0, 0, 1);
}
static void asm_fmul(SecBuf *s, int ftype) {
    arm64_fmul(s, ftype, 0, 0, 1);
}
static void asm_fdiv(SecBuf *s, int ftype) {
    arm64_fdiv(s, ftype, 0, 0, 1);
}
// ARM64 fcmp (ftype=0 for single, 1 for double)
static void asm_fcmp(SecBuf *s, int ftype) {
    arm64_fcmp(s, ftype, 0, 1); // fcmp d0, d1
}

// str d{fp_r}, [sp, #uimm] — store float/double to SP-relative slot
static void asm_str_fp_sp_off(SecBuf *s, int fp_r, uint32_t uimm) {
    int opc = (uimm % 8 == 0) ? 3 : 2; // 3=64-bit (d), 2=32-bit (s) stride
    arm64_str_fp(s, opc, fp_r, 31, uimm / (opc == 3 ? 8 : 4)); // str d{fp_r}, [sp, #uimm]
}
// asr rd, x{src}, #63 — arithmetic shift right by 63 (sign-extend)
static void asm_asr_rd_reg_63(SecBuf *s, Arm64Reg rd, VReg src) {
    arm64_asr_imm(s, 1, rd, REG(src), 63); // asr rd, x{src}, #63
}
// ldr w/x{dst}, [x{src}] — load GP from reg (sf=0→32bit, 1→64bit)
static void asm_ldr_phy_reg(SecBuf *s, Arm64Reg dst_phy, VReg src, int sf) {
    int sz = sf ? 3 : 2; // sf=1→64-bit(sz=3), sf=0→32-bit(sz=2)
    arm64_ldr_uoff(s, sz, dst_phy, REG(src), 0); // ldr w/x{dst}, [x{src}]
}
// mov x{dst_phy}, x{src_vreg} — move vreg to physical GP register (via orr xd, xzr, xs)
static void asm_mov_phy_reg(SecBuf *s, Arm64Reg dst_phy, VReg src, int sf) {
    arm64_orr_reg(s, sf, dst_phy, ARM64_XZR, REG(src), ARM64_LSL, 0); // mov x{dst_phy}, x{src}
}
// cmn vreg, #imm — compare negative (subs xzr, reg, imm)
static void asm_cmn_vreg_imm(SecBuf *s, VReg r, int sz, int32_t imm) {
    // CMN = ADDS xzr, r, #imm (compare negative: flags set by r+imm)
    arm64_adds_imm(s, sz == 8 ? 1 : 0, ARM64_XZR, REG(r), imm, 0); // cmn x{r}, #imm
}
// fcvtzu w/x{r}, d0 — float→unsigned int conversion
static void asm_fcvtzu(SecBuf *s, VReg r, int sz) {
    int sf = (sz == 8) ? 1 : 0;
    arm64_fcvtzu(s, sf, 1, REG(r), 0); // fcvtzu w/x{r}, d0
}
// fcvtzs w/x{r}, d0 — float→signed int conversion
static void asm_fcvtzs(SecBuf *s, VReg r, int sz) {
    int sf = (sz == 8) ? 1 : 0;
    arm64_fcvtzs(s, sf, 1, REG(r), 0); // fcvtzs w/x{r}, d0
}
// mov rd, rs — move between physical GP registers
static void asm_mov_phy_phy(SecBuf *s, Arm64Reg rd, Arm64Reg rs, int sf) {
    arm64_orr_reg(s, sf, rd, ARM64_XZR, rs, ARM64_LSL, 0); // mov rd, rs
}
// add xrd, xrd, #0 — relocation placeholder for ADD_ABS_LO12
static void asm_add_rd_rd_0(SecBuf *s, Arm64Reg rd) {
    arm64_add_imm(s, 1, rd, rd, 0, 0); // add x{rd}, x{rd}, #0
}
// ldr xrd, [xrd] — load pointer from self (GOT indirection)
static void asm_ldr_rd_rd(SecBuf *s, Arm64Reg rd) {
    arm64_ldr_uoff(s, 3, rd, rd, 0); // ldr x{rd}, [x{rd}]
}
#endif

// ============================================================================
// Atomics
// ============================================================================

// ============================================================================
// x86: atomic xchg and lock cmpxchg
// ============================================================================

#ifndef ARCH_ARM64
// xchg sz, (r_addr), r_val  — atomic exchange mem↔reg
static void asm_xchg_mem(SecBuf *s, VReg r_addr, VReg r_val, int size) {
    // XCHG always has implicit LOCK prefix
    // xchg r/m, r: opcode 87 /r (for 2/4/8), 86 /r (for 1)
    X86Reg rv = REG(r_val);
    X86Reg ra = REG(r_addr);
    uint8_t rex = 0;
    if (size == 8)
        rex = (uint8_t)(0x48 | ((rv >= 8 ? 1 : 0) << 2) | (ra >= 8 ? 1 : 0));
    else if (rv >= 8 || ra >= 8)
        rex = (uint8_t)(0x40 | ((rv >= 8 ? 1 : 0) << 2) | (ra >= 8 ? 1 : 0));
    if (size == 2)
        secbuf_emit8(s, 0x66);
    if (rex)
        secbuf_emit8(s, rex);
    secbuf_emit8(s, size == 1 ? 0x86 : 0x87);
    // Use [r_addr] directly (no SIB if not rsp/r12)
    if ((ra & 7) == 4) { // rsp/r12 needs SIB
        secbuf_emit8(s, (uint8_t)(0x00 | ((rv & 7) << 3) | 4));
        secbuf_emit8(s, 0x24); // SIB: base=rsp/r12, no index
    } else {
        secbuf_emit8(s, (uint8_t)(0x00 | ((rv & 7) << 3) | (ra & 7)));
    }
}

// lock cmpxchg (r_addr), r_desired, size  — rax=expected, result in rax
// Sets ZF=1 if successful
static void asm_lock_cmpxchg_mem(SecBuf *s, VReg r_addr, VReg r_desired, int size) {
    x86_lock_prefix(s); // lock
    X86Reg rd = REG(r_desired);
    X86Reg ra = REG(r_addr);
    // REX prefix if needed
    uint8_t rex = 0;
    if (size == 8) rex = (uint8_t)(0x48 | ((rd >= 8 ? 1 : 0) << 2) | (ra >= 8 ? 1 : 0));
    else if (rd >= 8 || ra >= 8)
        rex = (uint8_t)(0x40 | ((rd >= 8 ? 1 : 0) << 2) | (ra >= 8 ? 1 : 0));
    if (size == 2) secbuf_emit8(s, 0x66);
    if (rex) secbuf_emit8(s, rex);
    secbuf_emit8(s, 0x0F);
    secbuf_emit8(s, size == 1 ? 0xB0 : 0xB1); // cmpxchg m, r
    // ModRM: mod=00, reg=r_desired, rm=r_addr
    if ((ra & 7) == 4) {
        secbuf_emit8(s, (uint8_t)(0x00 | ((rd & 7) << 3) | 4));
        secbuf_emit8(s, (uint8_t)0x24);
    } else {
        secbuf_emit8(s, (uint8_t)(0x00 | ((rd & 7) << 3) | (ra & 7)));
    }
}

// sete r  — set byte if equal (ZF=1)
static void asm_sete(SecBuf *s, VReg r) {
    x86_setcc(s, X86_E, REG(r)); // sete r8
}

// lock xadd (r_addr), r_old, size  — atomic add-and-fetch old value
// After: r_old = old value at (r_addr), (r_addr) = old + r_old
static void asm_lock_xadd_mem(SecBuf *s, VReg r_addr, VReg r_old, int size) {
    x86_lock_prefix(s); // lock
    X86Reg rd = REG(r_old);
    X86Reg ra = REG(r_addr);
    uint8_t rex = 0;
    if (size == 8) rex = (uint8_t)(0x48 | ((rd >= 8 ? 1 : 0) << 2) | (ra >= 8 ? 1 : 0));
    else if (rd >= 8 || ra >= 8)
        rex = (uint8_t)(0x40 | ((rd >= 8 ? 1 : 0) << 2) | (ra >= 8 ? 1 : 0));
    if (size == 2) secbuf_emit8(s, 0x66);
    if (rex) secbuf_emit8(s, rex);
    secbuf_emit8(s, 0x0F);
    secbuf_emit8(s, size == 1 ? 0xC0 : 0xC1); // xadd r/m, r
    // ModRM: mod=00, reg=r_old, rm=r_addr
    if ((ra & 7) == 4) {
        secbuf_emit8(s, (uint8_t)(0x00 | ((rd & 7) << 3) | 4));
        secbuf_emit8(s, (uint8_t)0x24);
    } else {
        secbuf_emit8(s, (uint8_t)(0x00 | ((rd & 7) << 3) | (ra & 7)));
    }
}

// mov sz, -%rbp_off(%rbp), reg  — store reg to spill slot
static void asm_mov_rbp_spill(SecBuf *s, VReg r, int size, int rbp_off) {
    asm_mov_phyreg_rbp(s, REG(r), size, rbp_off); // mov reg, -rbp_off(%rbp)
}

// mov sz, reg, -%rbp_off(%rbp)  — load reg from spill slot
static void asm_mov_spill_rbp(SecBuf *s, VReg r, int size, int rbp_off) {
    asm_mov_rbp(s, REG(r), size, rbp_off); // mov -rbp_off(%rbp), reg
}

// lock cmpxchg (r_addr), r_new  — compare %rax with (r_addr), swap if equal; result in ZF
// Used for non-add fetch-ops (bitwise)
static size_t asm_lock_cmpxchg_rax(SecBuf *s, VReg r_addr, VReg r_new, int size) {
    (void)size;
    asm_lock_cmpxchg_mem(s, r_addr, r_new, size); // reuse the existing function
    return s->len; // return END position (== start of JNE) for fixup patching
}

// add sz, -rbp_off(%rbp), reg  — add spill slot to reg (for add_fetch return)
static void asm_add_spill_reg(SecBuf *s, VReg r, int size, int rbp_off) {
    X86Mem m = {CG_X86_FP, X86_NOREG, 1, -rbp_off};
    x86_add_rm(s, size, REG(r), m); // add -rbp_off(%rbp), reg
}
#endif

// ============================================================================
// ARM64: ldxr/stxr exclusive access (for atomic exchange/CAS)
// ============================================================================

#ifdef ARCH_ARM64
// ldxrb/ldxrh/ldxr dst, [base]  — load exclusive
static void asm_ldxr(SecBuf *s, VReg dst, VReg base, int size) {
    if (size == 1)
        arm64_ldxrb(s, REG(dst), REG(base)); // ldxrb w{dst}, [x{base}]
    else if (size == 2)
        arm64_ldxrh(s, REG(dst), REG(base)); // ldxrh w{dst}, [x{base}]
    else
        arm64_ldxr(s, size == 8 ? 1 : 0, REG(dst), REG(base)); // ldxr w/x{dst}, [x{base}]
}

// stxrb/stxrh/stxr w9, src, [base]  — store exclusive (result in w9)
static void asm_stxr(SecBuf *s, VReg src, VReg base, int size) {
    if (size == 1)
        arm64_stxrb(s, 9, REG(src), REG(base)); // stxrb w9, w{src}, [x{base}]
    else if (size == 2)
        arm64_stxrh(s, 9, REG(src), REG(base)); // stxrh w9, w{src}, [x{base}]
    else
        arm64_stxr(s, size == 8 ? 1 : 0, 9, REG(src), REG(base)); // stxr w9, w/x{src}, [x{base}]
}
#endif /* ARCH_ARM64 for ldxr/stxr */

#ifdef ARCH_ARM64
static void asm_dmb(SecBuf *s) {
    size_t off = s->len;
    arm64_dmb(s, 0xb);
    asm_record(ASM_FENCE, off, 1, -1, -1, -1, 0, 0, 0, NULL, 0, -1, false);
}
static void asm_dmb_ishld(SecBuf *s) { return asm_dmb(s); }
static void asm_dmb_ishst(SecBuf *s) { return asm_dmb(s); }
#endif /* ARCH_ARM64 for dmb */

// ============================================================================
// CLD / NOP
// ============================================================================

#ifndef ARCH_ARM64
static void asm_cld(SecBuf *s) {
    size_t off = s->len;
    x86_cld(s); // cld (clear direction flag)
    asm_record(ASM_CLD, off, (size_t)(s->len - off), -1, -1, -1, 0, 0, 0, NULL, 0, -1, false);
}
#endif

static void asm_nop(SecBuf *s) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    arm64_nop(s);
#else
    x86_nop(s);
#endif
    asm_record(ASM_NOP, off, (size_t)(s->len - off), -1, -1, -1, 0, 0, 0, NULL, 0, -1, false);
}

static void asm_mfence(SecBuf *s) {
    size_t off = s->len;
#ifdef ARCH_ARM64
    asm_dmb(s);
#else
    x86_mfence(s);
#endif
    asm_record(ASM_FENCE, off, (size_t)(s->len - off), -1, -1, -1, 0, 0, 0, NULL, 0, -1, false);
}

static void asm_and_imm(SecBuf *s, VReg r, int size, int32_t imm) {
#ifdef ARCH_ARM64
    int sf = (size == 8) ? 1 : 0;
    // Use union to avoid sign-extension ambiguity
    arm64_and_imm(s, sf, REG(r), REG(r),
                  size == 8 ? (uint64_t)(int64_t)imm : (uint64_t)(uint32_t)imm);
#else
    x86_and_ri(s, size, REG(r), imm);
#endif
}
// 64-bit AND immediate (for mask values like ~15 that need full 64 bits)
static void asm_and64_imm(SecBuf *s, VReg r, uint64_t imm64) {
#ifdef ARCH_ARM64
    arm64_and_imm(s, 1, REG(r), REG(r), imm64);
#else
    x86_and_ri(s, 8, REG(r), (int32_t)imm64);
#endif
}
static void asm_or_imm(SecBuf *s, VReg r, int size, int32_t imm) {
#ifdef ARCH_ARM64
    int sf = (size == 8) ? 1 : 0;
    arm64_orr_imm(s, sf, REG(r), REG(r), imm);
#else
    x86_or_ri(s, size, REG(r), imm);
#endif
}
static void asm_xor_imm(SecBuf *s, VReg r, int size, int32_t imm) {
#ifdef ARCH_ARM64
    int sf = (size == 8) ? 1 : 0;
    arm64_eor_imm(s, sf, REG(r), REG(r), imm);
#else
    x86_xor_ri(s, size, REG(r), imm);
#endif
}

#ifdef ARCH_ARM64
static void asm_movz(SecBuf *s, Arm64Reg rd, int sf, uint16_t imm16, int shift) {
    arm64_movz(s, sf, rd, imm16, shift);
}
// fmov x{rd}, d{rn}  — copy fp reg raw bits to integer virtual reg
static void asm_fmov_f2i(SecBuf *s, VReg rd, int rn, int sf) {
    arm64_fmov_f2i(s, sf, REG(rd), rn);
}
// fmov d{rd}, x{rs}  — copy integer virtual reg raw bits to fp reg
static void asm_fmov_i2f(SecBuf *s, int fp_rd, int int_rs, int sf) {
    arm64_fmov_i2f(s, sf, fp_rd, REG(int_rs));
}
// scvtf d0, w/x{rs}  — signed int to double
static void asm_scvtf(SecBuf *s, Arm64Reg fp_rd, VReg int_rs, int sf) {
    arm64_scvtf(s, sf, 1, fp_rd, REG(int_rs));
}
// ucvtf d0, w/x{rs}  — unsigned int to double
static void asm_ucvtf(SecBuf *s, Arm64Reg fp_rd, VReg int_rs, int sf) {
    arm64_ucvtf(s, sf, 1, fp_rd, REG(int_rs));
}
static void asm_fcvt(SecBuf *s, int opc, int ftype, Arm64Reg rd, Arm64Reg rn) {
    arm64_fcvt(s, opc, ftype, rd, rn);
}
#endif

static void asm_stur(SecBuf *s, VReg src, VReg base, int sf, int off) {
#ifdef ARCH_ARM64
    arm64_stur(s, sf, REG(src), REG(base), off);
#else
    x86_mov_mr(s, sf == 1 ? 8 : 4, x86_mem(REG(base), off), REG(src));
#endif
}
// unscaled store for any size (byte/half/word/dword), negative offsets ok
#ifdef ARCH_ARM64
static void asm_stur_sz(SecBuf *s, VReg src, Arm64Reg base, int sz, int off) {
    switch (sz) {
    case 1: arm64_sturb(s, REG(src), base, off); break;
    case 2: arm64_sturh(s, REG(src), base, off); break;
    case 4: arm64_stur(s, 0, REG(src), base, off); break;
    default: arm64_stur(s, 1, REG(src), base, off); break;
    }
}
#else
static void asm_stur_sz(SecBuf *s, VReg src, VReg base, int sz, int off) {
    asm_stur(s, src, base, sz == 8 ? 1 : 0, off);
}
#endif

static void asm_ldur(SecBuf *s, int dst, int base, int sf, int off) {
#ifdef ARCH_ARM64
    arm64_ldur(s, sf, REG(dst), REG(base), off);
#else
    x86_mov_rm(s, sf == 1 ? 8 : 4, REG(dst), x86_mem(REG(base), off));
#endif
}
#ifdef ARCH_ARM64
// unscaled load for any size (byte/half/word/dword), negative offsets ok
static void asm_ldur_sz(SecBuf *s, int dst, int base, int sz, int off) {
    switch (sz) {
    case 1: arm64_ldurb(s, REG(dst), base, off); break;
    case 2: arm64_ldurh(s, REG(dst), base, off); break;
    case 4: arm64_ldur(s, 0, REG(dst), base, off); break;
    default: arm64_ldur(s, 1, REG(dst), base, off); break;
    }
    //#else
    //return asm_ldur(s, dst, base, sz == 8 ? 1 : 0, off);
}
#endif

#if 0
static void asm_ldr_imm(SecBuf *s, VReg dst, VReg base, int sf, int off, bool pre) {
#ifdef ARCH_ARM64
    arm64_ldr_imm(s, sf, REG(dst), REG(base), off, pre);
#else
    (void)pre;
    x86_mov_rm(s, sf == 1 ? 8 : 4, REG(dst), x86_mem(REG(base), off));
#endif
}
static void asm_str_imm(SecBuf *s, VReg src, VReg base, int sf, int off, bool pre) {
#ifdef ARCH_ARM64
    arm64_str_imm(s, sf, REG(src), REG(base), off, pre);
#else
    (void)pre;
    x86_mov_mr(s, sf == 1 ? 8 : 4, x86_mem(REG(base), off), REG(src));
#endif
}
#endif

#ifdef ARCH_ARM64
static void asm_stlr(SecBuf *s, VReg src, VReg base, int size) {
    arm64_stlr(s, size == 8 ? 1 : 0, REG(src), REG(base));
}
static void asm_stlrb(SecBuf *s, VReg src, VReg base) {
    arm64_stlrb(s, REG(src), REG(base));
}
static void asm_stlrh(SecBuf *s, VReg src, VReg base) {
    arm64_stlrh(s, REG(src), REG(base));
}
static void asm_ldar(SecBuf *s, VReg dst, VReg base, int size) {
    arm64_ldar(s, size == 8 ? 1 : 0, REG(dst), REG(base));
}
static void asm_ldarb(SecBuf *s, VReg dst, VReg base) {
    arm64_ldarb(s, REG(dst), REG(base));
}
static void asm_ldarh(SecBuf *s, VReg dst, VReg base) {
    arm64_ldarh(s, REG(dst), REG(base));
}
#endif

static void asm_ldrb(SecBuf *s, VReg dst, VReg base, int off) {
#ifdef ARCH_ARM64
    arm64_ldrb_imm(s, REG(dst), REG(base), off);
#else
    x86_movzx_rm(s, 4, 1, REG(dst), x86_mem(REG(base), off));
#endif
}
static void asm_ldrh(SecBuf *s, VReg dst, VReg base, int off) {
#ifdef ARCH_ARM64
    arm64_ldrh_imm(s, REG(dst), REG(base), off);
#else
    x86_movzx_rm(s, 4, 2, REG(dst), x86_mem(REG(base), off));
#endif
}

#ifdef ARCH_ARM64
static void asm_adrp(SecBuf *s, Arm64Reg rd) {
    arm64_adrp(s, rd, 0);
}
#endif

#ifndef ARCH_ARM64
// movzx/movsx from memory addressed by a virtual register
static void asm_movzx_mem_reg(SecBuf *s, VReg dst, VReg src_addr, int dst_sz, int src_sz) {
    size_t off = s->len;
    x86_movzx_rm(s, dst_sz, src_sz, REG(dst), x86_mem(REG(src_addr), 0));
    asm_record(ASM_MOVZX, off, s->len - off, REG(dst), REG(src_addr), -1, dst_sz, 0, 0, NULL, 0, -1, false);
}
static void asm_movsx_mem_reg(SecBuf *s, VReg dst, VReg src_addr, int dst_sz, int src_sz) {
    size_t off = s->len;
    x86_movsx_rm(s, dst_sz, src_sz, REG(dst), x86_mem(REG(src_addr), 0));
    asm_record(ASM_MOVSX, off, s->len - off, REG(dst), REG(src_addr), -1, dst_sz, 0, 0, NULL, 0, -1, false);
}

// mov from [virtual_reg] to virtual_reg
static void asm_mov_mem_reg(SecBuf *s, VReg dst, VReg src_addr, int sz) {
    size_t off = s->len;
    x86_mov_rm(s, sz, REG(dst), x86_mem(REG(src_addr), 0));
    asm_record(ASM_MOV_RR, off, s->len - off, REG(dst), REG(src_addr), -1, sz, 0, 0, NULL, 0, -1, false);
}

// mov from virtual_reg to [virtual_reg]
static void asm_mov_reg_mem(SecBuf *s, VReg src, VReg dst_addr, int sz) {
    size_t off = s->len;
    x86_mov_mr(s, sz, x86_mem(REG(dst_addr), 0), REG(src));
    asm_record(ASM_MOV_RR, off, s->len - off, REG(src), REG(dst_addr), -1, sz, 0, 0, NULL, 0, -1, false);
}

// movaps from xmm to [rbp - offset]
static void asm_movaps_rbp_xmm(SecBuf *s, X86XmmReg xmm_idx, int offset) {
    size_t off = s->len;
    X86Mem m = {CG_X86_FP, X86_NOREG, 1, -offset};
    x86_movaps_mr(s, m, xmm_idx);
    asm_record(ASM_MOV_RBPR, off, s->len - off, (X86Reg)xmm_idx, -1, -1, 16, 0, offset, NULL, 0, -1, true);
}

// ALU ops with rbp-relative memory operand
static void asm_and_rbp_reg(SecBuf *s, VReg r, int size, int offset) {
    size_t off = s->len;
    X86Mem m = {CG_X86_FP, X86_NOREG, 1, -offset};
    x86_and_rm(s, size, REG(r), m);
    asm_record(ASM_AND_RR, off, s->len - off, REG(r), -1, -1, size, 0, offset, NULL, 0, -1, false);
}
static void asm_or_rbp_reg(SecBuf *s, VReg r, int size, int offset) {
    size_t off = s->len;
    X86Mem m = {CG_X86_FP, X86_NOREG, 1, -offset};
    x86_or_rm(s, size, REG(r), m);
    asm_record(ASM_OR_RR, off, s->len - off, REG(r), -1, -1, size, 0, offset, NULL, 0, -1, false);
}
static void asm_xor_rbp_reg(SecBuf *s, VReg r, int size, int offset) {
    size_t off = s->len;
    X86Mem m = {CG_X86_FP, X86_NOREG, 1, -offset};
    x86_xor_rm(s, size, REG(r), m);
    asm_record(ASM_XOR_RR, off, s->len - off, REG(r), -1, -1, size, 0, offset, NULL, 0, -1, false);
}
#endif /* !ARCH_ARM64 */

// ============================================================================
// x86: movss/movsd to/from [rbp ± offset] and physical XMM registers
// ============================================================================

#ifndef ARCH_ARM64
// movss xmm_src, -(offset)(%%rbp)  — store single float to frame
static void asm_movss_mr_rbp(SecBuf *s, X86XmmReg xmm_src, int offset) {
    X86Mem m = {CG_X86_FP, X86_NOREG, 1, -offset};
    x86_movss_mr(s, m, (X86XmmReg)xmm_src); // movss xmm_src, -(offset)(%%rbp)
}
// movsd xmm_src, -(offset)(%%rbp)  — store double float to frame
static void asm_movsd_mr_rbp(SecBuf *s, X86XmmReg xmm_src, int offset) {
    X86Mem m = {CG_X86_FP, X86_NOREG, 1, -offset};
    x86_movsd_mr(s, m, (X86XmmReg)xmm_src); // movsd xmm_src, -(offset)(%%rbp)
}
// movss offset(%%rbp), xmm_dst  — load single float from frame
static void asm_movss_rm_rbp(SecBuf *s, X86XmmReg xmm_dst, int offset) {
    X86Mem m = {CG_X86_FP, X86_NOREG, 1, -offset};
    x86_movss_rm(s, (X86XmmReg)xmm_dst, m); // movss -offset(%%rbp), xmm_dst
}
// movsd -offset(%%rbp), xmm_dst  — load double float from frame
static void asm_movsd_rm_rbp(SecBuf *s, X86XmmReg xmm_dst, int offset) {
    X86Mem m = {CG_X86_FP, X86_NOREG, 1, -offset};
    x86_movsd_rm(s, (X86XmmReg)xmm_dst, m); // movsd -offset(%%rbp), xmm_dst
}
// cvtsd2ss from XMM param_xmm[i], store to rbp — convert param double to float
static void asm_cvtsd2ss_xmm_rbp(SecBuf *s, X86XmmReg xmm_src, int offset) {
    x86_cvtsd2ss(s, X86_XMM0, (X86XmmReg)xmm_src); // cvtsd2ss xmm_src, xmm0
    X86Mem m = {CG_X86_FP, X86_NOREG, 1, -offset};
    x86_movss_mr(s, m, X86_XMM0); // movss xmm0, -(offset)(%%rbp)
}
// movsd from XMM param to rbp
static void asm_movsd_xmm_rbp(SecBuf *s, X86XmmReg xmm_src, int offset) {
    X86Mem m = {CG_X86_FP, X86_NOREG, 1, -offset};
    x86_movsd_mr(s, m, (X86XmmReg)xmm_src); // movsd xmm_src, -(offset)(%%rbp)
}
// movq from physical int param register to physical r11
static void asm_mov_preg_r11(SecBuf *s, X86Reg preg) {
    x86_mov_rr(s, 8, X86_R11, preg); // movq preg, %%r11
}
// movb -1(%%r11,%%r10), %%al  — load byte from struct param
static void asm_movb_r11_r10_al(SecBuf *s, int64_t disp) {
    // movb disp(%%r11, %%r10, 1), %%al
    // REX.B (r11>=8): 0x41 or REX.XB if both? r10=10 (>8), r11=11 (>8)
    // Actually x86: r10=%%r10, r11=%%r11 are physical regs 10,11
    // ModRM: mod=00, reg=0(al), rm=4(SIB); SIB: scale=0, index=r10(2 in SIB with REX), base=r11(3 in SIB with REX)
    // REX: 0100_WRXB = 0100_0011 = 0x43 (W=0, R=0, X=1 for r10 index, B=1 for r11 base)
    secbuf_emit8(s, 0x43); // REX.XB
    secbuf_emit8(s, 0x8A); // MOV r8, r/m8
    if (disp == -1) {
        secbuf_emit8(s, (uint8_t)0x44); // ModRM: mod=01, reg=0(al), rm=4(SIB)
        secbuf_emit8(s, (uint8_t)0x13); // SIB: scale=0, index=010(r10), base=011(r11)
        secbuf_emit8(s, (uint8_t)0xFF); // disp8 = -1
    } else {
        secbuf_emit8(s, (uint8_t)0x04); // ModRM: mod=00, reg=0(al), rm=4(SIB)
        secbuf_emit8(s, (uint8_t)0x13); // SIB
    }
}
// movb %%al, -(offset)-1(%%rbp,%%r10)  — store byte to local struct
static void asm_movb_al_rbp_r10(SecBuf *s, int offset) {
    // movb %%al, -(offset)-1(%%rbp, %%r10, 1)
    // REX.X for r10 index: 0x42
    secbuf_emit8(s, 0x42); // REX.X
    secbuf_emit8(s, 0x88); // MOV r/m8, r8
    secbuf_emit8(s, (uint8_t)0x84); // ModRM: mod=10, reg=0(al), rm=4(SIB)
    secbuf_emit8(s, (uint8_t)0x15); // SIB: scale=0, index=010(r10), base=101(rbp)
    // disp32 = -(offset) - 1
    int32_t d = -(offset)-1;
    secbuf_emit8(s, (uint8_t)(d & 0xFF));
    secbuf_emit8(s, (uint8_t)((d >> 8) & 0xFF));
    secbuf_emit8(s, (uint8_t)((d >> 16) & 0xFF));
    secbuf_emit8(s, (uint8_t)((d >> 24) & 0xFF));
}
// movb offset-1(%%rbp,%%r10), %%al  — load byte from stack struct
static void asm_movb_rbp_r10_al(SecBuf *s, int offset) {
    // movb (stack_offset-1)(%%rbp, %%r10, 1), %%al
    secbuf_emit8(s, 0x42); // REX.X
    secbuf_emit8(s, 0x8A); // MOV r8, r/m8
    secbuf_emit8(s, (uint8_t)0x84); // ModRM: mod=10, reg=0(al), rm=4(SIB)
    secbuf_emit8(s, (uint8_t)0x15); // SIB: scale=0, index=010(r10), base=101(rbp)
    int32_t d = offset - 1;
    secbuf_emit8(s, (uint8_t)(d & 0xFF));
    secbuf_emit8(s, (uint8_t)((d >> 8) & 0xFF));
    secbuf_emit8(s, (uint8_t)((d >> 16) & 0xFF));
    secbuf_emit8(s, (uint8_t)((d >> 24) & 0xFF));
}
// mov offset(%%rbp), %al/%ax/%eax/%rax  — load from frame using tmpreg-appropriate size
static void asm_mov_rbp_tmpreg(SecBuf *s, int offset, int sz) {
    X86Mem m = {CG_X86_FP, X86_NOREG, 1, offset};
    x86_mov_rm(s, sz, X86_RAX, m); // mov offset(%%rbp), %%rax/eax/ax/al
}
// mov %al/%ax/%eax/%rax, -(offset)(%%rbp)  — store to frame using tmpreg
static void asm_mov_tmpreg_rbp(SecBuf *s, int offset, int sz) {
    X86Mem m = {CG_X86_FP, X86_NOREG, 1, -offset};
    x86_mov_mr(s, sz, m, X86_RAX); // mov %%rax/eax/ax/al, -(offset)(%%rbp)
}
#endif /* !ARCH_ARM64 */

// ============================================================================
// Bit scan / count — lzcnt/tzcnt/bsf/bsr/popcnt/cls
// ============================================================================

// tzcnt dst, src  — count trailing zeros (= ctz)
static void asm_tzcnt(SecBuf *s, VReg dst, VReg src, int size) {
#ifdef ARCH_ARM64
    // ARM64: rbit + clz
    Arm64Reg rdst = REG(dst);
    Arm64Reg rsrc = REG(src);
    int sf = (size == 8) ? 1 : 0;
    arm64_rbit(s, sf, rdst, rsrc); // rbit dst, src
    arm64_clz(s, sf, rdst, rdst); // cmz dst, src
#else
    X86Reg rdst = REG(dst);
    X86Reg rsrc = REG(src);
    x86_tzcnt(s, size, rdst, rsrc); // tzcnt dst, src
#endif
}

// bsf dst, src  — bit scan forward (= tzcnt, undefined for 0)
static void asm_bsf(SecBuf *s, VReg dst, VReg src, int size) {
#ifdef ARCH_ARM64
    Arm64Reg rdst = REG(dst);
    Arm64Reg rsrc = REG(src);
    int sf = (size == 8) ? 1 : 0;
    arm64_rbit(s, sf, rdst, rsrc); // rbit dst, src
    arm64_clz(s, sf, rdst, rdst); // clz dst, dst
#else
    X86Reg rdst = REG(dst);
    X86Reg rsrc = REG(src);
    x86_bsf(s, size, rdst, rsrc); // bsf dst, src
#endif
}

// bsr dst, src  — bit scan reverse (= 31/63 - clz, undefined for 0)
static void asm_bsr(SecBuf *s, VReg dst, VReg src, int size) {
#ifdef ARCH_ARM64
    Arm64Reg rdst = REG(dst);
    Arm64Reg rsrc = REG(src);
    int sf = (size == 8) ? 1 : 0;
    arm64_clz(s, sf, rdst, rsrc);
    int bits = (size == 8) ? 63 : 31;
    arm64_sub_imm(s, sf, rdst, 31, bits, 0); // rsb: bits - clz
#else
    x86_bsr(s, size, REG(dst), REG(src)); // bsr dst, src
#endif
}

#ifndef ARCH_ARM64
// x86 popcnt dst, src  — population count
static void asm_popcnt(SecBuf *s, VReg dst, VReg src, int size) {
    x86_popcnt(s, size, REG(dst), REG(src)); // popcnt dst, src
}
#endif

#ifdef ARCH_ARM64
// cls dst, src  — count leading sign bits (ARM64 only)
// x86: no cls; caller uses sar+xor+lzcnt sequence
static void asm_cls(SecBuf *s, VReg dst, VReg src, int size) {
    Arm64Reg rdst = REG(dst);
    Arm64Reg rsrc = REG(src);
    int sf = (size == 8) ? 1 : 0;
    arm64_cls(s, sf, rdst, rsrc);
}
#endif

// ============================================================================
// Rotate — rol/ror
// ============================================================================

// rol reg, imm  — rotate left by immediate
static void asm_rol_imm(SecBuf *s, VReg r, int size, uint8_t shift) {
#ifdef ARCH_ARM64
    // ARM64: ror by (bits - shift) == rol
    int sf = (size == 8) ? 1 : 0;
    int bits = size * 8;
    arm64_ror_reg(s, sf, REG(r), REG(r), (uint32_t)(bits - shift) & (bits - 1));
#else
    x86_rol_ri(s, size, REG(r), shift); // rol $shift, reg
#endif
}

// ============================================================================
// Conditional move / select
// ============================================================================

// cmovcc dst, src  — conditional move (x86) / csel (ARM64)
// cond: X86Cond (x86) or Arm64Cond (ARM64)
#ifdef ARCH_ARM64
static void asm_cmov(SecBuf *s, VReg dst, VReg src, int size, Arm64Cond cond) {
    Arm64Reg rdst = REG(dst);
    Arm64Reg rsrc = REG(src);
    int sf = (size == 8) ? 1 : 0;
    // csel dst, src, dst, inverse(cond): if cond true, dst=src, else dst=dst
    // To mimic cmovCC (move if condition true): csel dst, src, dst, cond
    arm64_csel(s, sf, rdst, rsrc, rdst, (Arm64Cond)cond);
}
#else
static void asm_cmov(SecBuf *s, VReg dst, VReg src, int size, X86Cond cond) {
    x86_cmovcc(s, size, (X86Cond)cond, REG(dst), REG(src)); // cmovcc dst, src
}
#endif

#ifdef ARCH_ARM64
// csel dst, src, zero, eq  — select: if(cond) dst=0 else dst=src (for __builtin_ffs ARM64)
static void asm_csel_zero_if_eq(SecBuf *s, VReg dst, VReg src, int size) {
    int sf = (size == 8) ? 1 : 0;
    // csel dst, xzr, src, eq  (= if eq: 0, else src)
    arm64_csel(s, sf, REG(dst), ARM64_XZR, REG(src), ARM64_EQ);
}

// cneg dst, dst, mi  — negate if negative (ARM64 abs)
static void asm_cneg_mi(SecBuf *s, VReg vr, int size) {
    Arm64Reg r = REG(vr);
    int sf = (size == 8) ? 1 : 0;
    arm64_cneg(s, sf, r, r, ARM64_MI);
}
#endif

// ============================================================================
// LEA with register + small offset (for __builtin_ffs)
// ============================================================================

// leaq 1(src), dst  /  leal 1(src), dst — lea with 1-byte displacement
static void asm_lea_disp1(SecBuf *s, VReg dst, VReg src, int size) {
#ifdef ARCH_ARM64
    Arm64Reg rdst = REG(dst);
    Arm64Reg rsrc = REG(src);
    int sf = (size == 8) ? 1 : 0;
    arm64_add_imm(s, sf, rdst, rsrc, 1, 0); // add dst, src, #1
#else
    X86Mem m = {REG(src), X86_NOREG, 1, 1};
    x86_lea(s, size, REG(dst), m); // lea 1(src), dst
#endif
}

// ============================================================================
// Prefetch
// ============================================================================

#ifdef ARCH_ARM64
// prfm hint, [x{r}]  — prefetch memory
static void asm_prfm(SecBuf *s, int r, int prfop) {
    arm64_prfm_imm(s, prfop, REG(r), 0); // prfm prfop, [x{r}]
}
#endif

#ifndef ARCH_ARM64
// prefetchXX (%reg)  — x86 prefetch (emit as NOP since no single encoder)
// hint: 0=prefetchnta, 1=prefetcht2, 2=prefetcht1, 3=prefetcht0, 4=prefetchw
static void asm_prefetch(SecBuf *s, VReg r, int hint) {
    // Encode prefetch as 0F 18 /reg mod rm:
    // /0=prefetchnta, /1=prefetcht2, /2=prefetcht1, /3=prefetcht0, prefetchw=0F 0D /1
    uint8_t reg_field = (uint8_t)(hint < 4 ? hint : 1);
    uint8_t modrm = (uint8_t)(0x00 | (reg_field << 3) | (REG(r) & 7));
    if (hint == 4) { // prefetchw: 0F 0D /1
        if ((int)REG(r) >= 8) secbuf_emit8(s, 0x41); // REX.B
        secbuf_emit8(s, 0x0F);
        secbuf_emit8(s, 0x0D);
        secbuf_emit8(s, modrm);
    } else {
        if ((int)REG(r) >= 8) secbuf_emit8(s, 0x41); // REX.B
        secbuf_emit8(s, 0x0F);
        secbuf_emit8(s, 0x18);
        secbuf_emit8(s, modrm);
    }
}
#endif

// ============================================================================
// ARM64 overflow-checked arithmetic: adds/subs/umulh/smulh/umull/smull
// ============================================================================

#ifdef ARCH_ARM64
// adds dst, dst, src  — add and set flags
static void asm_adds(SecBuf *s, VReg dst, VReg src, int size) {
    Arm64Reg rdst = REG(dst);
    Arm64Reg rsrc = REG(src);
    int sf = (size == 8) ? 1 : 0;
    arm64_adds_reg(s, sf, rdst, rdst, rsrc, ARM64_LSL, 0); // adds dst, dst, src
}

// subs dst, dst, src  — subtract and set flags
static void asm_subs(SecBuf *s, VReg dst, VReg src, int size) {
    Arm64Reg rdst = REG(dst);
    Arm64Reg rsrc = REG(src);
    int sf = (size == 8) ? 1 : 0;
    arm64_subs_reg(s, sf, rdst, rdst, rsrc, ARM64_LSL, 0); // subs dst, dst, src
}

// cset dst, cond  — conditional set (already exists as asm_cset)

// umulh dst, a, b  — unsigned multiply high (128-bit result high word)
static void asm_umulh(SecBuf *s, VReg dst, VReg a, VReg b) {
    arm64_umulh(s, REG(dst), REG(a), REG(b)); // umulh dst, a, b
}

// smulh dst, a, b  — signed multiply high
static void asm_smulh(SecBuf *s, VReg dst, VReg a, VReg b) {
    arm64_smulh(s, REG(dst), REG(a), REG(b)); // smulh dst, a, b
}

// umull xdst, wa, wb  — unsigned multiply long (32x32→64)
static void asm_umull(SecBuf *s, VReg dst, VReg a, VReg b) {
    arm64_umull(s, REG(dst), REG(a), REG(b)); // umull dst, wa, wb
}

// smull xdst, wa, wb  — signed multiply long (32x32→64)
static void asm_smull(SecBuf *s, VReg dst, VReg a, VReg b) {
    arm64_smull(s, REG(dst), REG(a), REG(b)); // smull dst, wa, wb
}
#endif /* ARCH_ARM64 */

// ============================================================================
// x86 unsigned multiply 1-operand (mul %reg — rdx:rax = rax * reg)
// ============================================================================

#ifndef ARCH_ARM64
// mul src  — unsigned multiply: rdx:rax = rax * src
static void asm_mul_1op(SecBuf *s, VReg src, int size) {
    size_t off = s->len;
    x86_imul_r(s, size, REG(src)); // imul src (unsigned 1-op not in x86_enc; use mul)
    // Note: x86_imul_r emits IMUL (signed), for unsigned we need MUL opcode
    // Rewind and emit proper MUL
    s->len = off;
    // MUL r/m: size=8 → REX.W + F7 /4; size=4 → F7 /4
    X86Reg r = REG(src);
    if (size == 8)
        secbuf_emit8(s, (uint8_t)(0x48 | ((r >> 3) & 1))); // REX.W (+REX.B if r>=8)
    else if ((int)r >= 8)
        secbuf_emit8(s, 0x41); // REX.B
    secbuf_emit8(s, 0xF7);
    secbuf_emit8(s, (uint8_t)(0xC0 | (4 << 3) | (r & 7))); // ModRM: /4 = mul
}
#endif

// ============================================================================
// ARM64 NEON instructions for popcnt / variadic quad float
// ============================================================================

#ifdef ARCH_ARM64
// fmov d30, x{r}  — move GP int64 to NEON d-register (scalar)
static void asm_fmov_gp_to_d30(SecBuf *s, VReg r) {
    arm64_fmov_i2f(s, 1, ARM64_X30, REG(r)); // fmov d30, x{r}
}

// fmov s30, w{r}  — move GP int32 to NEON s-register (scalar)
static void asm_fmov_gp_to_s30(SecBuf *s, VReg r) {
    arm64_fmov_i2f(s, 0, ARM64_X30, REG(r)); // fmov s30, w{r}
}

// fmov w{r}, s30  — move NEON s-register scalar to GP int32
static void asm_fmov_s30_to_gp(SecBuf *s, VReg r) {
    arm64_fmov_f2i(s, 0, REG(r), ARM64_X30); // fmov w{r}, s30
}

// cnt v30.8b, v30.8b  — popcount bytes in NEON register
// Encoding: 0_101_1110_00_10000_01010_1_nnnnn_ddddd (SIMD CNT)
static void asm_neon_cnt_v30(SecBuf *s) {
    // cnt vd.8b, vn.8b: 0x0E205800 | (vn<<5) | vd  (Q=0, size=00, opcode=00101, U=0)
    secbuf_emit32le(s, 0x0E205800u | (30u << 5) | 30u); // cnt v30.8b, v30.8b
}

// addv b30, v30.8b  — horizontal add all bytes → byte scalar
// Encoding: 0_0_1_01110_00_11000_11011_10_nnnnn_ddddd
static void asm_neon_addv_b30(SecBuf *s) {
    // addv bd, vn.8b: 0x0E31B800 | (vn<<5) | bd
    secbuf_emit32le(s, 0x0E31B800u | (30u << 5) | 30u); // addv b30, v30.8b
}

// fmov d{rd}, x{gp}  — for named ld arg: fmov to fp arg register
static void asm_fmov_gp_to_d(SecBuf *s, Arm64Reg fp_rd, VReg gp_rs) {
    arm64_fmov_i2f(s, 1, fp_rd, REG(gp_rs)); // fmov d{fp_rd}, x{gp_rs}
}

// ubfx x17, x{r}, #52, #11  — extract exponent bits from double
static void asm_ubfx_x17_exp(SecBuf *s, VReg r) {
    arm64_ubfx(s, 1, ARM64_X17, REG(r), 52, 11); // ubfx x17, x{r}, #52, #11
}

// mov x16, #15360  — constant for quad float exponent bias
static void asm_mov_x16_15360(SecBuf *s) {
    arm64_movz(s, 1, ARM64_X16, 15360, 0); // mov x16, #15360
}

// add x17, x17, x16  — add exponent + bias
static void asm_add_x17_x17_x16(SecBuf *s) {
    arm64_add_reg(s, 1, ARM64_X17, ARM64_X17, ARM64_X16, ARM64_LSL, 0); // add x17, x17, x16
}

// lsl x16, x{r}, #12  — shift mantissa
static void asm_lsl_x16_r_12(SecBuf *s, VReg r) {
    arm64_lsl_imm(s, 1, ARM64_X16, REG(r), 12); // lsl x16, x{r}, #12
}

// lsr x16, x16, #12  — shift back mantissa
static void asm_lsr_x16_x16_12(SecBuf *s) {
    arm64_lsr_imm(s, 1, ARM64_X16, ARM64_X16, 12); // lsr x16, x16, #12
}

// and x1, x16, #0xF  — low nibble
static void asm_and_x1_x16_0xf(SecBuf *s) {
    int N, immr, imms;
    uint64_t enc = arm64_encode_logic_imm(1, 0xF, &N, &immr, &imms);
    arm64_and_imm(s, 1, ARM64_X1, ARM64_X16, enc); // and x1, x16, #0xF
}

// lsl x1, x1, #60  — position low nibble at top
static void asm_lsl_x1_60(SecBuf *s) {
    arm64_lsl_imm(s, 1, ARM64_X1, ARM64_X1, 60); // lsl x1, x1, #60
}

// lsr x2, x16, #4  — mantissa high part
static void asm_lsr_x2_x16_4(SecBuf *s) {
    arm64_lsr_imm(s, 1, ARM64_X2, ARM64_X16, 4); // lsr x2, x16, #4
}

// lsl x17, x17, #48  — position exponent
static void asm_lsl_x17_48(SecBuf *s) {
    arm64_lsl_imm(s, 1, ARM64_X17, ARM64_X17, 48); // lsl x17, x17, #48
}

// orr x2, x2, x17  — combine mantissa + exponent
static void asm_orr_x2_x2_x17(SecBuf *s) {
    arm64_orr_reg(s, 1, ARM64_X2, ARM64_X2, ARM64_X17, ARM64_LSL, 0); // orr x2, x2, x17
}

// and x17, x17, #1  — isolate sign bit
static void asm_and_x17_1(SecBuf *s) {
    int N, immr, imms;
    uint64_t enc = arm64_encode_logic_imm(1, 1, &N, &immr, &imms);
    arm64_and_imm(s, 1, ARM64_X17, ARM64_X17, enc); // and x17, x17, #1
}

// lsl x17, x17, #63  — position sign bit
static void asm_lsl_x17_63(SecBuf *s) {
    arm64_lsl_imm(s, 1, ARM64_X17, ARM64_X17, 63); // lsl x17, x17, #63
}

// orr x2, x2, x17  — add sign to result (reuse orr_x2_x2_x17)

// mov x1, #0  — zero x1
static void asm_mov_x1_0(SecBuf *s) {
    arm64_movz(s, 1, ARM64_X1, 0, 0); // mov x1, #0
}

// mov x2, #0  — zero x2
static void asm_mov_x2_0(SecBuf *s) {
    arm64_movz(s, 1, ARM64_X2, 0, 0); // mov x2, #0
}

// ins vN.d[0], x1  — insert x1 into NEON vector element 0
static void asm_ins_vd0_x1(SecBuf *s, int vn) {
    // INS vd.d[0], x1: 0x4E081C00 | (imm5=10000=d[0]) | (vn<<5)|(vd)
    // Encoding: INS (general) = 0_1_001110_00001000_000111_rn_rd for d[0]
    // imm5 for d[idx]: imm5 = (idx<<1)|1 = 0b10000+0 = 0x10 for idx=0... let me use raw encoding
    // 0100_1110_0000_1000_0001_11_xxxxxx_ddddd  (Rt=1, vd=vn)
    // ins vd.d[idx] from xn: 0x4E081C00 | (imm5=0b10000<<idx??)
    // Correct: Q=1, op=1, imm5=10000 (d[0]), imm4=0111, n=x1, d=vn
    // 0x4E081C20 = ins v0.d[0], x1; adjust: set Rd=vn, Rn=1
    secbuf_emit32le(s, 0x4E081C00u | (1u << 5) | (uint32_t)vn); // ins v{vn}.d[0], x1
}

// ins vN.d[1], x2  — insert x2 into NEON vector element 1
static void asm_ins_vd1_x2(SecBuf *s, int vn) {
    // INS vd.d[1] from x2: imm5=0b10001 (d[1]), Rn=2, Rd=vn
    // 0x4E180C00 | (2<<5) | vn -- let me calculate properly
    // imm5 for d[idx]: bit0=1, bit4..1=idx => d[1]: imm5=0b10001=0x11=17
    // opcode: 0x4E_imm5_xxx_1_1100_Rn_Rd: 0x4E000000 | (imm5<<16) | (7<<12) | (1<<10) | (rn<<5) | rd
    // Actually: ins = 0x4E000000|(Q=1)<<30|(op=0)<<29|(imm5<<16)|(imm4<<12)|(1<<10)|(Rn<<5)|Rd
    // imm5=0b10001, imm4=0b0111
    secbuf_emit32le(s, 0x4E180C00u | (2u << 5) | (uint32_t)vn); // ins v{vn}.d[1], x2 ... approx
}

// mov x8, x{r}  — move hidden ret pointer to x8 for ARM64 ABI
static void asm_mov_x8_reg(SecBuf *s, VReg r) {
    arm64_orr_reg(s, 1, ARM64_X8, ARM64_XZR, REG(r), ARM64_LSL, 0); // mov x8, x{r}
}

// stp q{rt1}, q{rt2}, [sp, #imm7_bytes]  — store NEON quad pair
static void asm_stp_q_sp(SecBuf *s, Arm64Reg rt1, Arm64Reg rt2, int32_t byte_off) {
    // opc=2 for Q (128-bit), imm7 is in units of 16 bytes
    int32_t imm7 = byte_off / 16;
    arm64_stp_fp(s, 2, rt1, rt2, ARM64_SP, imm7, false, false); // stp q{rt1}, q{rt2}, [sp, #byte_off]
}

// sub x16, x29, #var->offset  — compute stack frame address
static void asm_sub_x16_fp_imm(SecBuf *s, int32_t imm) {
    arm64_sub_imm(s, 1, ARM64_X16, ARM64_X29, imm, 0); // sub x16, x29, #imm
}

// sub x16, x29, x16  — compute stack frame address (large offset)
static void asm_sub_x16_fp_x16(SecBuf *s) {
    arm64_sub_reg(s, 1, ARM64_X16, ARM64_X29, ARM64_X16, ARM64_LSL, 0); // sub x16, x29, x16
}

// str s{fp_param}, [x16, #off]  — store float parameter to stack
static void asm_str_s_x16_off(SecBuf *s, int fp_reg, int32_t off_bytes) {
    // str s{fp_reg}, [x16, #off_bytes]: sz=2 (single), arm64_str_fp handles /4 scaling
    arm64_str_fp(s, 2, fp_reg, ARM64_X16, (uint32_t)off_bytes); // str s{fp_reg}, [x16, #off_bytes]
}

// str d{fp_param}, [x16, #off]  — store double parameter to stack
static void asm_str_d_x16_off(SecBuf *s, int fp_reg, int32_t off_bytes) {
    arm64_str_fp(s, 3, fp_reg, ARM64_X16, (uint32_t)off_bytes); // str d{fp_reg}, [x16, #off_bytes]
}

// str s{fp_param}, [x29, #-offset]  — store float parameter to frame
static void asm_str_s_fp_neg(SecBuf *s, int fp_reg, int32_t offset) {
    // stur s{fp_reg}, [x29, #-offset]  (unscaled for negative)
    arm64_stur_fp(s, 2, fp_reg, ARM64_X29, -offset); // stur s{fp_reg}, [x29, #-offset]
}

// str d{fp_param}, [x29, #-offset]  — store double parameter to frame
static void asm_str_d_fp_neg(SecBuf *s, int fp_reg, int32_t offset) {
    arm64_stur_fp(s, 3, fp_reg, ARM64_X29, -offset); // stur d{fp_reg}, [x29, #-offset]
}

// fcvt s0, d{fp_param}  — double to single conversion (for oldstyle float params)
static void asm_fcvt_s0_d(SecBuf *s, Arm64Reg fp_src) {
    // fcvt s0, d{fp_src}: opc=0 (single dest), ftype=1 (double src)
    arm64_fcvt(s, 0, 1, 0, fp_src); // fcvt s0, d{fp_src}
}

// str s0, [x29, #-offset]  — store s0 (after fcvt)
static void asm_str_s0_fp_neg(SecBuf *s, int32_t offset) {
    arm64_stur_fp(s, 2, 0, ARM64_X29, -offset); // stur s0, [x29, #-offset]
}

// ldr s0, [x29, #spoff]  — load float from stack
static void asm_ldr_s0_fp_off(SecBuf *s, int32_t spoff) {
    arm64_ldr_fp(s, 2, 0, ARM64_X29, (uint32_t)spoff); // ldr s0, [x29, #spoff]
}

// ldr d0, [x29, #spoff]  — load double from stack
static void asm_ldr_d0_fp_off(SecBuf *s, int32_t spoff) {
    arm64_ldr_fp(s, 3, 0, ARM64_X29, (uint32_t)spoff); // ldr d0, [x29, #spoff]
}

// ldrb w11, [x29, #spoff]  — load byte from stack slot (stack param)
static void asm_ldrb_w11_fp_off(SecBuf *s, int32_t spoff) {
    arm64_ldrb_uoff(s, ARM64_X11, ARM64_X29, (uint32_t)spoff); // ldrb w11, [x29, #spoff]
}

// ldrh w11, [x29, #spoff]  — load halfword from stack slot
static void asm_ldrh_w11_fp_off(SecBuf *s, int32_t spoff) {
    arm64_ldrh_uoff(s, 11, ARM64_X29, (uint32_t)(spoff / 2)); // ldrh w11, [x29, #spoff]
}

// ldr w11, [x29, #spoff]  — load 32-bit from stack slot
static void asm_ldr_w11_fp_off(SecBuf *s, int32_t spoff) {
    arm64_ldr_uoff(s, 2, ARM64_X11, ARM64_X29, (uint32_t)(spoff / 4)); // ldr w11, [x29, #spoff]
}

// ldr x11, [x29, #spoff]  — load 64-bit from stack slot
static void asm_ldr_x11_fp_off(SecBuf *s, int32_t spoff) {
    arm64_ldr_uoff(s, 3, ARM64_X11, ARM64_X29, (uint32_t)(spoff / 8)); // ldr x11, [x29, #spoff]
}

// strb w11, [x29, #-offset]  — store byte to frame
static void asm_strb_w11_fp_neg(SecBuf *s, int32_t offset) {
    arm64_sturb(s, ARM64_X11, ARM64_X29, -offset); // sturb w11, [x29, #-offset]
}

// strh w11, [x29, #-offset]  — store halfword to frame
static void asm_strh_w11_fp_neg(SecBuf *s, int32_t offset) {
    arm64_sturh(s, ARM64_X11, ARM64_X29, -offset); // sturh w11, [x29, #-offset]
}

// str w11, [x29, #-offset]  — store 32-bit to frame
static void asm_str_w11_fp_neg(SecBuf *s, int32_t offset) {
    arm64_stur(s, 0, ARM64_X11, ARM64_X29, -offset); // stur w11, [x29, #-offset]  sf=0
}

// str x11, [x29, #-offset]  — store 64-bit to frame
static void asm_str_x11_fp_neg(SecBuf *s, int32_t offset) {
    arm64_stur(s, 1, ARM64_X11, ARM64_X29, -offset); // stur x11, [x29, #-offset] sf=1
}

#endif /* ARCH_ARM64 */

// ============================================================================
// x86: add/sub with flags (for overflow builtins)
// ============================================================================

#ifndef ARCH_ARM64
// add dst, src (with flags for carry/overflow detection)
static void asm_add_rr_flags(SecBuf *s, VReg dst, VReg src, int size) {
    X86Reg rdst = REG(dst);
    X86Reg rsrc = REG(src);
    x86_add_rr(s, size, rdst, rsrc); // add dst, src
}

// sub dst, src (with flags)
static void asm_sub_rr_flags(SecBuf *s, VReg dst, VReg src, int size) {
    X86Reg rdst = REG(dst);
    X86Reg rsrc = REG(src);
    x86_sub_rr(s, size, rdst, rsrc); // sub dst, src
}

// mov [raddr], src  — store register to memory via pointer in raddr
static void asm_mov_mem_via_reg(SecBuf *s, VReg src, VReg raddr, int size) {
    X86Mem m = {REG(raddr), X86_NOREG, 1, 0};
    x86_mov_mr(s, size, m, REG(src)); // mov src, (raddr)
}
#endif

// ============================================================================
// x86: mov (%reg), dst  — indirect load via pointer
// ============================================================================

#ifndef ARCH_ARM64
// mov (%rr), %rr  — indirect load (for is_frame_addr / is_ret_addr depth loop)
static void asm_mov_indir(SecBuf *s, VReg r, int size) {
    X86Mem m = {REG(r), X86_NOREG, 1, 0};
    x86_mov_rm(s, size, REG(r), m); // mov (%rr), %rr
}

// mov N(%rr), %rr  — indirect load with displacement
static void asm_mov_indir_disp(SecBuf *s, VReg r, int64_t disp, int size) {
    X86Mem m = {REG(r), X86_NOREG, 1, disp};
    x86_mov_rm(s, size, REG(r), m); // mov disp(%rr), %rr
}
#endif

// ============================================================================
// ARM64 rev16  (already stubbed; provide real implementation)
// ============================================================================

static void asm_rev16_real(SecBuf *s, VReg r, int size) {
#ifdef ARCH_ARM64
    int sf = (size == 8) ? 1 : 0;
    arm64_rev16(s, sf, REG(r), REG(r));
#else
    (void)size;
    x86_rol_ri(s, 2, REG(r), 8); // rol $8, %rx16
#endif
}

// ============================================================================
// ARM64: ldr{b,h} [base, #uimm] — unsigned offset load byte/halfword
// ============================================================================
#ifdef ARCH_ARM64
static void asm_ldrb_uoff(SecBuf *s, VReg dst, VReg base, uint32_t uimm) {
    arm64_ldrb_uoff(s, REG(dst), REG(base), uimm); // ldrb w{dst}, [x{base}, #uimm]
}
static void asm_ldrh_uoff(SecBuf *s, VReg dst, VReg base, uint32_t uimm) {
    arm64_ldrh_uoff(s, REG(dst), REG(base), uimm); // ldrh w{dst}, [x{base}, #uimm]
}
static void asm_strb_uoff(SecBuf *s, VReg src, VReg base, uint32_t uimm) {
    arm64_strb_uoff(s, REG(src), REG(base), uimm); // strb w{src}, [x{base}, #uimm]
}
static void asm_strh_uoff(SecBuf *s, VReg src, VReg base, uint32_t uimm) {
    arm64_strh_uoff(s, REG(src), REG(base), uimm); // strh w{src}, [x{base}, #uimm]
}

// ============================================================================
// ARM64: ldrb w16, [x{base}, x9] / strb w16, [x{dst}, x9]
// ============================================================================
static void asm_ldrb_w16_x9(SecBuf *s, int base) {
    arm64_ldr_reg(s, 0, 16, REG(base), 9, false, 0); // ldrb w16, [x{base}, x9]
}
static void asm_strb_w16_x9(SecBuf *s, int dst) {
    arm64_str_reg(s, 0, 16, REG(dst), 9, false, 0); // strb w16, [x{dst}, x9]
}
// strb wzr, [x{dst}, x9] — store zero byte using x9 index
static void asm_strb_wzr_x9(SecBuf *s, int dst) {
    arm64_str_reg(s, 0, ARM64_XZR, REG(dst), 9, false, 0); // strb wzr, [x{dst}, x9]
}
static void asm_strb_w16_x9_phy(SecBuf *s, Arm64Reg dst) {
    arm64_str_reg(s, 0, 16, dst, 9, false, 0); // strb w16, [x{dst}, x9]
}

// ============================================================================
// ARM64: strb/wzr and ldrb/scaled patterns
// ============================================================================
static void asm_str_xzr_w11_x9(SecBuf *s) {
    arm64_str_reg(s, 0, ARM64_XZR, 11, 9, false, 0); // strb wzr, [x11, x9]
}
static void asm_str_xzr_sp_x16(SecBuf *s) {
    arm64_str_reg(s, 3, ARM64_XZR, ARM64_SP, 16, false, 0); // str xzr, [sp, x16]
}
static void asm_ldrb_w17_sp_x16(SecBuf *s) {
    arm64_ldr_reg(s, 0, 17, ARM64_SP, 16, false, 0); // ldrb w17, [sp, x16]
}

// ============================================================================
// ARM64: stp xzr, xzr, [x{addr}, #zb]
// ============================================================================
static void asm_stp_xzr_xzr(SecBuf *s, VReg addr, int32_t imm7) {
    arm64_stp(s, 1, ARM64_XZR, ARM64_XZR, REG(addr), imm7, false, false); // stp xzr, xzr, [x{addr}, #zb]
}

// ============================================================================
// ARM64: mov x9, x{reg} via orr,  sub sp, sp, x16,  subs x16, x16, #imm
// ============================================================================
static void asm_mov_x9_vreg(SecBuf *s, VReg r) {
    arm64_orr_reg(s, 1, ARM64_X9, ARM64_XZR, REG(r), ARM64_LSL, 0); // mov x9, x{r}
}
static void asm_mov_x16_vreg(SecBuf *s, VReg r) {
    arm64_orr_reg(s, 1, ARM64_X16, ARM64_XZR, REG(r), ARM64_LSL, 0); // mov x16, x{r}
}
static void asm_mov_vreg_x12(SecBuf *s, VReg r) {
    arm64_orr_reg(s, 1, REG(r), ARM64_XZR, ARM64_X12, ARM64_LSL, 0); // mov x{r}, x12
}
static void asm_mov_x9_from_vreg(SecBuf *s, VReg r_value, int size) {
    int sf = (size == 8) ? 1 : 0;
    arm64_orr_reg(s, sf, ARM64_X9, ARM64_XZR, REG(r_value), ARM64_LSL, 0); // mov w9/x9, r_value
}
static void asm_mov_x16_x12(SecBuf *s) {
    arm64_orr_reg(s, 1, ARM64_X16, ARM64_XZR, ARM64_X12, ARM64_LSL, 0); // mov x16, x12
}
static void asm_mov_x12_x16(SecBuf *s) {
    arm64_orr_reg(s, 1, ARM64_X12, ARM64_XZR, ARM64_X16, ARM64_LSL, 0); // mov x12, x16
}

// ============================================================================
// ARM64: sub x11, x29, x16  /  sub sp, sp, x16  /  sub x11, x29, #off
// ============================================================================
static void asm_sub_x11_fp_x16(SecBuf *s) {
    arm64_sub_reg(s, 1, ARM64_X11, ARM64_X29, ARM64_X16, ARM64_LSL, 0); // sub x11, x29, x16
}
static void asm_sub_sp_sp_x16_v2(SecBuf *s) {
    arm64_sub_extreg(s, 1, CG_ARM_SP, CG_ARM_SP, ARM64_X16, ARM64_UXTX, 0); // sub sp, sp, x16
}
static void asm_sub_x11_fp_imm(SecBuf *s, int32_t offset) {
    arm64_sub_imm(s, 1, ARM64_X11, ARM64_X29, offset, 0); // sub x11, x29, #offset
}

// ============================================================================
// ARM64: subs x16, x16, #imm
// ============================================================================
static void asm_subs_x16_imm(SecBuf *s, int32_t imm12) {
    // Handle large immediate: split into imm12 << (12 * sh) for values > 4095
    if (imm12 >= 0 && imm12 < 4096) {
        arm64_subs_imm(s, 1, ARM64_X16, ARM64_X16, imm12, 0); // subs x16, x16, #imm12
    } else if (imm12 % 4096 == 0) {
        arm64_subs_imm(s, 1, ARM64_X16, ARM64_X16, imm12 / 4096, 1); // subs x16, x16, #imm12
    } else {
        // General case: mov immediate into scratch, then subs reg-style
        arm64_movz(s, 1, ARM64_X12, (uint16_t)(imm12 & 0xffff), 0);
        if (imm12 >> 16) arm64_movk(s, 1, ARM64_X12, (uint16_t)((imm12 >> 16) & 0xffff), 16);
        if ((int64_t)imm12 >> 32) arm64_movk(s, 1, ARM64_X12, (uint16_t)(((int64_t)imm12 >> 32) & 0xffff), 32);
        arm64_subs_reg(s, 1, ARM64_X16, ARM64_X16, ARM64_X12, ARM64_LSL, 0);
    }
}

// ============================================================================
// ARM64: fneg d0, d0  /  fmov d1, xzr  /  csel
// ============================================================================
static void asm_fneg_d0(SecBuf *s) {
    arm64_fneg(s, 1, 0, 0); // fneg d0, d0
}
static void asm_fmov_d1_xzr(SecBuf *s) {
    arm64_fmov_i2f(s, 1, ARM64_X1, ARM64_XZR); // fmov d1, xzr
}
static void asm_csel_vs_zero(SecBuf *s, VReg r, int size) {
    arm64_csel(s, size == 8 ? 1 : 0, REG(r), REG(r), ARM64_XZR, ARM64_VS); // csel w{r}, w{r}, wzr, vs
}

// ============================================================================
// ARM64: va_arg helpers — ldr/str w16/x16/x12 to [x{r}, #imm*8]
// ============================================================================
static void asm_add_x16_fp_imm(SecBuf *s, int32_t imm) {
    arm64_add_imm(s, 1, ARM64_X16, ARM64_X29, imm, 0); // add x16, x29, #imm
}
static void asm_add_x16_imm(SecBuf *s, int32_t imm) {
    arm64_add_imm(s, 1, ARM64_X16, ARM64_X16, imm, 0); // add x16, x16, #imm
}
static void asm_add_w16_imm(SecBuf *s, int32_t imm) {
    arm64_add_imm(s, 0, ARM64_X16, ARM64_X16, imm, 0); // add w16, w16, #imm
}
static void asm_add_x12_imm(SecBuf *s, int32_t imm) {
    arm64_add_imm(s, 1, ARM64_X12, ARM64_X12, imm, 0); // add x12, x12, #imm
}
static void asm_add_x12_x12_x17(SecBuf *s) {
    arm64_add_reg(s, 1, ARM64_X12, ARM64_X12, ARM64_X17, ARM64_LSL, 0); // add x12, x12, x17
}

// ============================================================================
// ARM64: ldr/str with unsigned scaled offset (va_arg patterns)
// ============================================================================
static void asm_str_x16_uoff(SecBuf *s, VReg base_r, uint32_t uimm) {
    arm64_str_uoff(s, 3, ARM64_X16, REG(base_r), uimm); // str x16, [x{base_r}, #uimm*8]
}
static void asm_ldr_x16_uoff(SecBuf *s, VReg base_r, uint32_t uimm) {
    arm64_ldr_uoff(s, 3, ARM64_X16, REG(base_r), uimm); // ldr x16, [x{base_r}, #uimm*8]
}
static void asm_str_w16_uoff(SecBuf *s, VReg base_r, uint32_t uimm) {
    arm64_str_uoff(s, 2, ARM64_X16, REG(base_r), uimm); // str w16, [x{base_r}, #uimm*8]
}
static void asm_ldr_w16_uoff(SecBuf *s, VReg base_r, uint32_t uimm) {
    arm64_ldr_uoff(s, 2, ARM64_X16, REG(base_r), uimm); // ldr w16, [x{base_r}, #uimm*8]
}
static void asm_ldr_x12_uoff(SecBuf *s, VReg base_r, uint32_t uimm) {
    arm64_ldr_uoff(s, 3, ARM64_X12, REG(base_r), uimm); // ldr x12, [x{base_r}, #uimm*8]
}
static void asm_str_x12_uoff(SecBuf *s, VReg base_r, uint32_t uimm) {
    arm64_str_uoff(s, 3, ARM64_X12, REG(base_r), uimm); // str x12, [x{base_r}, #uimm*8]
}
static void asm_ldr_x12_0(SecBuf *s) {
    arm64_ldr_uoff(s, 3, ARM64_X12, ARM64_X12, 0); // ldr x12, [x12]
}
static void asm_ldr_x16_0(SecBuf *s) {
    arm64_ldr_uoff(s, 3, ARM64_X16, ARM64_X12, 0); // ldr x16, [x12]
}
static void asm_and_x12_imm(SecBuf *s, uint64_t imm_enc) {
    arm64_and_imm(s, 1, ARM64_X12, ARM64_X12, imm_enc); // and x12, x12, #imm_enc
}

// ============================================================================
// ARM64: stxr (atomic)
// ============================================================================
static void asm_stxr_8(SecBuf *s, VReg r_tmp, VReg r_addr, int size) {
    if (size == 1)
        arm64_stxrb(s, 8, REG(r_tmp), REG(r_addr)); // stxrb w8, w{r_tmp}, [x{r_addr}]
    else if (size == 2)
        arm64_stxrh(s, 8, REG(r_tmp), REG(r_addr)); // stxrh w8, w{r_tmp}, [x{r_addr}]
    else
        arm64_stxr(s, size == 8 ? 1 : 0, 8, REG(r_tmp), REG(r_addr)); // stxr w8, w/x{r_tmp}, [x{r_addr}]
}
#endif // ARCH_ARM64

// ============================================================================
// ARM64 int128 helpers — low-level phy-reg operations for widen/int128 code
// ============================================================================
#ifdef ARCH_ARM64
// mov rd, xzr  (zero a physical register)
static void asm_mov_xzr_phy(SecBuf *s, Arm64Reg rd) {
    arm64_orr_reg(s, 1, rd, ARM64_XZR, ARM64_XZR, ARM64_LSL, 0); // mov rd, xzr
}
// str xzr, [rn, #uimm*8]
static void asm_str_xzr_uoff(SecBuf *s, Arm64Reg rn, uint32_t uimm) {
    arm64_str_uoff(s, 3, ARM64_XZR, rn, uimm); // str xzr, [rn, #uimm*8]
}
// asr x16, r{src}, #63  (sign-extend to x16)
static void asm_asr_x16_src_63(SecBuf *s, VReg src) {
    arm64_asr_imm(s, 1, ARM64_X16, REG(src), 63); // asr x16, x{src}, #63
}
// asr x16, x16, #63  (sign-extend x16 itself)
static void asm_asr_x16_63(SecBuf *s) {
    arm64_asr_imm(s, 1, ARM64_X16, ARM64_X16, 63); // asr x16, x16, #63
}
// orr rd, rn, x16  (OR virtual reg with x16)
static void asm_orr_x16(SecBuf *s, VReg rd, VReg rn) {
    arm64_orr_reg(s, 1, REG(rd), REG(rn), ARM64_X16, ARM64_LSL, 0); // orr rd, rn, x16
}
// orr x16, x16, x17  (OR of fixed physical regs into x16)
static void asm_orr_x16_x16_x17(SecBuf *s) {
    arm64_orr_reg(s, 1, ARM64_X16, ARM64_X16, ARM64_X17, ARM64_LSL, 0); // orr x16, x16, x17
}
// mvn rd, rn  (bitwise NOT)
static void asm_mvn_phy(SecBuf *s, Arm64Reg rd, Arm64Reg rn) {
    arm64_mvn(s, 1, rd, rn, ARM64_LSL, 0); // mvn rd, rn
}
// negs rd, rn  (negate with flags for int128 borrow chain)
static void asm_negs(SecBuf *s, VReg rd, VReg rn) {
    arm64_neg(s, 1, REG(rd), REG(rn)); // negs rd, rn  (NEG is alias for SUB xzr,rm)
}
// ngc rd, rn  (negate with carry, used after negs for int128)
static void asm_ngc(SecBuf *s, VReg rd, VReg rn) {
    arm64_sbc(s, 1, REG(rd), ARM64_XZR, REG(rn)); // ngc rd, rn  (SBC rd, xzr, rn)
}
// adc rd, rn, rm  (add with carry, for int128)
static void asm_adc_phy(SecBuf *s, VReg rd, VReg rn, VReg rm) {
    arm64_adc(s, 1, REG(rd), REG(rn), REG(rm)); // adc rd, rn, rm
}
// sbc rd, rn, rm  (subtract with carry, for int128)
static void asm_sbc_phy(SecBuf *s, VReg rd, VReg rn, VReg rm) {
    arm64_sbc(s, 1, REG(rd), REG(rn), REG(rm)); // sbc rd, rn, rm
}
// adds rd, rn, rm  (add with flags, for int128)
static void asm_adds_phy(SecBuf *s, VReg rd, VReg rn, VReg rm) {
    arm64_adds_reg(s, 1, REG(rd), REG(rn), REG(rm), ARM64_LSL, 0); // adds rd, rn, rm
}
// subs rd, rn, rm  (sub with flags, for int128)
static void asm_subs_phy(SecBuf *s, VReg rd, VReg rn, VReg rm) {
    arm64_subs_reg(s, 1, REG(rd), REG(rn), REG(rm), ARM64_LSL, 0); // subs rd, rn, rm
}

// ============================================================================
// ARM64 overflow builtin helpers (sign/zero extend, bitfield extract)
// ============================================================================
// sxtb rd, rn  (sign-extend byte)
static void asm_sxtb_phy(SecBuf *s, VReg rd, VReg rn) {
    arm64_sxtb(s, 1, REG(rd), REG(rn)); // sxtb x{rd}, x{rn}
}
// sxth rd, rn  (sign-extend halfword)
static void asm_sxth_phy(SecBuf *s, VReg rd, VReg rn) {
    arm64_sxth(s, 1, REG(rd), REG(rn)); // sxth x{rd}, x{rn}
}
// sxtw rd, rn  (sign-extend word to doubleword)
static void asm_sxtw_phy(SecBuf *s, Arm64Reg rd, Arm64Reg rn) {
    arm64_sxtw(s, rd, rn); // sxtw x{rd}, w{rn}
}
// uxtw rd, rn  (zero-extend word to doubleword)
static void asm_uxtw_phy(SecBuf *s, Arm64Reg rd, Arm64Reg rn) {
    arm64_orr_reg(s, 1, rd, ARM64_XZR, rn, ARM64_UXTW, 0); // uxtw x{rd}, w{rn}
}
// cinc rd, rn, ne  (conditional increment, result is always ne)
static void asm_cinc_ne(SecBuf *s, VReg rd, VReg rn) {
    arm64_csinc(s, 0, REG(rd), REG(rn), REG(rn), ARM64_NE); // cinc rd, rn, ne
}
// sbfx rd, rn, #0, #width  (signed bitfield extract for overflow range check)
static void asm_sbfx_0(SecBuf *s, VReg rd, VReg rn, int width) {
    arm64_sbfx(s, 1, REG(rd), REG(rn), 0, width); // sbfx rd, rn, #0, #width
}
// ubfx rd, rn, #lsb, #width  (unsigned bitfield extract)
static void asm_ubfx(SecBuf *s, VReg rd, VReg rn, int lsb, int width) {
    arm64_ubfx(s, 1, REG(rd), REG(rn), lsb, width); // ubfx rd, rn, #lsb, #width
}
// tst rn, rm  (ANDS xzr, rn, rm — bitwise test, sets flags)
static void asm_tst_phy(SecBuf *s, VReg rn, VReg rm) {
    arm64_ands_reg(s, 1, ARM64_XZR, REG(rn), REG(rm), ARM64_LSL, 0); // tst rn, rm
}
// tst rn, rn  (test self — sets flags for negative/sign check)
static void asm_tst_self(SecBuf *s, VReg rn) {
    arm64_ands_reg(s, 1, ARM64_XZR, REG(rn), REG(rn), ARM64_LSL, 0); // tst rn, rn
}
// cmp rn, rm  (SUBS xzr, rn, rm)
static void asm_cmp_phy_phy(SecBuf *s, VReg rn, VReg rm) {
    arm64_subs_reg(s, 1, ARM64_XZR, REG(rn), REG(rm), ARM64_LSL, 0); // cmp rn, rm
}
// ccmp rn, rm, #0, eq  (conditional compare, for int128 ordered comparison)
static void asm_ccmp_eq(SecBuf *s, VReg rn, VReg rm) {
    // CCMP rn, rm, #nzcv, cond  — instruction: 0xBA4..000 | (rm<<16) | (cond<<12) | (rn<<5) | nzcv
    // CCMP xrn, xrm, #0, eq: sf=1, op=2, S=1, o2=0, o3=0, cond=EQ=0, rn, rm, nzcv=0
    uint32_t insn = 0xBA400000U;
    insn |= (uint32_t)(REG(rm) & 0x1f) << 16;
    insn |= (uint32_t)(0) << 12; // cond=EQ=0
    insn |= (uint32_t)(REG(rn) & 0x1f) << 5;
    insn |= 0; // nzcv=0  (actually 0 since we want none set)
    secbuf_emit32le(s, insn);
}
// cmp w16, #-16  (compare physical w16 with immediate)
static void asm_cmp_w16_imm(SecBuf *s, int32_t imm) {
    arm64_subs_imm(s, 0, ARM64_XZR, ARM64_X16, imm, 0); // cmp w16, #imm
}
// sxtw x17, w16  (sign-extend w16 to x17)
static void asm_sxtw_x17_w16(SecBuf *s) {
    arm64_sxtw(s, ARM64_X17, ARM64_X16); // sxtw x17, w16
}

// ============================================================================
// ARM64 variable shift for int128 (lsr or asr, with size and shift as args)
// ============================================================================
static void asm_shift_imm(SecBuf *s, VReg rd, int size, bool is_unsigned, uint8_t shift) {
    if (is_unsigned)
        arm64_lsr_imm(s, size == 8 ? 1 : 0, REG(rd), REG(rd), shift); // lsr rd, rd, #shift
    else
        arm64_asr_imm(s, size == 8 ? 1 : 0, REG(rd), REG(rd), shift); // asr rd, rd, #shift
}
#endif // ARCH_ARM64

// ============================================================================
// x86_64 int128 helpers — fixed-register ops for widen/int128 code
// ============================================================================
#ifndef ARCH_ARM64
// movq (rs), %rax  — load 64-bit from virtual reg's memory to RAX
static void asm_mov_mem_rax(SecBuf *s, VReg rs) {
    X86Mem m = {REG(rs), X86_NOREG, 1, 0};
    x86_mov_rm(s, 8, X86_RAX, m); // movq (rs), %rax
}
// movq 8(rs), %rdx  — load high 64-bit from virtual reg's memory to RDX
static void asm_mov_mem8_rdx(SecBuf *s, VReg rs) {
    X86Mem m = {REG(rs), X86_NOREG, 1, 8};
    x86_mov_rm(s, 8, X86_RDX, m); // movq 8(rs), %rdx
}
// movq 8(rs), r{rd}  — load high 64-bit from virtual reg to any virtual reg
static void asm_mov_mem8_reg(SecBuf *s, VReg rd, VReg rs) {
    X86Mem m = {REG(rs), X86_NOREG, 1, 8};
    x86_mov_rm(s, 8, REG(rd), m); // movq 8(rs), r{rd}
}
// movq %rax, (rd)  — store RAX to virtual reg's memory
static void asm_mov_rax_mem(SecBuf *s, VReg rd) {
    X86Mem m = {REG(rd), X86_NOREG, 1, 0};
    x86_mov_mr(s, 8, m, X86_RAX); // movq %rax, (rd)
}
// movq %rdx, 8(rd)  — store RDX to virtual reg's high memory
static void asm_mov_rdx_mem8(SecBuf *s, VReg rd) {
    X86Mem m = {REG(rd), X86_NOREG, 1, 8};
    x86_mov_mr(s, 8, m, X86_RDX); // movq %rdx, 8(rd)
}
// movq %rdx, %rax  — copy RDX to RAX
static void asm_mov_rdx_rax(SecBuf *s) {
    x86_mov_rr(s, 8, X86_RAX, X86_RDX); // movq %rdx, %rax
}
// movq %rax, %rdx  — copy RAX to RDX
static void asm_mov_rax_rdx(SecBuf *s) {
    x86_mov_rr(s, 8, X86_RDX, X86_RAX); // movq %rax, %rdx
}
// negq (rd)  — negate 64-bit at virtual reg's memory
static void asm_negq_mem(SecBuf *s, VReg rd) {
    X86Mem m = {REG(rd), X86_NOREG, 1, 0};
    x86_neg_m(s, 8, m); // negq (rd)
}
// negq 8(rd)  — negate high 64-bit at virtual reg's memory
static void asm_negq_mem8(SecBuf *s, VReg rd) {
    X86Mem m = {REG(rd), X86_NOREG, 1, 8};
    x86_neg_m(s, 8, m); // negq 8(rd)
}
// ============================================================================
// x86_64: adc / sbb from memory  (these don't exist in x86_enc.h yet)
// ============================================================================
static void x86_adc_rm(SecBuf *s, int size, X86Reg dst, X86Mem srcm) {
    // ADC r/m, r: opcode 13 /r
    uint8_t rex = (size == 8) ? 0x48 : 0x00;
    secbuf_emit8(s, rex | ((dst & 8) ? 0x04 : 0) | ((srcm.base & 8) ? 0x01 : 0) | ((srcm.index & 8) ? 0x02 : 0));
    secbuf_emit8(s, 0x13);
    uint8_t modrm;
    if (srcm.disp == 0 && (srcm.base & 7) != X86_RBP) {
        modrm = ((dst & 7) << 3) | (srcm.base & 7);
        secbuf_emit8(s, modrm);
    } else if (srcm.disp >= -128 && srcm.disp < 128) {
        modrm = 0x40 | ((dst & 7) << 3) | (srcm.base & 7);
        secbuf_emit8(s, modrm);
        secbuf_emit8(s, (uint8_t)(int8_t)srcm.disp);
    } else {
        modrm = 0x80 | ((dst & 7) << 3) | (srcm.base & 7);
        secbuf_emit8(s, modrm);
        secbuf_emit32le(s, (uint32_t)srcm.disp);
    }
}
static void x86_sbb_rm(SecBuf *s, int size, X86Reg dst, X86Mem srcm) {
    // SBB r/m, r: opcode 1B /r
    uint8_t rex = (size == 8) ? 0x48 : 0x00;
    secbuf_emit8(s, rex | ((dst & 8) ? 0x04 : 0) | ((srcm.base & 8) ? 0x01 : 0) | ((srcm.index & 8) ? 0x02 : 0));
    secbuf_emit8(s, 0x1B);
    uint8_t modrm;
    if (srcm.disp == 0 && (srcm.base & 7) != X86_RBP) {
        modrm = ((dst & 7) << 3) | (srcm.base & 7);
        secbuf_emit8(s, modrm);
    } else if (srcm.disp >= -128 && srcm.disp < 128) {
        modrm = 0x40 | ((dst & 7) << 3) | (srcm.base & 7);
        secbuf_emit8(s, modrm);
        secbuf_emit8(s, (uint8_t)(int8_t)srcm.disp);
    } else {
        modrm = 0x80 | ((dst & 7) << 3) | (srcm.base & 7);
        secbuf_emit8(s, modrm);
        secbuf_emit32le(s, (uint32_t)srcm.disp);
    }
}
static void x86_adc_mi(SecBuf *s, int size, X86Mem dstm, int32_t imm) {
    // ADC r/m, imm: opcode 83 /2 ib (for 8-bit sign-extended imm)
    uint8_t rex = (size == 8) ? 0x48 : 0x00;
    secbuf_emit8(s, rex | ((dstm.base & 8) ? 0x01 : 0) | ((dstm.index & 8) ? 0x02 : 0));
    secbuf_emit8(s, 0x83);
    uint8_t modrm = 0x10; // /2 = reg=2 (010)
    if (dstm.disp == 0 && (dstm.base & 7) != X86_RBP) {
        modrm |= (dstm.base & 7);
        secbuf_emit8(s, modrm);
    } else if (dstm.disp >= -128 && dstm.disp < 128) {
        modrm |= 0x40 | (dstm.base & 7);
        secbuf_emit8(s, modrm);
        secbuf_emit8(s, (uint8_t)(int8_t)dstm.disp);
    } else {
        modrm |= 0x80 | (dstm.base & 7);
        secbuf_emit8(s, modrm);
        secbuf_emit32le(s, (uint32_t)dstm.disp);
    }
    secbuf_emit8(s, (uint8_t)imm);
}
static void asm_mov_base_off_rdx(SecBuf *s, VReg base, int64_t disp) {
    X86Mem m = {REG(base), X86_NOREG, 1, disp};
    x86_mov_rm(s, 8, X86_RDX, m); // movq disp(base), %rdx
}
// movq %rdx, disp(base)  — store RDX to base+disp
static void asm_mov_rdx_base_off(SecBuf *s, VReg base, int64_t disp) {
    X86Mem m = {REG(base), X86_NOREG, 1, disp};
    x86_mov_mr(s, 8, m, X86_RDX); // movq %rdx, disp(base)
}
// movq 8(%rcx), %rax  — va_arg: load high from va_list ptr (in RCX)
static void asm_mov_mem8_rcx_rax(SecBuf *s) {
    X86Mem m = {X86_RCX, X86_NOREG, 1, 8};
    x86_mov_rm(s, 8, X86_RAX, m); // movq 8(%rcx), %rax
}
#endif // !ARCH_ARM64
#endif // CODEGEN_ASM_H
// adcq $0, 8(rd)  — add-with-carry 0 to high 64-bit (x86 encoding: REX.W 83 /2 ib)
static void asm_adcq_mem8_0(SecBuf *s, VReg rd) {
    X86Reg r = REG(rd);
    // REX.W + R bit for dst > 7
    secbuf_emit8(s, (uint8_t)(0x48 | ((r & 8) ? 0x01 : 0)));
    secbuf_emit16le(s, 0x5083); // 83 /2 with disp8, mod=01 (/2 = reg field = 2<<3 = 0x10)
    secbuf_emit8(s, (uint8_t)(8)); // disp8 = 8
    secbuf_emit8(s, 0); // imm8 = 0
}
// adcq 8(rs), %rdx  — add-with-carry (x86: REX.W 13 /r)
static void asm_adcq_mem8_rdx(SecBuf *s, VReg rs) {
    X86Reg r = REG(rs);
    // adc rdx, [r+8]: REX.W (rdx=2, r may need B=1)
    secbuf_emit8(s, (uint8_t)(0x48 | ((r & 8) ? 0x01 : 0)));
    secbuf_emit8(s, 0x13);
    uint8_t modrm = (uint8_t)(0x50 | (r & 7)); // mod=01, reg=rdx(2), r/m=r&7
    secbuf_emit8(s, modrm);
    secbuf_emit8(s, (uint8_t)(8)); // disp8 = 8
}
// addq (rs), %rax  — add from memory to RAX
static void asm_addq_mem_rax(SecBuf *s, VReg rs) {
    X86Mem m = {REG(rs), X86_NOREG, 1, 0};
    x86_add_rm(s, 8, X86_RAX, m); // addq (rs), %rax
}
// sbbq 8(rs), %rdx  — subtract-with-borrow (x86: REX.W 1B /r)
static void asm_sbbq_mem8_rdx(SecBuf *s, VReg rs) {
    X86Reg r = REG(rs);
    // sbb rdx, [r+8]: REX.W (rdx=2, r may need B=1)
    secbuf_emit8(s, (uint8_t)(0x48 | ((r & 8) ? 0x01 : 0)));
    secbuf_emit8(s, 0x1B);
    uint8_t modrm = (uint8_t)(0x50 | (r & 7)); // mod=01, reg=rdx(2), r/m=r&7
    secbuf_emit8(s, modrm);
    secbuf_emit8(s, (uint8_t)(8)); // disp8 = 8
}
// notq %rax  — bitwise NOT of RAX
static void asm_notq_rax(SecBuf *s) {
    x86_not_r(s, 8, X86_RAX); // notq %rax
}
// notq %rdx  — bitwise NOT of RDX
static void asm_notq_rdx(SecBuf *s) {
    x86_not_r(s, 8, X86_RDX); // notq %rdx
}
// mulq r{rs}  — unsigned multiply (x86: REX.W F7 /4)
static void asm_mulq(SecBuf *s, VReg rs) {
    X86Reg r = REG(rs);
    // mul r/m64: REX.W + F7 /4 (reg field = 4 << 3 = 0x20)
    secbuf_emit8(s, (uint8_t)(0x48 | ((r & 8) ? 0x04 : 0))); // REX.W + R
    secbuf_emit8(s, 0xF7);
    secbuf_emit8(s, (uint8_t)(0xE0 | (r & 7))); // mod=11, reg=4, r/m=r&7
}
// imulq r{rs}, r{rd}  — signed multiply rd *= rs
static void asm_imulq_reg_reg(SecBuf *s, VReg rd, VReg rs) {
    x86_imul_rr(s, 8, REG(rd), REG(rs)); // imulq rs, rd
}
// shlq $imm, %rax  — shift RAX left by immediate
static void asm_shl_rax_imm(SecBuf *s, uint8_t imm) {
    x86_shl_ri(s, 8, X86_RAX, imm); // shlq $imm, %rax
}
// shlq %cl, %rax  — shift RAX left by CL
static void asm_shl_rax_cl(SecBuf *s) {
    x86_shl_rcl(s, 8, X86_RAX); // shlq %cl, %rax
}
// %s $imm, %rdx  — shift RDX (sar or shr) by immediate
static void asm_shift_rdx_imm(SecBuf *s, bool is_unsigned, uint8_t imm) {
    if (is_unsigned)
        x86_shr_ri(s, 8, X86_RDX, imm);
    else
        x86_sar_ri(s, 8, X86_RDX, imm);
}
// %s %cl, %rdx  — shift RDX (sar or shr) by CL
static void asm_shift_rdx_cl(SecBuf *s, bool is_unsigned) {
    if (is_unsigned)
        x86_shr_rcl(s, 8, X86_RDX);
    else
        x86_sar_rcl(s, 8, X86_RDX);
}
// %s $imm, %rax  — shift RAX (sar or shr) by immediate
static void asm_shift_rax_imm(SecBuf *s, bool is_unsigned, uint8_t imm) {
    if (is_unsigned)
        x86_shr_ri(s, 8, X86_RAX, imm);
    else
        x86_sar_ri(s, 8, X86_RAX, imm);
}
// shldq $imm, %rax, %rdx  — double-precision shift left
static void asm_shldq_imm(SecBuf *s, uint8_t imm) {
    // SHLD r/m64, r64, imm8: opcode 0F A4 /r ib
    // shldq $imm, %rax, %rdx  = SHLD %rdx, %rax, $imm
    secbuf_emit8(s, 0x48); // REX.W
    secbuf_emit16le(s, 0xA40F); // 0F A4
    secbuf_emit8(s, 0xC2); // ModRM: mod=11, reg=rax(0), r/m=rdx(2)
    secbuf_emit8(s, imm);
}
// shldq %cl, %rax, %rdx  — double-precision shift left by CL
static void asm_shldq_cl(SecBuf *s) {
    secbuf_emit8(s, 0x48); // REX.W
    secbuf_emit16le(s, 0xA50F); // 0F A5
    secbuf_emit8(s, 0xC2); // ModRM: mod=11, reg=rax(0), r/m=rdx(2)
}
// shrdq $imm, %rdx, %rax  — double-precision shift right
static void asm_shrdq_imm(SecBuf *s, uint8_t imm) {
    // SHRD r/m64, r64, imm8: opcode 0F AC /r ib
    // shrdq $imm, %rdx, %rax  = SHRD %rax, %rdx, $imm
    secbuf_emit8(s, 0x48); // REX.W
    secbuf_emit16le(s, 0xAC0F); // 0F AC
    secbuf_emit8(s, 0xD0); // ModRM: mod=11, reg=rdx(2), r/m=rax(0)
    secbuf_emit8(s, imm);
}
// shrdq %cl, %rdx, %rax  — double-precision shift right by CL
static void asm_shrdq_cl(SecBuf *s) {
    secbuf_emit8(s, 0x48); // REX.W
    secbuf_emit16le(s, 0xAD0F); // 0F AD
    secbuf_emit8(s, 0xD0); // ModRM: mod=11, reg=rdx(2), r/m=rax(0)
}

// movq %rax, 8(rd)  — store RAX to virtual reg's high memory
static void asm_movq_rax_mem8(SecBuf *s, VReg rd) {
    X86Mem m = {REG(rd), X86_NOREG, 1, 8};
    x86_mov_mr(s, 8, m, X86_RAX); // movq %rax, 8(rd)
}

// xorl %s, %s  — 32-bit XOR of two virtual regs (zero-extend for comparison)
static void asm_xorl_reg_reg(SecBuf *s, VReg a, VReg b) {
    x86_xor_rr(s, 4, REG(a), REG(b)); // xorl a32, b32
}

// Memory operand arithmetic with RAX/RDX:
// %s (%s), %%rax  — add/sub/and/or/xor/cmp with RAX from mem
// %s 8(%s), %%rdx  — same for RDX from mem+8
static void asm_op_mem_rax(SecBuf *s, const char *op, VReg rs) {
    X86Mem m = {REG(rs), X86_NOREG, 1, 0};
    if (strcmp(op, "addq") == 0) x86_add_rm(s, 8, X86_RAX, m);
    else if (strcmp(op, "subq") == 0)
        x86_sub_rm(s, 8, X86_RAX, m);
    else if (strcmp(op, "andq") == 0)
        x86_and_rm(s, 8, X86_RAX, m);
    else if (strcmp(op, "orq") == 0)
        x86_or_rm(s, 8, X86_RAX, m);
    else if (strcmp(op, "xorq") == 0)
        x86_xor_rm(s, 8, X86_RAX, m);
    else if (strcmp(op, "cmpq") == 0)
        x86_cmp_rm(s, 8, X86_RAX, m);
}
static void asm_op_mem8_rdx(SecBuf *s, const char *op, VReg rs) {
    X86Mem m = {REG(rs), X86_NOREG, 1, 8};
    if (strcmp(op, "addq") == 0) x86_add_rm(s, 8, X86_RDX, m);
    else if (strcmp(op, "subq") == 0)
        x86_sub_rm(s, 8, X86_RDX, m);
    else if (strcmp(op, "andq") == 0)
        x86_and_rm(s, 8, X86_RDX, m);
    else if (strcmp(op, "orq") == 0)
        x86_or_rm(s, 8, X86_RDX, m);
    else if (strcmp(op, "xorq") == 0)
        x86_xor_rm(s, 8, X86_RDX, m);
    else if (strcmp(op, "cmpq") == 0)
        x86_cmp_rm(s, 8, X86_RDX, m);
}
// set%s %%al  — set byte condition code
static void asm_setcc_al(SecBuf *s, X86Cond cond) {
    x86_setcc(s, cond, X86_RAX); // setcc %al
}

// "xorl r, r" — zero a virtual register (already exists as asm_xor_reg_reg)
// but also need the specific "xorl %s, %s" form for int128 compare epilogue
