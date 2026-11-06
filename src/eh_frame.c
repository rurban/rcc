// SPDX-License-Identifier: LGPL-2.1-or-later
// .eh_frame call-frame information for -g builds (ELF x86-64).
//
// codegen records, per function, the handful of events at which the frame
// layout changes (see CfiOp in obj.h): rcc's AST backend always uses
// `push %rbp; mov %rsp,%rbp` and addresses everything off rbp, so one CIE
// plus one small FDE per function describes every instruction boundary that
// matters to a debugger or an unwinder (backtrace(), _Unwind_*, pthread_exit).
#include "obj.h"

#ifdef RCC_CFI
#include <string.h>

#define DW_CFA_advance_loc 0x40
#define DW_CFA_offset 0x80
#define DW_CFA_restore 0xc0
#define DW_CFA_advance_loc1 0x02
#define DW_CFA_advance_loc2 0x03
#define DW_CFA_advance_loc4 0x04
#define DW_CFA_def_cfa 0x0c
#define DW_CFA_def_cfa_register 0x0d
#define DW_CFA_def_cfa_offset 0x0e
#define DW_CFA_remember_state 0x0a
#define DW_CFA_restore_state 0x0b
#define DW_CFA_nop 0x00

#define DW_EH_PE_pcrel_sdata4 0x1b

#define DW_REG_RSP 7
#define DW_REG_RBP 6
#define DW_REG_RA 16

static void put_uleb(SecBuf *b, uint64_t v) {
    do {
        uint8_t byte = v & 0x7f;
        v >>= 7;
        secbuf_emit8(b, v ? byte | 0x80 : byte);
    } while (v);
}

static void pad_to_8(SecBuf *b, size_t from) {
    while ((b->len - from) % 8)
        secbuf_emit8(b, DW_CFA_nop);
}

// Patch the 4-byte length prefix at `at` to cover everything written since.
static void patch_len(SecBuf *b, size_t at) {
    uint32_t len = (uint32_t)(b->len - at - 4);
    memcpy(b->data + at, &len, 4);
}

static void advance(SecBuf *b, uint32_t delta) {
    if (delta < 0x40) {
        secbuf_emit8(b, DW_CFA_advance_loc | delta);
    } else if (delta < 0x100) {
        secbuf_emit8(b, DW_CFA_advance_loc1);
        secbuf_emit8(b, (uint8_t)delta);
    } else if (delta < 0x10000) {
        secbuf_emit8(b, DW_CFA_advance_loc2);
        secbuf_emit16le(b, (uint16_t)delta);
    } else {
        secbuf_emit8(b, DW_CFA_advance_loc4);
        secbuf_emit32le(b, delta);
    }
}

static void emit_op(SecBuf *b, const CfiOp *op) {
    switch (op->kind) {
    case CFI_PUSH_RBP:
        secbuf_emit8(b, DW_CFA_def_cfa_offset);
        put_uleb(b, 16);
        secbuf_emit8(b, DW_CFA_offset | DW_REG_RBP);
        put_uleb(b, 2); // CFA-16, data alignment -8
        break;
    case CFI_SET_FP:
        secbuf_emit8(b, DW_CFA_def_cfa_register);
        put_uleb(b, DW_REG_RBP);
        break;
    case CFI_SAVE: // saved at rbp-val == CFA-16-val
        secbuf_emit8(b, DW_CFA_offset | op->reg);
        put_uleb(b, (uint64_t)(16 + op->val) / 8);
        break;
    case CFI_TEARDOWN: // rbp already popped: only the return address is on the stack
        secbuf_emit8(b, DW_CFA_remember_state);
        secbuf_emit8(b, DW_CFA_def_cfa);
        put_uleb(b, DW_REG_RSP);
        put_uleb(b, 8);
        secbuf_emit8(b, DW_CFA_restore | DW_REG_RBP);
        break;
    case CFI_RESTORE:
        secbuf_emit8(b, DW_CFA_restore_state);
        break;
    case CFI_ENTRY: // e.g. the head of a cold block that sets up its own frame
        secbuf_emit8(b, DW_CFA_def_cfa);
        put_uleb(b, DW_REG_RSP);
        put_uleb(b, 8);
        secbuf_emit8(b, DW_CFA_restore | DW_REG_RBP);
        break;
    }
}

void objfile_flush_eh_frame(ObjFile *obj) {
    if (!obj->cfi_fn_count) return;
    int sec = objfile_find_or_add_section(obj, ".eh_frame", 2 /* SHF_ALLOC */, 0);
    objfile_section_align(obj, sec, 8);
    SecBuf *b = objfile_section_buf(obj, sec);
    int text_sym = objfile_add_sym(obj, ".text", SEC_TEXT, 0, 0, SB_LOCAL, ST_SECTION);

    // CIE: "zR", code align 1, data align -8, return address column 16 (rip),
    // FDE pointers pc-relative signed 32-bit. Initial state at function entry:
    // CFA = rsp+8, return address at CFA-8.
    size_t cie = b->len;
    secbuf_emit32le(b, 0); // length, patched below
    secbuf_emit32le(b, 0); // CIE id
    secbuf_emit8(b, 1); // version
    secbuf_emit8(b, 'z');
    secbuf_emit8(b, 'R');
    secbuf_emit8(b, 0);
    put_uleb(b, 1); // code alignment factor
    secbuf_emit8(b, 0x78); // data alignment factor: sleb128(-8)
    put_uleb(b, DW_REG_RA);
    put_uleb(b, 1); // augmentation data length
    secbuf_emit8(b, DW_EH_PE_pcrel_sdata4);
    secbuf_emit8(b, DW_CFA_def_cfa);
    put_uleb(b, DW_REG_RSP);
    put_uleb(b, 8);
    secbuf_emit8(b, DW_CFA_offset | DW_REG_RA);
    put_uleb(b, 1);
    pad_to_8(b, cie);
    patch_len(b, cie);

    for (int i = 0; i < obj->cfi_fn_count; i++) {
        const CfiFn *f = &obj->cfi_fns[i];
        size_t fde = b->len;
        secbuf_emit32le(b, 0); // length, patched below
        secbuf_emit32le(b, (uint32_t)(b->len - cie)); // CIE pointer: back to the CIE start
        size_t pc = b->len;
        secbuf_emit32le(b, 0); // pc_begin: pcrel to the function, via relocation
        objfile_add_reloc(obj, sec, pc, text_sym, R_X86_64_PC32, (int64_t)f->start);
        secbuf_emit32le(b, (uint32_t)f->size); // pc_range
        put_uleb(b, 0); // augmentation data length
        uint32_t loc = 0; // offset the last emitted advance reached
        for (int k = 0; k < f->nops; k++) {
            const CfiOp *op = &f->ops[k];
            uint32_t at = op->off - (uint32_t)f->start;
            if (at > loc) {
                advance(b, at - loc);
                loc = at;
            }
            emit_op(b, op);
        }
        pad_to_8(b, fde);
        patch_len(b, fde);
    }
    // No zero terminator: the linker merges every object's .eh_frame and the
    // C runtime's crtend.o supplies the final one.
}
#endif // RCC_CFI
