/* A local label (no `.globl`) that's *referenced* via a forward branch/
 * call/`%rip`-relative LEA before its own definition (e.g. `call foo` /
 * `jmp foo` / `lea foo(%rip), reg` appearing textually above `foo:`)
 * ended up bound STB_GLOBAL in the assembled object's symbol table
 * instead of the correct STB_LOCAL, even though the label was never
 * `.globl`'d anywhere.
 *
 * Root cause (src/asm.c): ensure_sym() -- used by every forward-reference
 * site (CALL/JMP to a not-yet-seen label, `SYMBOL(%rip)` LEA, ARM64
 * ADRP/ADR, `.quad SYM - .`, ...) -- creates the not-yet-defined symbol
 * with a *speculative* SB_GLOBAL binding, needed so a reference that
 * turns out to be genuinely external (never locally defined in this
 * translation unit) still produces a valid, linkable relocation. But
 * once the label *is* later defined locally via define_label(), nothing
 * ever downgraded that speculative guess back to SB_LOCAL: define_label()
 * only *upgraded* LOCAL->GLOBAL on an explicit `.globl`, never the
 * reverse. Every forward-referenced-then-locally-defined label was
 * therefore permanently, silently promoted to a real GLOBAL symbol.
 *
 * This is more than cosmetic: two same-named local labels (e.g. `Ltab`,
 * a common hand-written-asm helper-table name) defined identically in
 * two *different* translation units of the same static/shared library
 * link fine when both are truly local (no cross-TU naming collision,
 * ELF's whole point of STB_LOCAL) but fail with the linker's "multiple
 * definition of `Ltab'" once this bug wrongly promotes both to GLOBAL --
 * confirmed via a real GNU MP (libgmp) shared-library build, whose own
 * mpn/x86_64/*.asm files each define their own private `Ltab`/
 * `Laddmul_outer_N` labels this way.
 *
 * Fixed by adding ObjSym.bind_pinned (obj.h): true only once a *real*
 * `.globl`/`.weak` directive has set a symbol's binding explicitly.
 * define_label() now downgrades an unpinned SB_GLOBAL back to SB_LOCAL
 * when the label turns out to be defined without one, while a real
 * `.globl` (before or after the label) still pins it GLOBAL correctly.
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

/* Returns 1 if `objdump -t objf` shows `sym` bound LOCAL, 0 if GLOBAL/
 * WEAK/not found (mirrors other asm tests' `objdump -t` symbol-table
 * inspection, e.g. test_assembler_predefine.c / test_pcrel_paren_addend.c --
 * but this needs the *binding column*, not just presence, so it scans
 * the line containing `sym` as a whole token rather than a bare
 * substring search). */
static int compile_and_check_local(const char *rcc, const char *td, int pid,
                                    const char *tag, const char *src,
                                    const char *sym) {
    char srcf[160], objf[160], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_afllb_%s_%d.s", td, tag, pid);
    snprintf(objf, sizeof(objf), "%s/test_afllb_%s_%d.o", td, tag, pid);

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
    /* objdump -t's flag column encodes ELF STB_LOCAL/STB_GLOBAL
     * binding; PE/COFF (Windows' native object format) has no
     * equivalent binding concept in the same column shape, so this
     * specific verification method is ELF-only -- the underlying fix
     * (ObjSym.bind_pinned, obj.c/obj.h/asm.c) is format-independent and
     * the compile step above already exercises it, but there's no
     * portable way to read COFF's storage-class byte back through
     * objdump -t's differently-shaped output. Matches
     * test_asm_two_bugs_popcnt_alt.c / test_asm_goto_positional_label.c's
     * identical _WIN32 skip for their own ELF-specific relocation/
     * symbol-table inspection. */
    (void)sym;
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

    /* Find the line naming `sym` as a whole token (not a substring of a
     * longer name), then check its objdump -t flag column: "l" (local)
     * appears right after the address, "g"/"  " (global/default) for a
     * global/weak binding. objdump -t's flag field is fixed-width
     * "<addr> <flags> <sec> <size> <name>" -- the flags column's first
     * char is 'l' for local, 'g' for global, '!' for global+weak or ' '
     * otherwise; scan the whole line's leading run before the name. */
    size_t symlen = strlen(sym);
    for (const char *ln = out; *ln;) {
        const char *nl = strchr(ln, '\n');
        size_t linelen = nl ? (size_t)(nl - ln) : strlen(ln);
        const char *m = ln;
        while ((m = strstr(m, sym)) && m < ln + linelen) {
            char before = (m == ln) ? ' ' : m[-1];
            char after = m[symlen];
            if ((before == ' ' || before == '\t') &&
                (after == '\0' || after == '\n' || after == ' ' || after == '\t')) {
                /* objdump -t address field is 16 hex chars, then a
                 * space, then the flags field starts. Locate it
                 * robustly by finding the first space-delimited token
                 * after the leading address instead of assuming exact
                 * column widths (differs slightly between binutils
                 * versions). */
                const char *sp = strchr(ln, ' ');
                if (sp && sp < ln + linelen) {
                    int is_local = (sp[1] == 'l');
                    remove(objf);
                    return is_local;
                }
            }
            m++;
        }
        ln = nl ? nl + 1 : ln + linelen;
    }
    printf("FAIL: [%s] symbol %s not found in objdump -t output:\n%s\n", tag, sym, out);
    return 0;
#endif
}

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    int failures = 0;

    /* Case 1: `call Ltarget` (forward reference) before `Ltarget:`,
     * never .globl'd -- must end up LOCAL. */
    {
        static const char src[] =
            "\t.text\n"
            "\t.globl entry_call\n"
            "entry_call:\n"
            "\tcall\tLtarget\n"
            "\tret\n"
            "Ltarget:\n"
            "\tnop\n"
            "\tret\n";
        if (!compile_and_check_local(rcc, td, pid, "call", src, "Ltarget")) {
            printf("FAIL: forward-referenced `call`-target label Ltarget "
                   "is not bound LOCAL\n");
            failures++;
        }
    }

    /* Case 2: `lea Ltable(%rip), %rax` (forward reference) before
     * `Ltable:`, never .globl'd -- must end up LOCAL. This is the exact
     * shape GMP's mpn/x86_64/*.asm computed-dispatch code uses (`lea
     * Laddmul_outer_N(%rip), %r14`). */
    {
        static const char src[] =
            "\t.text\n"
            "\t.globl entry_lea\n"
            "entry_lea:\n"
            "\tlea\tLtable(%rip), %rax\n"
            "\tret\n"
            "\t.data\n"
            "Ltable:\n"
            "\t.long 1, 2, 3, 4\n";
        if (!compile_and_check_local(rcc, td, pid, "lea", src, "Ltable")) {
            printf("FAIL: forward-referenced `%%rip`-LEA-target label "
                   "Ltable is not bound LOCAL\n");
            failures++;
        }
    }

    /* Case 3: two *different* translation units each define their own
     * private, same-named `Ltab` local label referenced via a forward
     * `call` -- both must assemble to LOCAL bindings so linking them
     * together into one library never collides ("multiple definition of
     * `Ltab'"), the real libgmp shared-library build failure this fixed. */
    {
        static const char src_a[] =
            "\t.text\n"
            "\t.globl func_a\n"
            "func_a:\n"
            "\tcall\tLtab\n"
            "\tret\n"
            "Ltab:\n"
            "\tnop\n"
            "\tret\n";
        static const char src_b[] =
            "\t.text\n"
            "\t.globl func_b\n"
            "func_b:\n"
            "\tcall\tLtab\n"
            "\tret\n"
            "Ltab:\n"
            "\tnop\n"
            "\tret\n";
        int a_local = compile_and_check_local(rcc, td, pid, "multi_a", src_a, "Ltab");
        int b_local = compile_and_check_local(rcc, td, pid, "multi_b", src_b, "Ltab");
        if (!a_local || !b_local) {
            printf("FAIL: cross-TU same-named local label Ltab not LOCAL "
                   "in both objects (a=%d b=%d) -- would collide at link "
                   "time as GLOBAL\n", a_local, b_local);
            failures++;
        }
    }

    /* Case 4 (regression guard): a *real* `.globl` before the forward
     * reference must still pin the symbol GLOBAL -- the fix must not
     * blanket-downgrade every symbol. */
    {
        char srcf[160], objf[160], cmd[512];
        snprintf(srcf, sizeof(srcf), "%s/test_afllb_glob_%d.s", td, pid);
        snprintf(objf, sizeof(objf), "%s/test_afllb_glob_%d.o", td, pid);
        static const char src[] =
            "\t.text\n"
            "\t.globl entry_g\n"
            "\t.globl Gtarget\n"
            "entry_g:\n"
            "\tcall\tGtarget\n"
            "\tret\n"
            "Gtarget:\n"
            "\tnop\n"
            "\tret\n";
        FILE *f = fopen(srcf, "w");
        if (!f) { printf("FAIL: [globl] cannot write %s\n", srcf); return 1; }
        fputs(src, f);
        fclose(f);
        snprintf(cmd, sizeof(cmd), "%s -c -o %s %s " NULL_REDIRECT, rcc, objf, srcf);
        int rc = system(cmd);
        remove(srcf);
        if (rc != 0) {
            printf("FAIL: [globl] compile failed (rc=%d)\n", rc);
            remove(objf);
            failures++;
#ifndef _WIN32
        } else {
            snprintf(cmd, sizeof(cmd), "objdump -t %s " NULL_REDIRECT, objf);
            FILE *p = popen(cmd, "r");
            char out[2048];
            size_t n = p ? fread(out, 1, sizeof(out) - 1, p) : 0;
            if (p) pclose(p);
            out[n] = '\0';
            remove(objf);
            if (!strstr(out, "Gtarget") ||
                !(strstr(out, " g     ") || strstr(out, "GLOBAL") ||
                  strstr(out, "\tg\t"))) {
                /* objdump -t flag field for a plain global is literally
                 * "g" (lowercase) in the second column -- just require
                 * Gtarget's line to NOT be marked local ('l'). */
                const char *m = strstr(out, "Gtarget");
                int ok = 0;
                if (m) {
                    /* walk back to line start, then find flags col */
                    const char *ls = m;
                    while (ls > out && ls[-1] != '\n') ls--;
                    const char *sp = strchr(ls, ' ');
                    ok = sp && sp[1] != 'l';
                }
                if (!ok) {
                    printf("FAIL: [globl] explicit .globl Gtarget did not "
                           "stay GLOBAL:\n%s\n", out);
                    failures++;
                }
            }
#else
        } else {
            /* PE/COFF -- see compile_and_check_local()'s _WIN32 comment;
             * the compile step above already exercises the fix. */
            remove(objf);
#endif
        }
    }

    if (failures) {
        printf("FAIL: %d sub-case(s) failed\n", failures);
        return 1;
    }
    printf("OK forward-referenced local labels stay LOCAL; explicit "
           ".globl still pins GLOBAL\n");
    return 0;
}
#else
int main(void) { return 0; }
#endif
