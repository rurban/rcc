/* A bare symbol name (not "SYM - .", not a "LABEL2-LABEL1" difference, not
 * a plain number) in a .byte/.long/.4byte/.quad/... data directive is GAS's
 * plain-absolute-reference case: it needs a real relocation against that
 * symbol, the same as ".quad SYMBOL" already got right. rcc's data-emission
 * code only ever built that relocation for sz==8 ("is_sym && sz == 8") —
 * sz==4 (".long"/".4byte", by far the more common width for this pattern)
 * fell through untouched, silently emitting *nothing at all* for the whole
 * directive and desyncing every later entry's offset in whatever section
 * it shared with prior directives.
 *
 * Found via a real Linux kernel build: arch/x86/include/asm/alternative.h's
 * ALTINSTR_ENTRY() macro emits ".4byte " + a stringified ft_flags
 * expression into a merged .altinstructions section, once per
 * ALTERNATIVE()/alternative_io() call site. arch/x86/include/asm/
 * segment.h's vdso_read_cpunode() calls alternative_io() with plain
 * "X86_FEATURE_RDPID" as that expression — at that exact point in the
 * header chain (confirmed identical, byte for byte, in real GCC's own -E
 * output — not an rcc macro-expansion bug) X86_FEATURE_RDPID isn't yet
 * available as a preprocessor constant, so __stringify() falls back to
 * the bare macro name and GAS resolves *that* as a plain symbol
 * reference. The missing 4 bytes shifted every subsequent
 * .altinstructions entry in init/do_mounts.o, and objtool correctly
 * rejected the whole section: "size not a multiple of 14"
 * (sizeof(struct alt_instr)) — several steps removed from the real cause,
 * a single dropped .4byte field near the start of the file.
 *
 * (This bug also happened to hardcode R_AARCH64_ABS64 unconditionally for
 * the sz==8 case, even on x86-64 builds — fixed alongside to pick the
 * architecture-appropriate absolute relocation type.)
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

static int check_section_bytes_with_reloc(const char *rcc, const char *td, int pid,
                                          const char *tag, const char *src,
                                          const char *section) {
    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_dbs_%s_%d.c", td, tag, pid);
    snprintf(objf, sizeof(objf), "%s/test_dbs_%s_%d.o", td, tag, pid);

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 0; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -c -o %s %s " NULL_REDIRECT, rcc, objf, srcf);
    int rc = system(cmd);
    remove(srcf);
    if (rc != 0) {
        printf("FAIL: [%s] compile failed (rc=%d)\n", tag, rc);
        remove(objf);
        return 0;
    }

    /* Section must be exactly 12 bytes: .long 0 (4) + .4byte SYMBOL (4,
     * relocated) + .long 0 (4) — a missing middle field would collapse
     * this to 8 and misalign the relocation past the third .long. */
    snprintf(cmd, sizeof(cmd), "objdump -h %s " NULL_REDIRECT, objf);
    FILE *p = popen(cmd, "r");
    if (!p) { printf("FAIL: [%s] objdump -h failed\n", tag); remove(objf); return 0; }
    char out[4096];
    size_t n = fread(out, 1, sizeof(out) - 1, p);
    out[n] = '\0';
    pclose(p);

    char *secline = strstr(out, section);
    bool size_ok = false;
    if (secline) {
        char *lineend = strchr(secline, '\n');
        size_t linelen = lineend ? (size_t)(lineend - secline) : strlen(secline);
        char linebuf[256];
        if (linelen >= sizeof(linebuf)) linelen = sizeof(linebuf) - 1;
        memcpy(linebuf, secline, linelen);
        linebuf[linelen] = '\0';
        size_ok = strstr(linebuf, "0000000c") != NULL;
    }
    if (!size_ok) {
        printf("FAIL: [%s] expected %s section size 0xc (12), got:\n%s\n",
               tag, section, out);
        remove(objf);
        return 0;
    }

    snprintf(cmd, sizeof(cmd), "objdump -dr -j %s %s " NULL_REDIRECT, section, objf);
    p = popen(cmd, "r");
    if (!p) { printf("FAIL: [%s] objdump -dr failed\n", tag); remove(objf); return 0; }
    n = fread(out, 1, sizeof(out) - 1, p);
    out[n] = '\0';
    pclose(p);
    remove(objf);

    /* objdump names the same absolute 4-byte relocation differently per
     * object format: ELF's R_X86_64_32 vs. COFF's IMAGE_REL_AMD64_ADDR32. */
#ifdef _WIN32
    const char *reloc_name = "IMAGE_REL_AMD64_ADDR32";
#else
    const char *reloc_name = "R_X86_64_32";
#endif
    if (!strstr(out, reloc_name) || !strstr(out, "bare_sym_target")) {
        printf("FAIL: [%s] expected an absolute 32-bit relocation against "
               "bare_sym_target, got:\n%s\n", tag, out);
        return 0;
    }
    return 1;
}

int main(void)
{
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    int ok = 1;

    /* The exact real-world shape: a bare symbol used as a plain 4-byte
     * data value, sandwiched between two other .long fields (mirroring
     * ALTINSTR_ENTRY's label/replacement/ft_flags layout) so a dropped
     * middle field is directly visible as a section-size shrink. */
    static const char src[] =
        "int bare_sym_target;\n"
        "void f(void) {\n"
        "    asm volatile(\n"
        "        \".pushsection .rodata\\n\\t\"\n"
        "        \".long 0\\n\\t\"\n"
        "        \".4byte bare_sym_target\\n\\t\"\n"
        "        \".long 0\\n\\t\"\n"
        "        \".popsection\\n\\t\"\n"
        "    );\n"
        "}\n"
        "int main(void) { f(); return 0; }\n";
    /* rcc's own logical SEC_RODATA (which ".pushsection .rodata" maps to
     * regardless of target) is written out under a platform-specific real
     * section name: ".rodata" on ELF, but COFF follows the MS/MinGW
     * convention of ".rdata" instead — same section, different label. */
#ifdef _WIN32
    ok &= check_section_bytes_with_reloc(rcc, td, pid, "bare_4byte_sym", src,
                                         ".rdata");
#else
    ok &= check_section_bytes_with_reloc(rcc, td, pid, "bare_4byte_sym", src,
                                         ".rodata");
#endif

    if (!ok) return 1;
    printf("OK bare symbol in .4byte data directive emits a real relocation\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
