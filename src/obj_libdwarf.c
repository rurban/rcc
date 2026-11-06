// SPDX-License-Identifier: LGPL-2.1-or-later
// -g DWARF generation through libdwarf's producer library (libdwarfp).
// Compiled only when the Makefile found libdwarfp (HAVE_LIBDWARF); it fills the
// same ObjFile debug SecBufs and relocation-offset fields as the built-in
// emitter in obj.c, which is used as the fallback if this returns false.
#include "obj.h"
#include <libdwarf.h>
#include <libdwarfp.h>
#include <dwarf.h>
#include <stdio.h>
#include <string.h>

// Symbolic-relocation symbol index standing for the .text section symbol. Must
// not collide with the section indices the callback below hands out (1..N).
#define TEXT_SYM 1000

#define MAX_SECS 32
#define MAX_OUT 4096 // section chunks

// Section names libdwarfp asked us to create; the callback's return value
// (index+1) is the "elf section index" it later reports bytes/relocs under.
typedef struct {
    char names[MAX_SECS][32];
    int count;
} SecNames;

static int sect_cb(const char *name, int size, Dwarf_Unsigned type, Dwarf_Unsigned flags,
                   Dwarf_Unsigned link, Dwarf_Unsigned info, Dwarf_Unsigned *sect_name_index,
                   void *user_data, int *error) {
    (void)size;
    (void)type;
    (void)flags;
    (void)link;
    (void)info;
    SecNames *sn = user_data;
    if (sn->count >= MAX_SECS) {
        *error = DW_DLE_ELF_SECT_ERR;
        return -1;
    }
    snprintf(sn->names[sn->count], sizeof sn->names[0], "%s", name);
    *sect_name_index = 0;
    return ++sn->count;
}

typedef struct {
    SecBuf *dst;
    const void *bytes;
    size_t len;
} Out;

static SecBuf *debug_sec_by_name(ObjFile *obj, const char *name) {
    if (!strcmp(name, ".debug_info")) return &obj->debug_info_section;
    if (!strcmp(name, ".debug_abbrev")) return &obj->debug_abbrev_section;
    if (!strcmp(name, ".debug_line")) return &obj->debug_line_section;
    if (!strcmp(name, ".debug_aranges")) return &obj->debug_aranges_section;
    return NULL;
}

static const char *sec_name(const SecNames *sn, Dwarf_Unsigned idx) {
    return idx >= 1 && idx <= (Dwarf_Unsigned)sn->count ? sn->names[idx - 1] : "";
}

static bool fail(const char *what, Dwarf_Error err) {
    fprintf(stderr, "rcc: libdwarf: %s failed: %s; using built-in DWARF emitter\n", what,
            err ? dwarf_errmsg(err) : "(no error)");
    return false;
}

#define CHECK(call)                                                                                \
    do {                                                                                           \
        if ((call) != DW_DLV_OK) {                                                                 \
            ok = fail(#call, err);                                                                 \
            goto done;                                                                             \
        }                                                                                          \
    } while (0)

bool objfile_flush_debug_libdwarf(ObjFile *obj, uint64_t text_end) {
    SecNames sn = {.count = 0};
    Dwarf_P_Debug dbg = NULL;
    Dwarf_Error err = NULL;
    bool ok = false;
    Out out[MAX_OUT];
    int nout = 0;
    // Where the single address relocation of each section sits (SIZE_MAX: none).
    size_t line_off = SIZE_MAX, info_off = SIZE_MAX, aranges_off = SIZE_MAX;

    CHECK(dwarf_producer_init(DW_DLC_POINTER64 | DW_DLC_TARGET_LITTLEENDIAN |
                                  DW_DLC_SYMBOLIC_RELOCATIONS,
                              sect_cb, NULL, NULL, &sn, "x86_64", "V4", NULL, &dbg, &err));

    // --- .debug_line ---
    for (int i = 0; i < obj->debug_file_count; i++) {
        Dwarf_Unsigned idx = 0;
        char *fname = obj->debug_files[i] ? obj->debug_files[i] : "<unknown>";
        CHECK(dwarf_add_file_decl_a(dbg, fname, 0, 0, 0, &idx, &err));
        // objfile_add_debug_file() hands out 1-based indices in declaration order.
        if (idx != (Dwarf_Unsigned)i + 1) {
            ok = fail("file index mismatch", NULL);
            goto done;
        }
    }
    CHECK(dwarf_lne_set_address_a(dbg, 0, TEXT_SYM, &err));
    for (int i = 0; i < obj->debug_line_count; i++) {
        const DebugLineEntry *e = &obj->debug_lines[i];
        CHECK(dwarf_add_line_entry_c(dbg, e->file_idx, e->text_offset, e->line, 0, true, false,
                                     false, false, 0, 0, &err));
    }
    CHECK(dwarf_lne_end_sequence_a(dbg, text_end, &err));

    // --- .debug_info / .debug_abbrev: one DW_TAG_compile_unit ---
    // (DW_AT_stmt_list is added by libdwarfp since line entries exist.)
    Dwarf_P_Die cu = NULL;
    Dwarf_P_Attribute attr = NULL;
    CHECK(dwarf_new_die_a(dbg, DW_TAG_compile_unit, NULL, NULL, NULL, NULL, &cu, &err));
    CHECK(dwarf_add_AT_producer_a(cu, "rcc", &attr, &err));
    CHECK(dwarf_add_AT_unsigned_const_a(dbg, cu, DW_AT_language, DW_LANG_C11, &attr, &err));
    CHECK(dwarf_add_AT_name_a(cu, obj->debug_files[0] ? obj->debug_files[0] : "<unknown>", &attr,
                              &err));
    CHECK(dwarf_add_AT_targ_address_c(dbg, cu, DW_AT_low_pc, 0, TEXT_SYM, &attr, &err));
    CHECK(dwarf_add_AT_unsigned_const_a(dbg, cu, DW_AT_high_pc, text_end, &attr, &err));
    CHECK(dwarf_add_die_to_debug_a(dbg, cu, &err));

    // --- .debug_aranges ---
    CHECK(dwarf_add_arange_c(dbg, 0, text_end, TEXT_SYM, 0, 0, &err));

    Dwarf_Unsigned nbufs = 0;
    CHECK(dwarf_transform_to_disk_form_a(dbg, &nbufs, &err));

    // Collect section bytes, validating before the first write to obj.
    for (;;) {
        Dwarf_Unsigned idx = 0, len = 0;
        Dwarf_Ptr bytes = NULL;
        int r = dwarf_get_section_bytes_a(dbg, 0, &idx, &len, &bytes, &err);
        if (r == DW_DLV_NO_ENTRY) break;
        if (r != DW_DLV_OK) {
            ok = fail("dwarf_get_section_bytes_a", err);
            goto done;
        }
        if (len == 0) continue;
        SecBuf *dst = debug_sec_by_name(obj, sec_name(&sn, idx));
        if (!dst || nout >= MAX_OUT) {
            ok = fail("unexpected DWARF section", NULL);
            goto done;
        }
        // A large section arrives in several chunks (appended in order).
        out[nout++] = (Out){dst, bytes, (size_t)len};
    }

    // Address relocations against .text. Section-relative references (e.g.
    // DW_AT_stmt_list) stay 0 in this single-CU object, as in the built-in emitter.
    Dwarf_Unsigned nrelsecs = 0;
    int drd_version = 0;
    CHECK(dwarf_get_relocation_info_count(dbg, &nrelsecs, &drd_version, &err));
    for (Dwarf_Unsigned s = 0; s < nrelsecs; s++) {
        Dwarf_Unsigned relsec = 0, applies_to = 0, n = 0;
        Dwarf_Relocation_Data rd = NULL;
        CHECK(dwarf_get_relocation_info(dbg, &relsec, &applies_to, &n, &rd, &err));
        const char *target = sec_name(&sn, applies_to);
        size_t *slot = !strcmp(target, ".debug_line") ? &line_off
            : !strcmp(target, ".debug_info")          ? &info_off
            : !strcmp(target, ".debug_aranges")       ? &aranges_off
                                                      : NULL;
        for (Dwarf_Unsigned k = 0; k < n; k++) {
            if (rd[k].drd_symbol_index != TEXT_SYM) continue;
            // drd_length is not trusted: libdwarfp reports 4 for the 8-byte
            // DW_LNE_set_address operand; writers emit R_*_64 at the offset.
            if (!slot || *slot != SIZE_MAX || rd[k].drd_type != dwarf_drt_data_reloc) {
                ok = fail("unsupported .text relocation layout", NULL);
                goto done;
            }
            *slot = (size_t)rd[k].drd_offset;
        }
    }
    if (line_off == SIZE_MAX || info_off == SIZE_MAX || aranges_off == SIZE_MAX) {
        ok = fail("missing .text relocation", NULL);
        goto done;
    }
    for (int i = 0; i < nout; i++) {
        if (out[i].dst == &obj->debug_abbrev_section) continue; // no relocation
        bool later_chunk = false; // the reloc offsets lie in a section's first chunk
        for (int j = 0; j < i; j++)
            if (out[j].dst == out[i].dst) later_chunk = true;
        if (later_chunk) continue;
        size_t off = out[i].dst == &obj->debug_line_section ? line_off
            : out[i].dst == &obj->debug_info_section        ? info_off
                                                            : aranges_off;
        if (off > out[i].len || out[i].len - off < 8) {
            ok = fail("relocation offset outside section", NULL);
            goto done;
        }
    }

    for (int i = 0; i < nout; i++)
        secbuf_emitbuf(out[i].dst, out[i].bytes, out[i].len);
    obj->debug_line_addr_off = line_off;
    obj->debug_has_line_addr = true;
    obj->debug_low_pc_offset = info_off;
    obj->debug_has_low_pc = true;
    obj->debug_aranges_addr_off = aranges_off;
    obj->debug_has_aranges_addr = true;
    ok = true;

done:
    if (dbg) {
        Dwarf_Error ferr = NULL;
        dwarf_producer_finish_a(dbg, &ferr);
    }
    return ok;
}
