// SPDX-License-Identifier: LGPL-2.1-or-later
// Section buffer and object file helpers.
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include "obj.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

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

void objfile_init(ObjFile *obj) {
    // wrong -Wstringop-overflow= warning. known gcc bug
    memset(obj, 0, sizeof(ObjFile));
    secbuf_init(&obj->text);
    secbuf_init(&obj->data);
    secbuf_init(&obj->rodata);
    secbuf_init(&obj->init_array);
    secbuf_init(&obj->fini_array);
    secbuf_init(&obj->data_tls);
}

void objfile_free(ObjFile *obj) {
    secbuf_free(&obj->text);
    secbuf_free(&obj->data);
    secbuf_free(&obj->rodata);
    secbuf_free(&obj->init_array);
    secbuf_free(&obj->fini_array);
    secbuf_free(&obj->data_tls);
    for (int i = 0; i < obj->sym_count; i++)
        free(obj->syms[i].name);
    free(obj->syms);
    free(obj->text_relocs);
    free(obj->data_relocs);
    free(obj->rodata_relocs);
    free(obj->init_array_relocs);
    free(obj->fini_array_relocs);
    free(obj->data_tls_relocs);
    free(obj->unwind);
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
    // Check for existing symbol with same name
    for (int i = 0; i < obj->sym_count; i++) {
        if (strcmp(obj->syms[i].name, name) == 0) {
            // Update to defined version if previously undefined
            if (obj->syms[i].section == SEC_UNDEF && section != SEC_UNDEF) {
                obj->syms[i].section = section;
                obj->syms[i].offset = offset;
                obj->syms[i].size = size;
                obj->syms[i].bind = bind;
                obj->syms[i].type = type;
            }
            return i;
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
    return idx;
}

int objfile_find_sym(ObjFile *obj, const char *name) {
    for (int i = 0; i < obj->sym_count; i++)
        if (strcmp(obj->syms[i].name, name) == 0)
            return i;
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
    default: return;
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
