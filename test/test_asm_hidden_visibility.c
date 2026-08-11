/* GAS's `.hidden`/`.protected`/`.internal` directives (ELF symbol
 * visibility, ELF64_Sym.st_other) were not recognized by rcc's
 * assembler at all -- silently falling through to a no-op, leaving
 * every symbol at the default STV_DEFAULT visibility regardless of what
 * the source asked for.
 *
 * This isn't cosmetic: GNU MP (libgmp)'s own hand-written x86-64 asm
 * marks internal-linkage-but-cross-object-file data tables (e.g.
 * `mpn_invert_limb_table`, referenced from a *different* .asm file via
 * a plain `%rip`-relative LEA) with its own `PROTECT()` macro, which
 * expands to `.hidden`. Without real STV_HIDDEN visibility recorded,
 * `ld -shared` rejected the resulting GLOBAL-bound-but-default-
 * visibility symbol's direct PC32 relocation outright: "relocation
 * R_X86_64_PC32 against symbol `...' can not be used when making a
 * shared object; recompile with -fPIC" -- a default-visibility GLOBAL
 * symbol could in principle be interposed by another shared object at
 * load time, which a bare PC-relative displacement can't express;
 * STV_HIDDEN tells the linker this symbol can never be interposed, so
 * the direct reference is safe despite the GLOBAL binding. Confirmed
 * via a real libgmp shared-library build.
 *
 * Fixed by adding ObjSym.visibility (obj.h, STV_DEFAULT/STV_INTERNAL/
 * STV_HIDDEN/STV_PROTECTED) and a `.hidden`/`.protected`/`.internal`
 * directive handler (src/asm.c) that sets it; elf_write.c now emits it
 * as the symbol's st_other byte instead of always writing 0.
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"
/* Returns 1 if objdump -t marks `sym`'s visibility as `want` (one of
 * "hidden"/"protected"/"internal", or "" for plain default visibility).
 * objdump -t prints non-default ELF symbol visibility as a literal
 * ".hidden "/".protected "/".internal " prefix folded directly into the
 * name column (e.g. "g  .data  00000000 .hidden mytab"), not as a
 * separate flag or a bracketed "HIDDEN" keyword -- confirmed identical
 * to real GNU as's own objdump -t output for the same source. */
static int compile_and_check_visibility(const char *rcc, const char *td, int pid,
                                        const char *tag, const char *src,
                                        const char *sym, const char *want) {
    char srcf[160], objf[160], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_ahv_%s_%d.s", td, tag, pid);
    snprintf(objf, sizeof(objf), "%s/test_ahv_%s_%d.o", td, tag, pid);

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: [%s] cannot write %s\n", tag, srcf); return 0; }
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


#ifdef _WIN32
    /* ELF symbol visibility (STV_HIDDEN/STV_PROTECTED/STV_INTERNAL) has
     * no PE/COFF equivalent at all -- this specific verification method
     * is ELF-only. The `.hidden`/`.protected`/`.internal` directives
     * still parse and the compile step above already exercises that;
     * matches test_asm_forward_local_label_binding.c's identical
     * _WIN32 skip for the same underlying reason. */
    (void)sym;
    (void)want;
    remove(objf);
    return 1;
#else
    snprintf(cmd, sizeof(cmd), "objdump -t %s " NULL_REDIRECT, objf);
    FILE *p = popen(cmd, "r");
    if (!p) { printf("FAIL: [%s] objdump failed\n", tag); remove(objf); return 0; }
    char out[4096];
    size_t n = fread(out, 1, sizeof(out) - 1, p);
    out[n] = '\0';
    pclose(p);
    remove(objf);

    /* Build the exact expected name-column text: "<prefix ><sym>" (no
     * prefix at all for default visibility), then require it to appear
     * as a whole token -- i.e. preceded by whitespace and followed by
     * end-of-line/whitespace -- so e.g. "int_fn" alone never falsely
     * matches ".internal int_fn"'s trailing token, and a default-
     * visibility check ("" prefix) doesn't match a *different*,
     * unrelated ".hidden other_fn" line containing the same bytes. */
    char needle[192];
    if (*want)
        snprintf(needle, sizeof(needle), ".%s %s", want, sym);
    else
        snprintf(needle, sizeof(needle), "%s", sym);
    size_t needlen = strlen(needle);
    for (const char *ln = out; *ln;) {
        const char *nl = strchr(ln, '\n');
        size_t linelen = nl ? (size_t)(nl - ln) : strlen(ln);
        const char *m = ln;
        while ((m = strstr(m, needle)) && m < ln + linelen) {
            char before = (m == ln) ? ' ' : m[-1];
            char after = m[needlen];
            if ((before == ' ' || before == '\t') &&
                (after == '\0' || after == '\n')) {
                /* For the default-visibility case, also reject a match
                 * whose name column actually carries a visibility
                 * prefix right before it (".hidden sym" etc. contains
                 * "sym" as a trailing substring too). */
                if (!*want) {
                    const char *dot = m >= ln + 9 ? m - 9 : ln;
                    if (!strncmp(dot, ".hidden", 7) || !strncmp(dot, ".internal", 9) ||
                        (m >= ln + 10 && !strncmp(m - 10, ".protected", 10))) {
                        m++;
                        continue;
                    }
                }
                return 1;
            }
            m++;
        }
        ln = nl ? nl + 1 : ln + linelen;
    }
    printf("FAIL: [%s] expected \"%s\" as a whole name-column token in "
           "objdump -t output:\n%s\n", tag, needle, out);
    return 0;
#endif
}


int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    int failures = 0;

    /* .hidden on an already-.globl'd symbol -- the GMP PROTECT() shape:
     * "GLOBL sym" + "PROTECT(sym)" (order GMP actually uses: .hidden
     * appears before the .globl/definition in its m4-expanded output,
     * but real GAS accepts either order). */
    {
        static const char src[] =
            "\t.text\n"
            "\t.globl user_fn\n"
            "user_fn:\n"
            "\tlea\tmytab(%rip), %rax\n"
            "\tret\n"
            "\t.data\n"
            "\t.hidden mytab\n"
            "\t.globl mytab\n"
            "mytab:\n"
            "\t.long 1, 2, 3, 4\n";
        if (!compile_and_check_visibility(rcc, td, pid, "hidden", src, "mytab", "hidden")) {
            printf("FAIL: .hidden did not produce STV_HIDDEN visibility "
                   "on mytab\n");
            failures++;
        }
    }

    /* .protected */
    {
        static const char src[] =
            "\t.text\n"
            "\t.globl prot_fn\n"
            "\t.protected prot_fn\n"
            "prot_fn:\n"
            "\tret\n";
        if (!compile_and_check_visibility(rcc, td, pid, "protected", src, "prot_fn", "protected")) {
            printf("FAIL: .protected did not produce STV_PROTECTED "
                   "visibility on prot_fn\n");
            failures++;
        }
    }

    /* .internal */
    {
        static const char src[] =
            "\t.text\n"
            "\t.globl int_fn\n"
            "\t.internal int_fn\n"
            "int_fn:\n"
            "\tret\n";
        if (!compile_and_check_visibility(rcc, td, pid, "internal", src, "int_fn", "internal")) {
            printf("FAIL: .internal did not produce STV_INTERNAL "
                   "visibility on int_fn\n");
            failures++;
        }
    }

    /* Regression guard: a plain .globl with no visibility directive at
     * all must stay at default visibility (no HIDDEN/PROTECTED/INTERNAL
     * marker), i.e. the fix must not affect symbols that never opt in. */
    {
        static const char src[] =
            "\t.text\n"
            "\t.globl plain_fn\n"
            "plain_fn:\n"
            "\tret\n";
        if (!compile_and_check_visibility(rcc, td, pid, "default", src, "plain_fn", "")) {
            printf("FAIL: plain_fn (no visibility directive) unexpectedly "
                   "shows non-default visibility\n");
            failures++;
        }
    }

    if (failures) {
        printf("FAIL: %d sub-case(s) failed\n", failures);
        return 1;
    }
    printf("OK .hidden/.protected/.internal set the correct ELF symbol "
           "visibility\n");
    return 0;
}
#else
int main(void) { return 0; }
#endif
