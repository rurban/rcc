// SPDX-License-Identifier: LGPL-2.1-or-later
// Shared helpers for the rcc native linker.
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include "link.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

// ---------------------------------------------------------------------------
// Low-level I/O helpers
// ---------------------------------------------------------------------------

static uint8_t r8(const uint8_t *p) { return p[0]; }
static uint16_t r16le(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static uint32_t r32le(const uint8_t *p) {
    return (uint32_t)r16le(p) | ((uint32_t)r16le(p + 2) << 16);
}
static uint64_t r64le(const uint8_t *p) {
    return (uint64_t)r32le(p) | ((uint64_t)r32le(p + 4) << 32);
}

static uint8_t *read_file(const char *path, size_t *out_size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    long sz = ftell(f);
    if (sz < 0) {
        fclose(f);
        return NULL;
    }
    rewind(f);
    uint8_t *buf = malloc((size_t)sz + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    size_t n = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    if (n != (size_t)sz) {
        free(buf);
        return NULL;
    }
    buf[sz] = '\0';
    *out_size = (size_t)sz;
    return buf;
}

// ---------------------------------------------------------------------------
// State management
// ---------------------------------------------------------------------------

void link_state_init(LinkState *s, LinkArch arch, const char *out_path,
                     bool opt_static, bool opt_pie) {
    memset(s, 0, sizeof(*s));
    s->arch = arch;
    s->out_path = out_path;
    s->opt_static = opt_static;
    s->opt_pie = opt_pie;
}

void link_state_free(LinkState *s) {
    for (int i = 0; i < s->n_secs; i++) {
        free(s->secs[i].name);
        free(s->secs[i].data);
        free(s->secs[i].relocs);
    }
    free(s->secs);
    for (int i = 0; i < s->n_syms; i++) free(s->syms[i].name);
    free(s->syms);
    for (int i = 0; i < s->n_objs; i++) {
        free(s->objs[i].path);
        free(s->objs[i].image);
    }
    free(s->objs);
    free(s->sym_hash);
    memset(s, 0, sizeof(*s));
}

static uint64_t align_up(uint64_t v, uint64_t a) {
    return (v + a - 1) & ~(a - 1);
}

// ---------------------------------------------------------------------------
// Sections
// ---------------------------------------------------------------------------

int link_find_or_create_sec(LinkState *s, const char *name, bool alloc,
                            bool write, bool exec, bool is_bss, bool is_tls,
                            size_t align) {
    for (int i = 0; i < s->n_secs; i++) {
        if (strcmp(s->secs[i].name, name) == 0) return i;
    }
    if (s->n_secs == s->cap_secs) {
        s->cap_secs = s->cap_secs ? s->cap_secs * 2 : 8;
        s->secs = realloc(s->secs, (size_t)s->cap_secs * sizeof(LinkSec));
    }
    LinkSec *sec = &s->secs[s->n_secs];
    memset(sec, 0, sizeof(*sec));
    sec->name = strdup(name);
    sec->alloc = alloc;
    sec->write = write;
    sec->exec = exec;
    sec->is_bss = is_bss;
    sec->is_tls = is_tls;
    sec->align = align < 1 ? 1 : align;
    sec->data = is_bss ? NULL : malloc(64);
    sec->cap = is_bss ? 0 : 64;
    sec->cap_relocs = 0;
    sec->relocs = NULL;
    return s->n_secs++;
}

uint64_t link_sec_append(LinkState *s, int sec_idx, const uint8_t *data,
                         size_t len, size_t align) {
    if (sec_idx < 0 || sec_idx >= s->n_secs) return 0;
    LinkSec *sec = &s->secs[sec_idx];
    if (sec->is_bss) {
        sec->len = align_up(sec->len, align) + len;
        return sec->len - len;
    }
    uint64_t off = align_up(sec->len, align);
    size_t need = off + len;
    if (need > sec->cap) {
        size_t newcap = sec->cap ? sec->cap : 64;
        while (newcap < need) newcap *= 2;
        sec->data = realloc(sec->data, newcap);
        sec->cap = newcap;
    }
    memset(sec->data + sec->len, 0, off - sec->len);
    memcpy(sec->data + off, data, len);
    sec->len = need;
    return off;
}

// ---------------------------------------------------------------------------
// Relocations
// ---------------------------------------------------------------------------

void link_add_reloc(LinkState *s, int sec_idx, uint64_t offset, uint32_t type,
                    int sym, int64_t addend) {
    if (sec_idx < 0 || sec_idx >= s->n_secs) return;
    LinkSec *sec = &s->secs[sec_idx];
    if (sec->n_relocs == sec->cap_relocs) {
        sec->cap_relocs = sec->cap_relocs ? sec->cap_relocs * 2 : 16;
        sec->relocs = realloc(sec->relocs, (size_t)sec->cap_relocs * sizeof(LinkReloc));
    }
    LinkReloc *r = &sec->relocs[sec->n_relocs++];
    r->offset = offset;
    r->type = type;
    r->sym = sym;
    r->addend = addend;
}

// ---------------------------------------------------------------------------
// Symbol table with simple hash map
// ---------------------------------------------------------------------------

static unsigned sym_hash_fn(const char *name) {
    unsigned h = 5381;
    while (*name) h = ((h << 5) + h) + (unsigned)*name++;
    return h;
}

static void sym_hash_grow(LinkState *s) {
    int oldcap = s->sym_hash_cap;
    int *old = s->sym_hash;
    s->sym_hash_cap = oldcap ? oldcap * 2 : 256;
    s->sym_hash = calloc((size_t)s->sym_hash_cap, sizeof(int));
    for (int i = 0; i < s->n_syms; i++) s->syms[i].hash_next = 0;
    for (int i = 0; i < s->n_syms; i++) {
        unsigned h = sym_hash_fn(s->syms[i].name) & (s->sym_hash_cap - 1);
        int *slot = &s->sym_hash[h];
        while (*slot) slot = &s->syms[*slot - 1].hash_next;
        *slot = i + 1;
    }
    free(old);
}

int link_find_sym(LinkState *s, const char *name) {
    if (!s->sym_hash || s->sym_hash_cap == 0) return -1;
    unsigned h = sym_hash_fn(name) & (s->sym_hash_cap - 1);
    int idx = s->sym_hash[h];
    while (idx) {
        if (strcmp(s->syms[idx - 1].name, name) == 0) return idx - 1;
        idx = s->syms[idx - 1].hash_next;
    }
    return -1;
}

LinkSym *link_get_sym(LinkState *s, int idx) {
    if (idx < 0 || idx >= s->n_syms) return NULL;
    return &s->syms[idx];
}

int link_add_sym(LinkState *s, const char *name, int sec, uint64_t value,
                 uint64_t size, int bind, int type, int src_obj) {
    if (s->n_syms >= s->sym_hash_cap / 2) sym_hash_grow(s);

    int idx = link_find_sym(s, name);
    if (idx >= 0) {
        LinkSym *sym = &s->syms[idx];
        bool new_def = sec >= 0;
        bool old_def = sym->sec >= 0;
        if (new_def && old_def) {
            if (bind == 0 || sym->bind == 0) {
                // local vs anything: keep existing
                return idx;
            }
            fprintf(stderr, "rcc: link error: duplicate definition of '%s'\n", name);
            return -1;
        }
        if (new_def) {
            sym->sec = sec;
            sym->value = value;
            sym->size = size;
            sym->bind = bind;
            sym->type = type;
            sym->src_obj = src_obj;
            sym->resolved = true;
        }
        return idx;
    }

    if (s->n_syms == s->cap_syms) {
        s->cap_syms = s->cap_syms ? s->cap_syms * 2 : 64;
        s->syms = realloc(s->syms, (size_t)s->cap_syms * sizeof(LinkSym));
    }
    LinkSym *sym = &s->syms[s->n_syms];
    sym->name = strdup(name);
    sym->sec = sec;
    sym->value = value;
    sym->size = size;
    sym->bind = bind;
    sym->type = type;
    sym->src_obj = src_obj;
    sym->hash_next = 0;
    sym->resolved = sec >= 0;

    unsigned h = sym_hash_fn(name) & (s->sym_hash_cap - 1);
    int *slot = &s->sym_hash[h];
    while (*slot) slot = &s->syms[*slot - 1].hash_next;
    *slot = s->n_syms + 1;

    return s->n_syms++;
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

int link_layout(LinkState *s, uint64_t base, uint64_t page_align) {
    if (page_align == 0) page_align = 1;
    uint64_t addr = base;
    uint64_t fileoff = 0;
    // text first
    for (int i = 0; i < s->n_secs; i++) {
        LinkSec *sec = &s->secs[i];
        if (!sec->alloc || !sec->exec || sec->is_tls) continue;
        sec->addr = align_up(addr, sec->align);
        sec->fileoff = sec->is_bss ? 0 : align_up(fileoff, sec->align);
        addr = sec->addr + sec->len;
        if (!sec->is_bss) fileoff = sec->fileoff + sec->len;
    }
    // rodata
    addr = align_up(addr, page_align);
    fileoff = align_up(fileoff, page_align);
    for (int i = 0; i < s->n_secs; i++) {
        LinkSec *sec = &s->secs[i];
        if (!sec->alloc || sec->write || sec->exec || sec->is_tls) continue;
        sec->addr = align_up(addr, sec->align);
        sec->fileoff = sec->is_bss ? 0 : align_up(fileoff, sec->align);
        addr = sec->addr + sec->len;
        if (!sec->is_bss) fileoff = sec->fileoff + sec->len;
    }
    // data
    addr = align_up(addr, page_align);
    fileoff = align_up(fileoff, page_align);
    for (int i = 0; i < s->n_secs; i++) {
        LinkSec *sec = &s->secs[i];
        if (!sec->alloc || !sec->write || sec->is_bss || sec->is_tls) continue;
        sec->addr = align_up(addr, sec->align);
        sec->fileoff = align_up(fileoff, sec->align);
        addr = sec->addr + sec->len;
        fileoff = sec->fileoff + sec->len;
    }
    // bss
    addr = align_up(addr, page_align);
    for (int i = 0; i < s->n_secs; i++) {
        LinkSec *sec = &s->secs[i];
        if (!sec->is_bss || sec->is_tls) continue;
        sec->addr = align_up(addr, sec->align);
        sec->fileoff = 0;
        addr = sec->addr + sec->len;
    }
    // tls
    addr = align_up(addr, page_align);
    fileoff = align_up(fileoff, page_align);
    for (int i = 0; i < s->n_secs; i++) {
        LinkSec *sec = &s->secs[i];
        if (!sec->is_tls) continue;
        sec->addr = align_up(addr, sec->align);
        sec->fileoff = sec->is_bss ? 0 : align_up(fileoff, sec->align);
        addr = sec->addr + sec->len;
        if (!sec->is_bss) fileoff = sec->fileoff + sec->len;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Architecture-specific relocation encoding
// ---------------------------------------------------------------------------

static void w64le(uint8_t *p, uint64_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
    p[4] = (uint8_t)(v >> 32);
    p[5] = (uint8_t)(v >> 40);
    p[6] = (uint8_t)(v >> 48);
    p[7] = (uint8_t)(v >> 56);
}
static void w32le(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}
static uint64_t r64le_m(uint8_t *p) { return r64le(p); }
static uint32_t r32le_m(uint8_t *p) { return r32le(p); }

static int64_t sign_extend(uint64_t v, int bits) {
    uint64_t mask = 1ULL << (bits - 1);
    return (int64_t)((v ^ mask) - mask);
}

void link_reloc_apply(LinkArch arch, LinkSec *sec, LinkReloc *r,
                      uint64_t sym_addr, uint64_t pc) {
    uint8_t *p = sec->data + r->offset;
    uint64_t S = sym_addr;
    int64_t A = r->addend;
    (void)arch;
    switch (r->type) {
    case RL_ABS64:
        w64le(p, r64le_m(p) + S + (uint64_t)A);
        break;
    case RL_ABS32:
        w32le(p, (uint32_t)((int32_t)r32le_m(p) + (int32_t)A + (int32_t)S));
        break;
    case RL_ABS32U:
        w32le(p, (uint32_t)(r32le_m(p) + (uint64_t)A + S));
        break;
    case RL_PC32:
    case RL_PC32_PLT:
        w32le(p, (uint32_t)((int32_t)r32le_m(p) + (int32_t)A + (int64_t)(S - pc)));
        break;
    case RL_PC64:
        w64le(p, r64le_m(p) + (uint64_t)A + S - pc);
        break;
    case RL_GOTPCREL: {
        // For a minimal linker without a real GOT, treat as PC32.
        w32le(p, (uint32_t)((int32_t)r32le_m(p) + (int32_t)A + (int64_t)(S - pc)));
        break;
    }
    case RL_TPOFF32: {
        uint64_t tls_base = 0; // runtime fills this; TLSLE is relative to tcb
        (void)tls_base;
        w32le(p, (uint32_t)((int32_t)r32le_m(p) + (int32_t)A + (int64_t)S));
        break;
    }
    case RL_ARM64_B26: {
        uint32_t ins = r32le_m(p);
        int64_t delta = (int64_t)(S + (uint64_t)A - pc);
        delta >>= 2;
        ins = (ins & ~0x03ffffffu) | ((uint32_t)(delta & 0x03ffffffu));
        w32le(p, ins);
        break;
    }
    case RL_ARM64_ADR_PG: {
        uint32_t ins = r32le_m(p);
        int64_t delta = (int64_t)((S + (uint64_t)A) - (pc & ~(uint64_t)0xfff));
        int64_t imm = delta >> 12;
        ins = (ins & 0x9f00001f) |
            ((uint32_t)(imm & 3) << 29) |
            ((uint32_t)((imm >> 2) & 0x7ffff) << 5);
        w32le(p, ins);
        break;
    }
    case RL_ARM64_ADD_LO: {
        uint32_t ins = r32le_m(p);
        uint64_t off = (S + (uint64_t)A) & 0xfff;
        ins = (ins & 0xffc003ff) | ((uint32_t)(off >> 3) << 10);
        w32le(p, ins);
        break;
    }
    case RL_ARM64_GOT_PG:
    case RL_ARM64_GOT_LO:
        // No real GOT yet; resolve directly.
        link_reloc_apply(arch, sec, r, sym_addr, pc);
        break;
    default:
        fprintf(stderr, "rcc: link warning: unhandled relocation %u\n", r->type);
        break;
    }
}

// ---------------------------------------------------------------------------
// Apply all relocations
// ---------------------------------------------------------------------------

void link_apply_relocs(LinkState *s) {
    for (int i = 0; i < s->n_secs; i++) {
        LinkSec *sec = &s->secs[i];
        for (int j = 0; j < sec->n_relocs; j++) {
            LinkReloc *r = &sec->relocs[j];
            LinkSym *sym = link_get_sym(s, r->sym);
            if (!sym) continue;
            uint64_t sym_addr = 0;
            if (sym->sec >= 0) {
                sym_addr = s->secs[sym->sec].addr + sym->value;
            }
            uint64_t pc = sec->addr + r->offset;
            link_reloc_apply(s->arch, sec, r, sym_addr, pc);
        }
    }
}

// ---------------------------------------------------------------------------
// Archive loading (simple ar format reader)
// ---------------------------------------------------------------------------

static int link_try_load_archive(LinkState *s, const char *path) {
    size_t sz;
    uint8_t *data = read_file(path, &sz);
    if (!data) return -1;
    if (sz < 8 || memcmp(data, "!<arch>\n", 8) != 0) {
        free(data);
        return -1;
    }
    // Minimal ar parser: we only handle the GNU/SVR4 variant with long names.
    // For now, leave this as a hook.  Real archive loading is format-specific.
    (void)s;
    free(data);
    return 0;
}

int link_load_archive(LinkState *s, const char *name, const char *lib_paths) {
    (void)s;
    (void)name;
    (void)lib_paths;
    return -1; // TODO: implement real archive search
}

// ---------------------------------------------------------------------------
// Top-level driver
// ---------------------------------------------------------------------------

int rcc_link(const char *out_path, char **obj_paths, int n_objs,
             const char *libs, bool opt_pie, bool opt_pic, bool opt_static) {
    // Native linker only handles host-native ELF; fall back for cross targets.
    // On Windows/mingw hosts, .exe is the normal extension for native binaries.
#if !defined(_WIN32) && !defined(__MINGW32__)
    size_t out_len = strlen(out_path);
    if (out_len > 4 && strcmp(out_path + out_len - 4, ".exe") == 0) return -1;
#endif
    (void)libs;
#ifdef __APPLE__
    LinkArch arch = ARCH_AARCH64;
#elif defined(__x86_64__)
    LinkArch arch = ARCH_X86_64;
#elif defined(__aarch64__)
    LinkArch arch = ARCH_AARCH64;
#else
#error "unsupported host architecture"
#endif

    LinkState state;
    link_state_init(&state, arch, out_path, opt_static, opt_pie || opt_pic);

    for (int i = 0; i < n_objs; i++) {
        if (link_load_object(&state, obj_paths[i]) != 0) {
            link_state_free(&state);
            return 1;
        }
    }

    int rc;
#ifdef __APPLE__
    rc = link_macho(&state);
#elif defined(_WIN32) || defined(__MINGW32__)
    rc = link_pe(&state);
#else
    rc = link_elf(&state);
#endif

    link_state_free(&state);
    return rc;
}
