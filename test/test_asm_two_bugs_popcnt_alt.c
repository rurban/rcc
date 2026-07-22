/* Two separate, independently confirmed rcc bugs, both hit by the exact
 * same real kernel construct in sequence, so they're bundled in one test:
 *
 * 1. encode_x86()'s "pop" mnemonic dispatch used a 3-character prefix
 *    match ("!strncmp(mnem, "pop", 3)"), which also swallowed "popcnt"/
 *    "popcntl"/"popcntq" (POPCNT — an entirely different instruction that
 *    merely happens to start with "pop") and silently mis-encoded it as a
 *    plain POP of whatever the first operand happened to be, dropping the
 *    rest — no warning, no error, just the wrong bytes.
 *
 * 2. When the same numeric local label text (GAS's "774:"/"774f"-style
 *    reused labels) is defined many times across *separate* inline-asm
 *    statements in one translation unit — as arch_hweight32()'s
 *    ALTERNATIVE() macro does at every one of its many call sites via
 *    __always_inline — a still-forward-referenced "774f" used in a
 *    "(label) - ." data directive (ALTERNATIVE()'s ".long 774f - .")
 *    resolved through one shared, non-unique symbol named "774" in the
 *    object's symbol table, rather than a fresh per-occurrence symbol.
 *    Since that symbol's value is whatever the *last* "774:" definition
 *    anywhere in the file set it to, every earlier occurrence's
 *    relocation silently pointed at the wrong (someone else's) replacement
 *    instruction — or, once the object is fully linked, at nothing at
 *    all for all but the very last occurrence, which is what objtool's
 *    ".altinstr_replacement+0x..: special: can't find new instruction"
 *    surfaces.
 *
 * Found via a real Linux kernel build: crypto/jitterentropy.c and (for
 * bug 2, needing multiple call sites) crypto/asymmetric_keys/x509_loader.c
 * both use arch_hweight32()/arch_hweight64(), i.e.
 * arch/x86/include/asm/arch_hweight.h's
 *   asm_inline(ALTERNATIVE("call __sw_hweight32",
 *                          "popcntl %[val], %[cnt]", X86_FEATURE_POPCNT)
 *              : [cnt] "=" REG_OUT (res), ASM_CALL_CONSTRAINT
 *              : [val] REG_IN (w));
 * called from many different functions throughout one file.
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

#define HWEIGHT32_ASM(w, res) \
    asm volatile ( \
        "771:\n\tcall __sw_hweight32\n772:\n" \
        ".pushsection .altinstructions, \"aM\", @progbits, 14\n" \
        " .long 771b - .\n" \
        " .long 774f - .\n" \
        " .4byte 3\n" \
        " .byte 772b-771b\n" \
        " .byte 775f-774f\n" \
        ".popsection\n" \
        ".pushsection .altinstr_replacement, \"ax\"\n" \
        "774:\n\tpopcntl %[val], %[cnt]\n775:\n" \
        ".popsection\n" \
        : [cnt] "=a" (res) \
        : [val] "D" (w))

unsigned int __sw_hweight32(unsigned int w) { return 0; }

/* Three separate call sites, one per function, mirroring how
 * __always_inline duplicates the whole ALTERNATIVE() construct (and its
 * "774:" label text) at each one — exactly the shape that broke bug 2. */
static int f1(unsigned int a) { unsigned int r; HWEIGHT32_ASM(a, r); return (int)r; }
static int f2(unsigned int a) { unsigned int r; HWEIGHT32_ASM(a, r); return (int)r; }
static int f3(unsigned int a) { unsigned int r; HWEIGHT32_ASM(a, r); return (int)r; }

/* Bug 2's real regression only shows up in the object file's
 * relocations, not at runtime (an unpatched ALTERNATIVE() site simply
 * executes the *original* instruction either way) — compile the same
 * multi-occurrence source this test links against as a standalone
 * translation unit and check .rela.altinstructions directly: each
 * "774f" occurrence must resolve against its own distinct symbol, not
 * all three colliding on one shared "774". */
static int check_multi_occurrence_relocs(const char *rcc, const char *td, int pid)
{
    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_pca_multi_%d.c", td, pid);
    snprintf(objf, sizeof(objf), "%s/test_pca_multi_%d.o", td, pid);

    static const char src[] =
        "#define HWEIGHT32_ASM(w, res) \\\n"
        "    asm volatile ( \\\n"
        "        \"771:\\n\\tcall __sw_hweight32\\n772:\\n\" \\\n"
        "        \".pushsection .altinstructions, \\\"aM\\\", @progbits, 14\\n\" \\\n"
        "        \" .long 771b - .\\n\" \\\n"
        "        \" .long 774f - .\\n\" \\\n"
        "        \" .4byte 3\\n\" \\\n"
        "        \" .byte 772b-771b\\n\" \\\n"
        "        \" .byte 775f-774f\\n\" \\\n"
        "        \".popsection\\n\" \\\n"
        "        \".pushsection .altinstr_replacement, \\\"ax\\\"\\n\" \\\n"
        "        \"774:\\n\\tpopcntl %[val], %[cnt]\\n775:\\n\" \\\n"
        "        \".popsection\\n\" \\\n"
        "        : [cnt] \"=a\" (res) \\\n"
        "        : [val] \"D\" (w))\n"
        "unsigned int __sw_hweight32(unsigned int w) { return 0; }\n"
        "int f1(unsigned int a) { unsigned int r; HWEIGHT32_ASM(a, r); return (int)r; }\n"
        "int f2(unsigned int a) { unsigned int r; HWEIGHT32_ASM(a, r); return (int)r; }\n"
        "int f3(unsigned int a) { unsigned int r; HWEIGHT32_ASM(a, r); return (int)r; }\n";

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 0; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -c -o %s %s " NULL_REDIRECT, rcc, objf, srcf);
    int rc = system(cmd);
    remove(srcf);
    if (rc != 0) {
        printf("FAIL: [multi_occurrence] compile failed (rc=%d)\n", rc);
        remove(objf);
        return 0;
    }

#ifdef _WIN32
    /* readelf can't parse a COFF .obj (Windows' native output format) —
     * the underlying fix is arch-independent, but this specific
     * verification method is ELF-only. The compile step above already
     * exercises the fix; skip the relocation-symbol inspection here. */
    remove(objf);
    return 1;
#else
    snprintf(cmd, sizeof(cmd), "readelf -r %s " NULL_REDIRECT, objf);
    FILE *p = popen(cmd, "r");
    if (!p) { printf("FAIL: [multi_occurrence] readelf failed\n"); remove(objf); return 0; }
    char out[8192];
    size_t n = fread(out, 1, sizeof(out) - 1, p);
    out[n] = '\0';
    pclose(p);
    remove(objf);

    /* Each occurrence's "774f" relocation must land on a distinct
     * ".Lrcc_num774.N" symbol. A bare (non-unique) " 774 + 0" symbol
     * name, or fewer than 3 distinct ".Lrcc_num774." symbols, means
     * they collided. */
    if (strstr(out, " 774 + 0")) {
        printf("FAIL: [multi_occurrence] found a non-unique bare \"774\" "
               "relocation symbol:\n%s\n", out);
        return 0;
    }
    int distinct = 0;
    const char *p2 = out;
    while ((p2 = strstr(p2, ".Lrcc_num774."))) {
        distinct++;
        p2 += strlen(".Lrcc_num774.");
    }
    if (distinct < 3) {
        printf("FAIL: [multi_occurrence] expected 3 distinct .Lrcc_num774.N "
               "symbols, found %d:\n%s\n", distinct, out);
        return 0;
    }
    return 1;
#endif
}

static int check_popcnt_encoding(const char *rcc, const char *td, int pid)
{
    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_pca_%d.c", td, pid);
    snprintf(objf, sizeof(objf), "%s/test_pca_%d.o", td, pid);

    static const char src[] =
        "int main(void) { __asm__ volatile(\"popcntl %edi, %eax\\n\\t\"); return 0; }\n";

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 0; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -c -o %s %s " NULL_REDIRECT, rcc, objf, srcf);
    int rc = system(cmd);
    remove(srcf);
    if (rc != 0) {
        printf("FAIL: [popcnt_encoding] compile failed (rc=%d)\n", rc);
        remove(objf);
        return 0;
    }

    snprintf(cmd, sizeof(cmd), "objdump -s -j .text %s " NULL_REDIRECT, objf);
    FILE *p = popen(cmd, "r");
    if (!p) { printf("FAIL: [popcnt_encoding] objdump failed\n"); remove(objf); return 0; }
    char out[4096];
    size_t n = fread(out, 1, sizeof(out) - 1, p);
    out[n] = '\0';
    pclose(p);
    remove(objf);

    /* popcnt %edi,%eax = F3 40 0F B8 C7 (the harmless REX.40 rcc emits is
     * fine); the bug produced "5f" (pop %rdi) instead. */
    if (!strstr(out, "f3400fb8c7") && !strstr(out, "f3 40 0f b8 c7")) {
        char collapsed[4096];
        size_t cn = 0;
        for (const char *s = out; *s && cn + 1 < sizeof(collapsed); s++)
            if (!isspace((unsigned char)*s)) collapsed[cn++] = *s;
        collapsed[cn] = '\0';
        if (!strstr(collapsed, "f3400fb8c7")) {
            printf("FAIL: [popcnt_encoding] expected popcnt encoding, got:\n%s\n", out);
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    int ok = 1;

    ok &= check_popcnt_encoding(rcc, td, pid);
    ok &= check_multi_occurrence_relocs(rcc, td, pid);

    /* Smoke test: this file's own three HWEIGHT32_ASM call sites must at
     * least compile, link, and run without crashing. */
    if (f1(0) != 0 || f2(0) != 0 || f3(0) != 0) {
        printf("FAIL: [smoke] unexpected result\n");
        ok = 0;
    }

    if (!ok) return 1;
    printf("OK popcnt mnemonic dispatch and multi-occurrence numeric "
           "local label resolution\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
