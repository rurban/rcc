/* pp_tokens_to_text() re-glues tokens with no separating space whenever they
 * were byte-adjacent in whatever buffer they were lexed from — needed
 * because the C lexer splits assembly's dot-containing directive/section
 * names into separate punctuation + identifier tokens (".section" lexes as
 * "." "section"). That adjacency check only ever compared raw source
 * pointers, which breaks the moment a *macro parameter* is substituted
 * immediately before a literal token from the macro's own body: the
 * substituted argument's spelling lives in the call site's buffer, the
 * following body token's spelling lives in the macro definition's buffer —
 * two unrelated locations that can never satisfy a pointer-adjacency check,
 * even when the macro body itself had zero whitespace between them.
 *
 * Found via a real Linux kernel build: arch/x86/entry/entry.S defines
 * write_ibpb via SYM_FUNC_START(write_ibpb), which macro-expands (through
 * include/linux/linkage.h's SYM_START -> SYM_ENTRY chain) to something
 * ending in "... name:" where `name` is a macro parameter. The
 * reconstructed text came out "... write_ibpb :" (a spurious space before
 * the colon), which doesn't parse as a label at all — the assembler read
 * "write_ibpb" as a bare mnemonic with ":" as its operand instead, silently
 * dropping the label definition. Every reference to write_ibpb (e.g. "call
 * write_ibpb" elsewhere in the same file) then targeted an undefined
 * symbol, and objtool's own instruction-stream validation broke in turn.
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifndef _WIN32
#include <sys/wait.h>
#endif
#include "test_common.h"

int main(void)
{
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();

    char srcf[128], exef[160], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_ppa_%d.S", td, pid);
    snprintf(exef, sizeof(exef), "%s/test_ppa_%d", td, pid);
#ifdef _WIN32
    strcat(exef, ".exe");
#endif

    /* Mirrors linkage.h's real macro chain (renamed, trimmed to the
     * minimum needed to reproduce): a label built by pasting a macro
     * parameter directly against a literal ":" with no space in the
     * #define's own text. Compile straight to a runnable executable and
     * check the exit code — a dropped label definition falls through to
     * two independent failure modes (link error, since "call my_func"
     * targets an undefined symbol; or, on the off chance it somehow still
     * links, main's own call never lands on the intended code) rather than
     * trying to inspect the object file's symbol table, whose textual
     * format (section names, column layout) differs across ELF/COFF/
     * Mach-O in ways unrelated to what's actually being tested here. */
    /* Mach-O requires a leading underscore on any C-visible symbol name —
     * the runtime startup code looks for "_main" specifically as the entry
     * point on Darwin, unlike ELF/COFF's bare "main". my_func itself is
     * only ever referenced from within this same file, so it doesn't need
     * the same treatment. */
#ifdef __APPLE__
#define TEST_MAIN_SYM "_main"
#else
#define TEST_MAIN_SYM "main"
#endif
    static const char src[] =
        "#define ASM_NL ;\n"
        "#define MY_ENTRY(name) .globl name ASM_NL name:\n"
        "#define MY_FUNC_START(name) MY_ENTRY(name)\n"
        ".text\n"
        "MY_FUNC_START(my_func)\n"
        "\tmovl $42, %eax\n"
        "\tret\n"
        ".globl " TEST_MAIN_SYM "\n"
        TEST_MAIN_SYM ":\n"
        "\tcall my_func\n"
        "\tret\n";

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return 1; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -o %s %s -nostdinc -D__ASSEMBLY__ " NULL_REDIRECT,
             rcc, exef, srcf);
    int rc = system(cmd);
    remove(srcf);
    if (rc != 0) {
        printf("FAIL: compile+link failed (rc=%d)\n", rc);
        remove(exef);
        return 1;
    }

    snprintf(cmd, sizeof(cmd), "%s " NULL_REDIRECT, exef);
    int status = system(cmd);
    remove(exef);
#ifndef _WIN32
    int exit_code = (status >= 0 && WIFEXITED(status)) ? WEXITSTATUS(status) : -1;
#else
    int exit_code = status; /* Windows system() returns the exit code directly */
#endif
    if (exit_code != 42) {
        printf("FAIL: expected exit code 42 (my_func's label survived "
               "expansion and main actually called it), got %d\n", exit_code);
        return 1;
    }

    printf("OK a label built from a macro parameter (\"name:\" with no "
           "space in the #define) stays a real label after expansion\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
