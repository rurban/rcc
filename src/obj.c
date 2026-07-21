// SPDX-License-Identifier: LGPL-2.1-or-later
// Section buffer and object file helpers.
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include "obj.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#ifdef _WIN32
#include <direct.h>
#endif

// ---------------------------------------------------------------------------
// SecBuf
// ---------------------------------------------------------------------------

void secbuf_init(SecBuf *s) {
    s->data = NULL;
    s->len = 0;
    s->cap = 0;
}

void secbuf_free(SecBuf *s) {
    free(s->data);
    secbuf_init(s);
}

void secbuf_reserve(SecBuf *s, size_t extra) {
    if (s->len + extra <= s->cap) return;
    size_t newcap = s->cap ? s->cap * 2 : 256;
    while (newcap < s->len + extra) newcap *= 2;
    s->data = realloc(s->data, newcap);
    if (!s->data) {
        fprintf(stderr, "secbuf: out of memory\n");
        exit(1);
    }
    s->cap = newcap;
}

size_t secbuf_emit8(SecBuf *s, uint8_t v) {
    secbuf_reserve(s, 1);
    size_t off = s->len;
    s->data[s->len++] = v;
    return off;
}

size_t secbuf_emit16le(SecBuf *s, uint16_t v) {
    secbuf_reserve(s, 2);
    size_t off = s->len;
    s->data[s->len++] = (uint8_t)(v);
    s->data[s->len++] = (uint8_t)(v >> 8);
    return off;
}

size_t secbuf_emit32le(SecBuf *s, uint32_t v) {
    secbuf_reserve(s, 4);
    size_t off = s->len;
    s->data[s->len++] = (uint8_t)(v);
    s->data[s->len++] = (uint8_t)(v >> 8);
    s->data[s->len++] = (uint8_t)(v >> 16);
    s->data[s->len++] = (uint8_t)(v >> 24);
    return off;
}

size_t secbuf_emit64le(SecBuf *s, uint64_t v) {
    secbuf_reserve(s, 8);
    size_t off = s->len;
    for (int i = 0; i < 8; i++, v >>= 8)
        s->data[s->len++] = (uint8_t)v;
    return off;
}

void secbuf_emitbuf(SecBuf *s, const void *buf, size_t n) {
    secbuf_reserve(s, n);
    memcpy(s->data + s->len, buf, n);
    s->len += n;
}

void secbuf_align(SecBuf *s, int align) {
    if (align <= 1) return;
    size_t rem = s->len % (size_t)align;
    if (!rem) return;
    size_t pad = (size_t)align - rem;
    secbuf_reserve(s, pad);
    memset(s->data + s->len, 0, pad);
    s->len += pad;
}

void secbuf_patch32le(SecBuf *s, size_t off, uint32_t v) {
    assert(off + 4 <= s->len);
    s->data[off] = (uint8_t)(v);
    s->data[off + 1] = (uint8_t)(v >> 8);
    s->data[off + 2] = (uint8_t)(v >> 16);
    s->data[off + 3] = (uint8_t)(v >> 24);
}

void secbuf_patch64le(SecBuf *s, size_t off, uint64_t v) {
    assert(off + 8 <= s->len);
    for (int i = 0; i < 8; i++, v >>= 8)
        s->data[off + i] = (uint8_t)v;
}

// ---------------------------------------------------------------------------
// ObjFile
// ---------------------------------------------------------------------------

#define SYM_HT_INIT 4096 // power of two

static uint32_t sym_hash(const char *name) {
    uint32_t h = 2166136261u;
    for (const char *s = name; *s; s++) {
        h ^= (uint8_t)*s;
        h *= 16777619;
    }
    return h;
}

// Double the bucket array once the table averages >2 symbols per bucket.
// Nodes carry their hash, so this never re-reads a symbol name.
static void sym_htab_grow(ObjFile *obj) {
    uint32_t new_size = obj->sym_htab_size * 2;
    struct SymHashNode **nt = calloc(new_size, sizeof(*nt));
    if (!nt)
        return; // keep the smaller table; correctness does not depend on size
    for (uint32_t i = 0; i < obj->sym_htab_size; i++) {
        struct SymHashNode *n = obj->sym_htab[i];
        while (n) {
            struct SymHashNode *next = n->next;
            uint32_t b = n->hash & (new_size - 1);
            n->next = nt[b];
            nt[b] = n;
            n = next;
        }
    }
    free(obj->sym_htab);
    obj->sym_htab = nt;
    obj->sym_htab_size = new_size;
}

void objfile_init(ObjFile *obj) {
    // wrong -Wstringop-overflow= warning. known gcc bug
    memset(obj, 0, sizeof(ObjFile));
    obj->sym_htab_size = SYM_HT_INIT;
    obj->sym_htab = calloc(obj->sym_htab_size, sizeof(*obj->sym_htab));
    if (!obj->sym_htab) {
        fprintf(stderr, "objfile: out of memory\n");
        exit(1);
    }
    secbuf_init(&obj->text);
    secbuf_init(&obj->data);
    secbuf_init(&obj->rodata);
    secbuf_init(&obj->init_array);
    secbuf_init(&obj->fini_array);
    secbuf_init(&obj->data_tls);
    secbuf_init(&obj->thread_vars);
    secbuf_init(&obj->debug_line_section);
    secbuf_init(&obj->debug_info_section);
    secbuf_init(&obj->debug_abbrev_section);
    secbuf_init(&obj->debug_aranges_section);
}

void objfile_free(ObjFile *obj) {
    secbuf_free(&obj->text);
    secbuf_free(&obj->data);
    secbuf_free(&obj->rodata);
    secbuf_free(&obj->init_array);
    secbuf_free(&obj->fini_array);
    secbuf_free(&obj->data_tls);
    secbuf_free(&obj->thread_vars);
    secbuf_free(&obj->debug_line_section);
    secbuf_free(&obj->debug_info_section);
    secbuf_free(&obj->debug_abbrev_section);
    secbuf_free(&obj->debug_aranges_section);
    free(obj->debug_lines);
    for (int i = 0; i < obj->sym_count; i++)
        free(obj->syms[i].name);
    free(obj->syms);
    free(obj->text_relocs);
    free(obj->data_relocs);
    free(obj->rodata_relocs);
    free(obj->init_array_relocs);
    free(obj->fini_array_relocs);
    free(obj->data_tls_relocs);
    free(obj->thread_vars_relocs);
    for (int i = 0; i < obj->extra_sec_count; i++) {
        free(obj->extra_secs[i].name);
        secbuf_free(&obj->extra_secs[i].buf);
        free(obj->extra_secs[i].relocs);
    }
    free(obj->extra_secs);
    free(obj->unwind);
    for (uint32_t i = 0; i < obj->sym_htab_size; i++) {
        struct SymHashNode *n = obj->sym_htab[i];
        while (n) {
            struct SymHashNode *next = n->next;
            free(n);
            n = next;
        }
    }
    free(obj->sym_htab);
    memset(obj, 0, sizeof(*obj));
}

UnwindEntry *objfile_add_unwind(ObjFile *obj) {
    if (obj->unwind_count == obj->unwind_cap) {
        obj->unwind_cap = obj->unwind_cap ? obj->unwind_cap * 2 : 16;
        UnwindEntry *tmp = realloc(obj->unwind, (size_t)obj->unwind_cap * sizeof(UnwindEntry));
        if (!tmp) {
            fprintf(stderr, "objfile: out of memory\n");
            exit(1);
        }
        obj->unwind = tmp;
    }
    UnwindEntry *e = &obj->unwind[obj->unwind_count++];
    memset(e, 0, sizeof(*e));
    return e;
}

int objfile_add_sym(ObjFile *obj, const char *name, int section,
                    uint64_t offset, uint64_t size, SymBind bind, SymType type) {
    // Fast path: hash table lookup
    uint32_t h = sym_hash(name);
    for (struct SymHashNode *n = obj->sym_htab[h & (obj->sym_htab_size - 1)]; n; n = n->next) {
        if (n->hash == h && strcmp(obj->syms[n->sym_idx].name, name) == 0) {
            // Update to defined version if previously undefined
            if (obj->syms[n->sym_idx].section == SEC_UNDEF && section != SEC_UNDEF) {
                obj->syms[n->sym_idx].section = section;
                obj->syms[n->sym_idx].offset = offset;
                obj->syms[n->sym_idx].size = size;
                obj->syms[n->sym_idx].bind = bind;
                obj->syms[n->sym_idx].type = type;
            }
            return n->sym_idx;
        }
    }
    if (obj->sym_count == obj->sym_cap) {
        obj->sym_cap = obj->sym_cap ? obj->sym_cap * 2 : 16;
        obj->syms = realloc(obj->syms, obj->sym_cap * sizeof(ObjSym));
        if (!obj->syms) {
            fprintf(stderr, "objfile: out of memory\n");
            exit(1);
        }
    }
    int idx = obj->sym_count++;
    obj->syms[idx].name = strdup(name);
    obj->syms[idx].section = section;
    obj->syms[idx].offset = offset;
    obj->syms[idx].size = size;
    obj->syms[idx].bind = bind;
    obj->syms[idx].type = type;
    // Insert into hash table
    struct SymHashNode *node = malloc(sizeof(struct SymHashNode));
    node->hash = h;
    node->sym_idx = idx;
    uint32_t b = h & (obj->sym_htab_size - 1);
    node->next = obj->sym_htab[b];
    obj->sym_htab[b] = node;
    if ((uint32_t)obj->sym_count > obj->sym_htab_size * 2)
        sym_htab_grow(obj);
    return idx;
}

int objfile_find_sym(ObjFile *obj, const char *name) {
    uint32_t h = sym_hash(name);
    for (struct SymHashNode *n = obj->sym_htab[h & (obj->sym_htab_size - 1)]; n; n = n->next)
        if (n->hash == h && strcmp(obj->syms[n->sym_idx].name, name) == 0)
            return n->sym_idx;
    return -1;
}

void objfile_add_reloc(ObjFile *obj, int section, uint64_t offset,
                       int sym_idx, uint32_t type, int64_t addend) {
    ObjReloc **relocs;
    int *count, *cap;
    switch (section) {
    case SEC_TEXT:
        relocs = &obj->text_relocs;
        count = &obj->text_reloc_count;
        cap = &obj->text_reloc_cap;
        break;
    case SEC_DATA:
        relocs = &obj->data_relocs;
        count = &obj->data_reloc_count;
        cap = &obj->data_reloc_cap;
        break;
    case SEC_RODATA:
        relocs = &obj->rodata_relocs;
        count = &obj->rodata_reloc_count;
        cap = &obj->rodata_reloc_cap;
        break;
    case SEC_INIT_ARRAY:
        relocs = &obj->init_array_relocs;
        count = &obj->init_array_reloc_count;
        cap = &obj->init_array_reloc_cap;
        break;
    case SEC_FINI_ARRAY:
        relocs = &obj->fini_array_relocs;
        count = &obj->fini_array_reloc_count;
        cap = &obj->fini_array_reloc_cap;
        break;
    case SEC_TDATA:
        relocs = &obj->data_tls_relocs;
        count = &obj->data_tls_reloc_count;
        cap = &obj->data_tls_reloc_cap;
        break;
    case SEC_THREAD_VARS:
        relocs = &obj->thread_vars_relocs;
        count = &obj->thread_vars_reloc_count;
        cap = &obj->thread_vars_reloc_cap;
        break;
    default:
        if (section < SEC_NUM || section - SEC_NUM >= obj->extra_sec_count) return;
        {
            ExtraSection *es = &obj->extra_secs[section - SEC_NUM];
            relocs = &es->relocs;
            count = &es->reloc_count;
            cap = &es->reloc_cap;
        }
        break;
    }
    if (*count == *cap) {
        *cap = *cap ? *cap * 2 : 8;
        ObjReloc *tmp = realloc(*relocs, (size_t)*cap * sizeof(ObjReloc));
        if (!tmp) {
            fprintf(stderr, "objfile: out of memory\n");
            exit(1);
        }
        *relocs = tmp;
    }
    ObjReloc *r = &(*relocs)[(*count)++];
    r->offset = offset;
    r->sym_idx = sym_idx;
    r->type = type;
    r->addend = addend;
}

int objfile_find_or_add_section(ObjFile *obj, const char *name,
                                uint32_t sh_flags, uint32_t sh_entsize) {
    for (int i = 0; i < obj->extra_sec_count; i++)
        if (!strcmp(obj->extra_secs[i].name, name))
            return SEC_NUM + i;
    if (obj->extra_sec_count == obj->extra_sec_cap) {
        obj->extra_sec_cap = obj->extra_sec_cap ? obj->extra_sec_cap * 2 : 4;
        ExtraSection *tmp = realloc(obj->extra_secs,
                                    (size_t)obj->extra_sec_cap * sizeof(ExtraSection));
        if (!tmp) {
            fprintf(stderr, "objfile: out of memory\n");
            exit(1);
        }
        obj->extra_secs = tmp;
    }
    ExtraSection *es = &obj->extra_secs[obj->extra_sec_count++];
    memset(es, 0, sizeof(*es));
    es->name = strdup(name);
    secbuf_init(&es->buf);
    es->sh_flags = sh_flags;
    es->sh_entsize = sh_entsize;
    return SEC_NUM + (obj->extra_sec_count - 1);
}

SecBuf *objfile_section_buf(ObjFile *obj, int section) {
    switch (section) {
    case SEC_TEXT: return &obj->text;
    case SEC_DATA: return &obj->data;
    case SEC_RODATA: return &obj->rodata;
    case SEC_INIT_ARRAY: return &obj->init_array;
    case SEC_FINI_ARRAY: return &obj->fini_array;
    case SEC_TDATA: return &obj->data_tls;
    case SEC_THREAD_VARS: return &obj->thread_vars;
    case SEC_BSS:
    case SEC_UNDEF:
        return NULL;
    default:
        if (section < SEC_NUM || section - SEC_NUM >= obj->extra_sec_count) return NULL;
        return &obj->extra_secs[section - SEC_NUM].buf;
    }
}

// ---------------------------------------------------------------------------
// Debug info helpers (for -g)
// ---------------------------------------------------------------------------
int objfile_add_debug_file(ObjFile *obj, char *filename) {
    if (!filename) return 1;
    for (int i = 0; i < obj->debug_file_count; i++)
        if (obj->debug_files[i] == filename)
            return i + 1;
    if (obj->debug_file_count >= MAX_DEBUG_FILES)
        return 1;
    int idx = ++obj->debug_file_count;
    obj->debug_files[idx - 1] = filename;
    return idx;
}

void objfile_add_debug_line(ObjFile *obj, uint64_t text_offset, int file_idx, int line) {
    if (obj->debug_line_count == obj->debug_line_cap) {
        obj->debug_line_cap = obj->debug_line_cap ? obj->debug_line_cap * 2 : 256;
        DebugLineEntry *tmp = realloc(obj->debug_lines,
                                      (size_t)obj->debug_line_cap * sizeof(DebugLineEntry));
        if (!tmp) {
            fprintf(stderr, "objfile: out of memory\n");
            exit(1);
        }
        obj->debug_lines = tmp;
    }
    DebugLineEntry *e = &obj->debug_lines[obj->debug_line_count++];
    e->text_offset = text_offset;
    e->file_idx = (uint32_t)file_idx;
    e->line = (uint32_t)line;
}

bool objfile_has_debug(ObjFile *obj) {
    return obj->debug_file_count > 0;
}

// Write a ULEB128 value to a SecBuf
static void secbuf_emit_uleb128(SecBuf *s, uint64_t value) {
    do {
        uint8_t byte = (uint8_t)(value & 0x7f);
        value >>= 7;
        if (value != 0) byte |= 0x80;
        secbuf_emit8(s, byte);
    } while (value != 0);
}

// Write a SLEB128 value to a SecBuf
static void secbuf_emit_sleb128(SecBuf *s, int64_t value) {
    bool more = true;
    while (more) {
        uint8_t byte = (uint8_t)(value & 0x7f);
        value >>= 7;
        if ((value == 0 && (byte & 0x40) == 0) ||
            (value == -1 && (byte & 0x40) != 0))
            more = false;
        else
            byte |= 0x80;
        secbuf_emit8(s, byte);
    }
}

// Serialize the collected debug line info into a DWARF4 .debug_line section.
// Also builds minimal .debug_info and .debug_abbrev sections.
void objfile_flush_debug_line(ObjFile *obj, uint64_t text_end) {
    SecBuf *dl = &obj->debug_line_section;
    SecBuf *di = &obj->debug_info_section;
    SecBuf *da = &obj->debug_abbrev_section;

    if (obj->debug_file_count == 0) return;

    // --- .debug_abbrev ---
    // Abbreviation 1: DW_TAG_compile_unit
    secbuf_emit_uleb128(da, 1); // abbreviation code
    secbuf_emit_uleb128(da, 0x11); // DW_TAG_compile_unit
    secbuf_emit8(da, 0); // DW_CHILDREN_no
    secbuf_emit_uleb128(da, 0x25); // DW_AT_producer
    secbuf_emit_uleb128(da, 0x08); // DW_FORM_string
    secbuf_emit_uleb128(da, 0x13); // DW_AT_language
    secbuf_emit_uleb128(da, 0x0b); // DW_FORM_data1
    secbuf_emit_uleb128(da, 0x03); // DW_AT_name
    secbuf_emit_uleb128(da, 0x08); // DW_FORM_string
    secbuf_emit_uleb128(da, 0x10); // DW_AT_stmt_list
    secbuf_emit_uleb128(da, 0x06); // DW_FORM_data4 (4-byte offset)
    secbuf_emit_uleb128(da, 0x11); // DW_AT_low_pc
    secbuf_emit_uleb128(da, 0x01); // DW_FORM_addr
    secbuf_emit_uleb128(da, 0x12); // DW_AT_high_pc
    secbuf_emit_uleb128(da, 0x07); // DW_FORM_data8 (size, not address)
    secbuf_emit_uleb128(da, 0); // DW_AT_null (end of attrs)
    secbuf_emit_uleb128(da, 0); // DW_FORM_null
    secbuf_emit_uleb128(da, 0); // end of abbrev table (code 0)

    // --- .debug_info ---
    // CU header + DIE
    size_t di_start = di->len;
    secbuf_emit32le(di, 0); // unit_length (placeholder)
    secbuf_emit16le(di, 4); // version (DWARF4)
    secbuf_emit32le(di, 0); // debug_abbrev_offset
    secbuf_emit8(di, 8); // address_size

    // DIE: DW_TAG_compile_unit (abbrev 1)
    secbuf_emit_uleb128(di, 1);
    // DW_AT_producer
    secbuf_emitbuf(di, "rcc", 3);
    secbuf_emit8(di, 0);
    // DW_AT_language: DW_LANG_C11 (0x001d = 29)
    secbuf_emit8(di, 29);
    // DW_AT_name
    const char *name = obj->debug_files[0] ? obj->debug_files[0] : "<unknown>";
    secbuf_emitbuf(di, name, strlen(name));
    secbuf_emit8(di, 0);
    // DW_AT_stmt_list: offset into .debug_line section
    secbuf_emit32le(di, 0);
    // DW_AT_low_pc (needs relocation: offset from .text section start)
    obj->debug_low_pc_offset = di->len;
    obj->debug_has_low_pc = true;
    secbuf_emit64le(di, 0);
    // DW_AT_high_pc: text section size (not an address, no relocation)
    secbuf_emit64le(di, text_end);

    // Patch unit_length
    secbuf_patch32le(di, di_start, (uint32_t)(di->len - di_start - 4));
    // --- .debug_line ---
    size_t dl_start = dl->len;
    secbuf_emit32le(dl, 0); // unit_length (placeholder)
    secbuf_emit16le(dl, 4); // version (DWARF4)
    size_t hdr_len_off = dl->len;
    secbuf_emit32le(dl, 0); // header_length (placeholder)
    secbuf_emit8(dl, 1); // minimum_instruction_length
    secbuf_emit8(dl, 1); // maximum_operations_per_instruction
    secbuf_emit8(dl, 1); // default_is_stmt
    secbuf_emit8(dl, (uint8_t)-5); // line_base (signed)
    secbuf_emit8(dl, 14); // line_range
    secbuf_emit8(dl, 13); // opcode_base (standard)
    secbuf_emit8(dl, 1); // DW_LNS_copy
    secbuf_emit8(dl, 1); // DW_LNS_advance_pc
    secbuf_emit8(dl, 1); // DW_LNS_advance_line
    secbuf_emit8(dl, 1); // DW_LNS_set_file
    secbuf_emit8(dl, 1); // DW_LNS_set_column
    secbuf_emit8(dl, 0); // DW_LNS_negate_stmt
    secbuf_emit8(dl, 0); // DW_LNS_set_basic_block
    secbuf_emit8(dl, 0); // DW_LNS_const_add_pc
    secbuf_emit8(dl, 1); // DW_LNS_fixed_advance_pc
    secbuf_emit8(dl, 0); // DW_LNS_set_prologue_end
    secbuf_emit8(dl, 0); // DW_LNS_set_epilogue_begin
    secbuf_emit8(dl, 1); // DW_LNS_set_isa

    // include_directories (empty)
    secbuf_emit8(dl, 0);

    // file_names
    for (int i = 0; i < obj->debug_file_count; i++) {
        const char *fname = obj->debug_files[i] ? obj->debug_files[i] : "<unknown>";
        secbuf_emitbuf(dl, fname, strlen(fname));
        secbuf_emit8(dl, 0);
        secbuf_emit_uleb128(dl, 0);
        secbuf_emit_uleb128(dl, 0);
        secbuf_emit_uleb128(dl, 0);
    }
    secbuf_emit8(dl, 0); // end of file names

    // Patch header_length
    secbuf_patch32le(dl, hdr_len_off, (uint32_t)(dl->len - hdr_len_off - 4));
    // Line number program — emit DW_LNE_set_address with relocatable address
    // so that gdb can resolve line numbers in linked executables.
    secbuf_emit8(dl, 0); // extended opcode
    secbuf_emit_uleb128(dl, 9); // length: 1 (opcode) + 8 (addr) = 9
    secbuf_emit8(dl, 2); // DW_LNE_set_address
    obj->debug_line_addr_off = dl->len;
    obj->debug_has_line_addr = true;
    secbuf_emit64le(dl, 0); // address placeholder (relocated against .text)
    uint64_t current_pc = 0;
    int current_file = 1;
    int current_line = 1;

    for (int i = 0; i < obj->debug_line_count; i++) {
        DebugLineEntry *e = &obj->debug_lines[i];
        uint64_t addr_advance = e->text_offset - current_pc;

        // Set file if changed
        if ((int)e->file_idx != current_file) {
            secbuf_emit8(dl, 4); // DW_LNS_set_file
            secbuf_emit_uleb128(dl, e->file_idx);
            current_file = (int)e->file_idx;
        }

        int line_delta = (int)e->line - current_line;
        int opcode_base = 13;
        int line_base = -5;
        int line_range = 14;

        // Try to use special opcode
        if ((int64_t)addr_advance <= (255 - opcode_base) / (int)line_range &&
            line_delta >= line_base &&
            line_delta < line_base + line_range) {
            int special = (line_delta - line_base) + (int)addr_advance * line_range + opcode_base;
            if (special <= 255) {
                secbuf_emit8(dl, (uint8_t)special);
                current_pc = e->text_offset;
                current_line = (int)e->line;
                continue;
            }
        }

        // Standard opcodes
        if (addr_advance > 0) {
            secbuf_emit8(dl, 2); // DW_LNS_advance_pc
            secbuf_emit_uleb128(dl, addr_advance);
            current_pc = e->text_offset;
        }
        if (line_delta != 0) {
            secbuf_emit8(dl, 3); // DW_LNS_advance_line
            secbuf_emit_sleb128(dl, line_delta);
            current_line = (int)e->line;
        }
        secbuf_emit8(dl, 1); // DW_LNS_copy
    }

    // End sequence
    secbuf_emit8(dl, 0); // extended opcode
    secbuf_emit_uleb128(dl, 1); // length
    secbuf_emit8(dl, 1); // DW_LNE_end_sequence

    // Patch unit_length
    secbuf_patch32le(dl, dl_start, (uint32_t)(dl->len - dl_start - 4));

    // --- .debug_aranges (address range lookup table) ---
    SecBuf *dar = &obj->debug_aranges_section;
    size_t dar_start = dar->len;
    secbuf_emit32le(dar, 0); // unit_length (placeholder)
    secbuf_emit16le(dar, 2); // version
    secbuf_emit32le(dar, 0); // debug_info_offset = 0
    secbuf_emit8(dar, 8); // address_size
    secbuf_emit8(dar, 0); // segment_size
    while ((dar->len - dar_start) % (2 * 8) != 0)
        secbuf_emit8(dar, 0);
    obj->debug_aranges_addr_off = dar->len;
    obj->debug_has_aranges_addr = true;
    secbuf_emit64le(dar, 0); // address (relocated against .text)
    secbuf_emit64le(dar, text_end); // length
    secbuf_emit64le(dar, 0);
    secbuf_emit64le(dar, 0); // terminator
    secbuf_patch32le(dar, dar_start, (uint32_t)(dar->len - dar_start - 4));
}

// Create parent directories for filepath (like mkdir -p dirname).
void mkdir_p(const char *filepath) {
    char tmp[4096];
    size_t len = strlen(filepath);
    if (len >= sizeof(tmp)) return;
    memcpy(tmp, filepath, len + 1);
    // Find the last path separator to determine the directory part.
    // If no separator is found, there is no parent directory to create.
    bool has_dir = false;
    for (size_t i = len; i > 0; i--) {
        char c = tmp[i - 1];
        if (c == '/'
#ifdef _WIN32
            || c == '\\'
#endif
        ) {
            tmp[i - 1] = '\0';
            len = i - 1;
            has_dir = true;
            break;
        }
    }
    if (!has_dir) return;
    // Now create each parent directory in the path
    for (size_t i = 1; i < len; i++) {
        char c = tmp[i];
        if (c == '/'
#ifdef _WIN32
            || c == '\\'
#endif
        ) {
            tmp[i] = '\0';
#ifdef _WIN32
            _mkdir(tmp);
#else
            mkdir(tmp, 0755);
#endif
            tmp[i] = c;
        }
    }
#ifdef _WIN32
    _mkdir(tmp);
#else
    mkdir(tmp, 0755);
#endif
}
