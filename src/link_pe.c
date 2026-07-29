#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
// SPDX-License-Identifier: LGPL-2.1-or-later
// Native PE/COFF linker for rcc (Windows/MinGW).
#include "link.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef _WIN32
#include <malloc.h>
#else
#include <sys/mman.h>
#endif
#include <errno.h>

// ---------------------------------------------------------------------------
// COFF / PE constants
// ---------------------------------------------------------------------------

#define IMAGE_FILE_MACHINE_AMD64  0x8664
#define IMAGE_FILE_MACHINE_ARM64  0xAA64

// COFF relocation types (x86-64)
#define IMAGE_REL_AMD64_ADDR64    1
#define IMAGE_REL_AMD64_ADDR32    2
#define IMAGE_REL_AMD64_ADDR32NB  3
#define IMAGE_REL_AMD64_REL32     4
#define IMAGE_REL_AMD64_REL32_1   5
#define IMAGE_REL_AMD64_REL32_2   6
#define IMAGE_REL_AMD64_REL32_4   7
#define IMAGE_REL_AMD64_REL32_5   8
#define IMAGE_REL_AMD64_SECTION   10
#define IMAGE_REL_AMD64_SECREL    11

// COFF relocation types (ARM64)
#define IMAGE_REL_ARM64_ADDR64           1
#define IMAGE_REL_ARM64_ADDR32           2
#define IMAGE_REL_ARM64_BRANCH26         3
#define IMAGE_REL_ARM64_PAGEBASE_REL21   4
#define IMAGE_REL_ARM64_PAGEOFFSET_12A   5

// Section flags
#define IMAGE_SCN_CNT_CODE               0x00000020
#define IMAGE_SCN_CNT_INITIALIZED_DATA   0x00000040
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA 0x00000080
#define IMAGE_SCN_MEM_EXECUTE            0x20000000
#define IMAGE_SCN_MEM_READ               0x40000000
#define IMAGE_SCN_MEM_WRITE              0x80000000

// Symbol storage class
#define IMAGE_SYM_CLASS_EXTERNAL         2
#define IMAGE_SYM_CLASS_STATIC           3
#define IMAGE_SYM_CLASS_WEAK_EXTERNAL    105

// PE optional header magic
#define PE32PLUS_MAGIC 0x20b

// PE subsystem
#define IMAGE_SUBSYSTEM_WINDOWS_CUI  3

// DLL characteristics
#define IMAGE_DLLCHARACTERISTICS_NX_COMPAT  0x100
#define IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE 0x40

// Section alignment
#define PE_SECTION_ALIGN  0x1000
#define PE_FILE_ALIGN     0x200

// ---------------------------------------------------------------------------
// Helpers (local)
// ---------------------------------------------------------------------------

static uint16_t pe_r16le(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static uint32_t pe_r32le(const uint8_t *p) {
    return (uint32_t)pe_r16le(p) | ((uint32_t)pe_r16le(p + 2) << 16);
}
static void pe_w16le(FILE *f, uint16_t v) {
    fputc(v & 0xFF, f);
    fputc(v >> 8, f);
}
static void pe_w32le(FILE *f, uint32_t v) {
    pe_w16le(f, (uint16_t)v);
    pe_w16le(f, (uint16_t)(v >> 16));
}
static void pe_w64le(FILE *f, uint64_t v) {
    pe_w32le(f, (uint32_t)v);
    pe_w32le(f, (uint32_t)(v >> 32));
}
static void pe_wbuf(FILE *f, const void *b, size_t n) { fwrite(b, 1, n, f); }
static void pe_wzeros(FILE *f, size_t n) {
    static const uint8_t z[512] = {0};
    while (n > 512) {
        pe_wbuf(f, z, 512);
        n -= 512;
    }
    pe_wbuf(f, z, n);
}
static uint64_t pe_align_up(uint64_t v, uint64_t a) {
    return (v + a - 1) & ~(a - 1);
}

// ---------------------------------------------------------------------------
// COFF → internal relocation type mapping
// ---------------------------------------------------------------------------
static int pe_map_reloc(uint16_t coff_type, uint16_t machine) {
    if (machine == IMAGE_FILE_MACHINE_AMD64) {
        switch (coff_type) {
        case IMAGE_REL_AMD64_ADDR64: return RL_ABS64;
        case IMAGE_REL_AMD64_ADDR32: return RL_ABS32;
        case IMAGE_REL_AMD64_REL32: return RL_PC32;
        case IMAGE_REL_AMD64_REL32_1: return RL_PC32;
        case IMAGE_REL_AMD64_REL32_2: return RL_PC32;
        case IMAGE_REL_AMD64_REL32_4: return RL_PC32;
        case IMAGE_REL_AMD64_REL32_5: return RL_PC32;
        default: return -1;
        }
    } else if (machine == IMAGE_FILE_MACHINE_ARM64) {
        switch (coff_type) {
        case IMAGE_REL_ARM64_ADDR64: return RL_ABS64;
        case IMAGE_REL_ARM64_ADDR32: return RL_ABS32;
        case IMAGE_REL_ARM64_BRANCH26: return RL_ARM64_B26;
        case IMAGE_REL_ARM64_PAGEBASE_REL21: return RL_ARM64_ADR_PG;
        case IMAGE_REL_ARM64_PAGEOFFSET_12A: return RL_ARM64_ADD_LO;
        default: return -1;
        }
    }
    return -1;
}

// ---------------------------------------------------------------------------
// COFF object file loader
// ---------------------------------------------------------------------------

int link_load_object(LinkState *s, const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "rcc: link: cannot open %s: %s\n", path, strerror(errno));
        return -1;
    }
    struct stat st;
    if (fstat(fd, &st) != 0) {
        close(fd);
        return -1;
    }
    uint8_t *image;
#ifdef _WIN32
    image = malloc((size_t)st.st_size);
    if (!image) {
        close(fd);
        return -1;
    }
    if (read(fd, image, (size_t)st.st_size) != (ssize_t)st.st_size) {
        free(image);
        close(fd);
        return -1;
    }
    close(fd);
#else
    image = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (image == MAP_FAILED) return -1;
#endif

    // Check if ELF (shouldn't happen on Windows, but handle gracefully)
    if (st.st_size >= 4 && image[0] == 0x7f && image[1] == 'E' &&
        image[2] == 'L' && image[3] == 'F') {
        free(image);
        fprintf(stderr, "rcc: link: %s: ELF objects not supported by PE linker\n", path);
        return -1;
    }

    // COFF file header (20 bytes)
    if (st.st_size < 20) {
        free(image);
        return -1;
    }
    uint16_t machine = pe_r16le(image);
    uint16_t n_secs = pe_r16le(image + 2);
    uint32_t symtab_off = pe_r32le(image + 8);
    uint32_t n_syms = pe_r32le(image + 12);
    uint16_t opthdr_size = pe_r16le(image + 16);

    if (machine != IMAGE_FILE_MACHINE_AMD64 && machine != IMAGE_FILE_MACHINE_ARM64) {
        fprintf(stderr, "rcc: link: %s: unsupported machine 0x%x\n", path, machine);
        free(image);
        return -1;
    }

    // Section headers (40 bytes each, after 20-byte header + optional header)
    uint32_t sec_hdr_off = 20 + opthdr_size;
    int *out_sec_map = calloc((size_t)n_secs, sizeof(int));

    for (uint16_t i = 0; i < n_secs; i++) {
        const uint8_t *shdr = image + sec_hdr_off + (uint32_t)i * 40;
        char sname[9] = {0};
        memcpy(sname, shdr, 8);
        for (int j = 7; j >= 0 && (sname[j] == ' ' || sname[j] == '\0'); j--)
            sname[j] = '\0';

        uint32_t raw_size = pe_r32le(shdr + 16);
        uint32_t raw_offset = pe_r32le(shdr + 20);
        uint32_t n_relocs = pe_r16le(shdr + 32);
        uint32_t reloc_off = pe_r32le(shdr + 24);
        uint32_t sec_flags = pe_r32le(shdr + 36);

        bool exec = (sec_flags & IMAGE_SCN_MEM_EXECUTE) != 0;
        bool write = (sec_flags & IMAGE_SCN_MEM_WRITE) != 0;
        bool is_bss = (sec_flags & IMAGE_SCN_CNT_UNINITIALIZED_DATA) != 0;

        int out_idx = link_find_or_create_sec(s, sname, true, write, exec,
                                              is_bss, false, 16);
        out_sec_map[i] = out_idx;

        if (!is_bss && raw_size > 0 && raw_offset > 0) {
            uint64_t off = link_sec_append(s, out_idx, image + raw_offset,
                                           raw_size, 16);

            for (uint32_t j = 0; j < n_relocs; j++) {
                const uint8_t *rp = image + reloc_off + j * 10;
                uint32_t r_va = pe_r32le(rp);
                uint32_t r_sym = pe_r32le(rp + 4);
                uint16_t r_type = pe_r16le(rp + 8);
                int rl = pe_map_reloc(r_type, machine);
                if (rl < 0) {
                    fprintf(stderr, "rcc: link: %s: unhandled COFF reloc type %u\n",
                            path, r_type);
                    continue;
                }
                link_add_reloc(s, out_idx, off + r_va, (uint32_t)rl,
                               (int)r_sym, 0);
            }
        } else if (is_bss && raw_size > 0) {
            s->secs[out_idx].len += raw_size;
        }
    }

    // Symbol table (18 bytes per entry)
    const uint8_t *symtab = image + symtab_off;
    const uint8_t *strtab_base = symtab + (uint32_t)n_syms * 18;
    int *sym_map = calloc((size_t)n_syms, sizeof(int));

    for (uint32_t i = 0; i < n_syms;) {
        const uint8_t *se = symtab + (uint32_t)i * 18;
        char sym_name[256];
        if (se[0] == 0 && se[1] == 0 && se[2] == 0 && se[3] == 0) {
            uint32_t off = pe_r32le(se + 4);
            const char *strtab = (const char *)(strtab_base);
            snprintf(sym_name, sizeof(sym_name), "%s", strtab + off);
        } else {
            memcpy(sym_name, se, 8);
            sym_name[8] = '\0';
            for (int j = 7; j >= 0; j--) {
                if (sym_name[j] == '\0' || sym_name[j] == ' ') sym_name[j] = '\0';
                else
                    break;
            }
        }

        uint32_t value = pe_r32le(se + 8);
        int16_t sec_num = (int16_t)pe_r16le(se + 12);
        uint16_t sym_type = pe_r16le(se + 14);
        uint8_t storage_class = se[16];
        uint8_t num_aux = se[17];

        int bind, type, out_sec;
        if (sec_num <= 0) {
            bind = (storage_class == IMAGE_SYM_CLASS_WEAK_EXTERNAL) ? 2 : 1;
            // COFF: bit 0x20 in type field indicates function
            type = (sym_type & 0x20) ? 2 : 0;
            out_sec = -1;
        } else if (sec_num <= (int16_t)n_secs) {
            bind = (storage_class == IMAGE_SYM_CLASS_EXTERNAL) ? 1 : 0;
            type = 0;
            out_sec = out_sec_map[sec_num - 1];
        } else {
            sym_map[i] = -1;
            i += 1 + num_aux;
            continue;
        }

        uint64_t sym_value = (out_sec >= 0) ? value : 0;
        int sym_idx = link_add_sym(s, sym_name, out_sec, sym_value,
                                   0, bind, type, -1);
        sym_map[i] = sym_idx;
        i += 1 + num_aux;
    }

    // Re-map relocations: COFF sym index → LinkSym index
    for (int i = 0; i < s->n_secs; i++) {
        LinkSec *sec = &s->secs[i];
        for (int j = 0; j < sec->n_relocs; j++) {
            int cs = sec->relocs[j].sym;
            if (cs >= 0 && cs < (int)n_syms)
                sec->relocs[j].sym = sym_map[cs];
        }
    }

    // Track object for cleanup
    LinkObj obj = {.path = strdup(path), .image = image, .image_size = (size_t)st.st_size};
    if (s->n_objs == s->cap_objs) {
        s->cap_objs = s->cap_objs ? s->cap_objs * 2 : 4;
        s->objs = realloc(s->objs, (size_t)s->cap_objs * sizeof(LinkObj));
    }
    s->objs[s->n_objs++] = obj;
    free(out_sec_map);
    free(sym_map);
    return 0;
}

// ---------------------------------------------------------------------------
// PE executable writer
// ---------------------------------------------------------------------------

static uint64_t pe_symbol_address(LinkState *s, int idx) {
    LinkSym *sym = &s->syms[idx];
    if (sym->sec >= 0) return s->secs[sym->sec].addr + sym->value;
    return sym->value;
}

// ---------------------------------------------------------------------------
// Main PE link entry point
// ---------------------------------------------------------------------------

int link_pe(LinkState *s) {
    // Create standard sections
    link_find_or_create_sec(s, ".text", true, false, true, false, false, 16);
    link_find_or_create_sec(s, ".data", true, true, false, false, false, 16);
    link_find_or_create_sec(s, ".rdata", true, false, false, false, false, 16);
    link_find_or_create_sec(s, ".bss", true, true, false, true, false, 16);

    // Find entry point
    int entry_sym = link_find_sym(s, "main");
    if (entry_sym < 0) entry_sym = link_find_sym(s, "_main");
    if (entry_sym < 0) entry_sym = link_find_sym(s, "WinMain");
    if (entry_sym < 0) entry_sym = link_find_sym(s, "mainCRTStartup");

    // Layout sections
    uint64_t base = 0x140000000ULL;
    if (link_layout(s, base, PE_SECTION_ALIGN) != 0) return -1;

    // Apply relocations
    link_apply_relocs(s);

    // Entry point
    uint64_t entry_addr = 0;
    if (entry_sym >= 0) entry_addr = pe_symbol_address(s, entry_sym);

    // Count output sections and compute image size
    int n_output_secs = 0;
    uint64_t image_end = 0;
    for (int i = 0; i < s->n_secs; i++) {
        LinkSec *sec = &s->secs[i];
        if (!sec->alloc || sec->len == 0) continue;
        n_output_secs++;
        uint64_t end = sec->addr + pe_align_up(sec->len, PE_SECTION_ALIGN);
        if (end > image_end) image_end = end;
    }
    uint64_t size_of_image = pe_align_up(image_end - base, PE_SECTION_ALIGN);

    // Header sizes
    uint32_t dos_stub_size = 64;
    uint32_t hdr_off = 64 + dos_stub_size + 4 + 20 + 112; // end of optional header
    uint32_t sec_hdr_total = (uint32_t)n_output_secs * 40;
    uint32_t hdr_total = hdr_off + sec_hdr_total;
    uint32_t hdr_file_size = (uint32_t)pe_align_up(hdr_total, PE_FILE_ALIGN);

    // Open output file
    FILE *f = fopen(s->out_path, "wb");
    if (!f) {
        fprintf(stderr, "rcc: link: cannot create %s: %s\n",
                s->out_path, strerror(errno));
        return -1;
    }

    // --- DOS Header ---
    static const uint8_t dos_stub[] =
        "This program cannot be run in DOS mode.\r\n$";
    pe_w16le(f, 0x5A4D); // MZ
    pe_w16le(f, 0x0090);
    pe_w16le(f, 0x0003); // size fields
    pe_w16le(f, 0x0000);
    pe_w16le(f, 0x0004); // reloc, hdr paras
    pe_w16le(f, 0x0000);
    pe_w16le(f, 0xFFFF);
    pe_w16le(f, 0x0000);
    pe_w16le(f, 0x00B8); // SS:SP
    pe_w16le(f, 0x0000); // checksum
    pe_w16le(f, 0x0000);
    pe_w16le(f, 0x0000); // IP:CS
    pe_w16le(f, 0x0040);
    pe_w16le(f, 0x0000); // reloc off, overlay
    for (int i = 0; i < 4; i++) pe_w16le(f, 0);
    pe_w16le(f, 0);
    pe_w16le(f, 0); // OEM
    for (int i = 0; i < 10; i++) pe_w16le(f, 0);
    pe_w32le(f, 64 + dos_stub_size); // e_lfanew
    pe_wbuf(f, dos_stub, sizeof(dos_stub) - 1);
    pe_wzeros(f, dos_stub_size - (sizeof(dos_stub) - 1));

    // --- PE Signature ---
    pe_wbuf(f, "PE\0\0", 4);

    // --- COFF Header ---
    uint16_t machine = IMAGE_FILE_MACHINE_AMD64;
    if (s->arch == ARCH_AARCH64) machine = IMAGE_FILE_MACHINE_ARM64;
    pe_w16le(f, machine);
    pe_w16le(f, (uint16_t)n_output_secs);
    pe_w32le(f, 0);
    pe_w32le(f, 0);
    pe_w32le(f, 0); // timestamp, symtab, nsyms
    pe_w16le(f, 112); // sizeof optional header
    pe_w16le(f, 0x0022); // EXECUTABLE | LARGE_ADDRESS_AWARE

    // --- Optional Header (PE32+) ---
    pe_w16le(f, PE32PLUS_MAGIC);
    fputc(0, f);
    fputc(0, f); // linker ver
    pe_w32le(f, 0);
    pe_w32le(f, 0);
    pe_w32le(f, 0); // sizes of code/data/bss
    pe_w32le(f, (uint32_t)(entry_addr - base)); // RVA of entry
    pe_w32le(f, 0x1000); // base of code RVA
    pe_w64le(f, base);
    pe_w32le(f, PE_SECTION_ALIGN);
    pe_w32le(f, PE_FILE_ALIGN);
    pe_w16le(f, 6);
    pe_w16le(f, 0); // OS ver
    pe_w16le(f, 0);
    pe_w16le(f, 0); // image ver
    pe_w16le(f, 6);
    pe_w16le(f, 0); // subsystem ver
    pe_w32le(f, 0); // Win32Version
    pe_w32le(f, (uint32_t)size_of_image);
    pe_w32le(f, hdr_file_size); // SizeOfHeaders
    pe_w32le(f, 0); // CheckSum
    pe_w16le(f, IMAGE_SUBSYSTEM_WINDOWS_CUI);
    uint16_t dll_flags = IMAGE_DLLCHARACTERISTICS_NX_COMPAT |
        IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE;
    pe_w16le(f, dll_flags);
    pe_w64le(f, 0x100000);
    pe_w64le(f, 0x1000); // stack
    pe_w64le(f, 0x100000);
    pe_w64le(f, 0x1000); // heap
    pe_w32le(f, 0); // loader flags
    pe_w32le(f, 16); // number of directory entries

    // Data directories (16 entries)
    for (int i = 0; i < 16; i++) {
        pe_w32le(f, 0);
        pe_w32le(f, 0);
    }

    // --- Section Headers ---
    // We need to know file offsets before writing; compute them in a first pass
    uint32_t *sec_file_offs = calloc((size_t)s->n_secs, sizeof(uint32_t));
    uint32_t cur_file_off = hdr_file_size;
    for (int i = 0; i < s->n_secs; i++) {
        LinkSec *sec = &s->secs[i];
        if (!sec->alloc || sec->len == 0) continue;
        sec_file_offs[i] = cur_file_off;
        cur_file_off += (uint32_t)pe_align_up(sec->len, PE_FILE_ALIGN);
    }

    uint32_t sec_idx = 0;
    for (int i = 0; i < s->n_secs; i++) {
        LinkSec *sec = &s->secs[i];
        if (!sec->alloc || sec->len == 0) continue;

        char sname[8] = {0};
        memcpy(sname, sec->name, 8);
        sname[7] = '\0';
        pe_wbuf(f, sname, 8);

        uint32_t virt_size = (uint32_t)pe_align_up(sec->len, PE_SECTION_ALIGN);
        uint32_t virt_addr = (uint32_t)(sec->addr - base);
        uint32_t raw_size = (uint32_t)pe_align_up(sec->len, PE_FILE_ALIGN);
        uint32_t raw_off = sec_file_offs[i];
        uint32_t flags = IMAGE_SCN_MEM_READ;
        if (sec->exec)
            flags |= IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE;
        else if (sec->is_bss)
            flags |= IMAGE_SCN_CNT_UNINITIALIZED_DATA;
        else
            flags |= IMAGE_SCN_CNT_INITIALIZED_DATA;
        if (sec->write) flags |= IMAGE_SCN_MEM_WRITE;

        pe_w32le(f, virt_size);
        pe_w32le(f, virt_addr);
        pe_w32le(f, raw_size);
        pe_w32le(f, raw_off);
        pe_w32le(f, 0);
        pe_w32le(f, 0); // reloc ptr, linenum ptr
        pe_w16le(f, 0);
        pe_w16le(f, 0); // nrelocs, nlinenums
        pe_w32le(f, flags);
        sec_idx++;
    }
    free(sec_file_offs);

    // Pad to section alignment
    uint64_t ft = ftell(f);
    if ((uint64_t)hdr_file_size > ft)
        pe_wzeros(f, (size_t)((uint64_t)hdr_file_size - ft));

    // --- Write Section Data ---
    // Collect writeable sections sorted by address
    typedef struct {
        LinkSec *sec;
        uint32_t file_off;
    } Wsec;
    int n_ws = 0;
    Wsec *ws = NULL;
    cur_file_off = hdr_file_size;
    for (int i = 0; i < s->n_secs; i++) {
        LinkSec *sec = &s->secs[i];
        if (!sec->alloc || sec->is_bss || sec->len == 0) continue;
        ws = realloc(ws, (size_t)(n_ws + 1) * sizeof(Wsec));
        ws[n_ws].sec = sec;
        ws[n_ws].file_off = cur_file_off;
        cur_file_off += (uint32_t)pe_align_up(sec->len, PE_FILE_ALIGN);
        n_ws++;
    }
    // Sort by file offset
    for (int i = 0; i < n_ws; i++)
        for (int j = i + 1; j < n_ws; j++)
            if (ws[j].file_off < ws[i].file_off) {
                Wsec t = ws[i];
                ws[i] = ws[j];
                ws[j] = t;
            }

    cur_file_off = hdr_file_size;
    for (int i = 0; i < n_ws; i++) {
        if (ws[i].file_off > cur_file_off)
            pe_wzeros(f, ws[i].file_off - cur_file_off);
        pe_wbuf(f, ws[i].sec->data, ws[i].sec->len);
        cur_file_off = ws[i].file_off +
            (uint32_t)pe_align_up(ws[i].sec->len, PE_FILE_ALIGN);
    }
    free(ws);

    fclose(f);
    return 0;
}
