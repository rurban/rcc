/* GAS's PC-relative-to-here idiom ("SYM - .") can carry an explicit
 * addend attached to the dot itself, grouped in parens: "SYM - (. + N)"
 * or "SYM - (. - N)" — distinct from "SYM + N - ." (addend attached to
 * the *symbol* side instead, already supported). rcc's .byte/.long/...
 * data-directive handler only recognized a bare trailing "- ." (the
 * character right before the final dot had to be the '-'); "- (. + N)"
 * ends in ')', not '.', so the whole pattern failed to match at all and
 * fell through to whatever generic (non-relocatable) expression handling
 * comes after — which silently emitted *zero bytes* for the entire
 * directive instead of erroring.
 *
 * Found via a real Linux kernel build: arch/x86/include/asm/
 * static_call.h's ARCH_DEFINE_STATIC_CALL_TRAMP macro encodes a raw jmp
 * rel32 as ".byte 0xe9; .long target - (. + 4)" — every static call
 * trampoline in the kernel is built this exact way. Losing the .long
 * entirely truncated every trampoline to just its leading opcode byte,
 * corrupting whatever code happened to sit right after it in .text.
 * objtool caught the fallout as "STT_FUNC at end of section" on the
 * *next* symbol over — a confusing, distant symptom for a truncated
 * jmp several bytes earlier.
 *
 * The "+ 4" here is exactly the same "4 bytes past the start of this
 * displacement field" PC32 reference point every other rel32 fixup in
 * this codebase already computes as a plain relocation addend — just
 * spelled out explicitly in the source instead of left implicit. Note
 * the sign flips: "SYM - (. + N)" needs addend -N (the whole
 * parenthesized group is subtracted), not +N.
 *
 * Test not working on cross or non-x86.
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

int main(void)
{
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    int ok = 1;

    /* Compile-only: verify the trampoline's declared size (via ". -
     * SYM", the same idiom this fix doesn't touch) matches its real
     * encoded length (5 bytes: e9 + rel32) — before the fix this was 1
     * (just the leading 0xe9; the .long emitted nothing at all). */
    {
        char srcf[128], objf[128], cmd[512];
        snprintf(srcf, sizeof(srcf), "%s/test_ppa_sz_%d.c", td, pid);
        snprintf(objf, sizeof(objf), "%s/test_ppa_sz_%d.o", td, pid);
        static const char src[] =
            "void real_target(void) {}\n"
            "asm(\".globl mytramp\\n\"\n"
            "    \"mytramp:\\n\"\n"
            "    \".byte 0xe9\\n\"\n"
            "    \".long real_target - (. + 4)\\n\"\n"
            "    \".size mytramp, . - mytramp\\n\");\n"
            "int main(void) { return 0; }\n";
        FILE *f = fopen(srcf, "w");
        if (!f) { printf("FAIL: cannot write %s\n", srcf); return 1; }
        fputs(src, f);
        fclose(f);
        snprintf(cmd, sizeof(cmd), "%s -c -o %s %s " NULL_REDIRECT, rcc, objf, srcf);
        int rc = system(cmd);
        remove(srcf);
        if (rc != 0) {
            printf("FAIL: [size] compile failed (rc=%d)\n", rc);
            remove(objf);
            return 2;
        }
        snprintf(cmd, sizeof(cmd), "objdump -t %s " NULL_REDIRECT, objf);
        FILE *p = popen(cmd, "r");
        char out[4096];
        size_t n = p ? fread(out, 1, sizeof(out) - 1, p) : 0;
        if (p) pclose(p);
        out[n] = '\0';
        remove(objf);
        /* objdump -t prints "<offset> <flags> <sec> <size> <name>" —
         * the trampoline's size field must be 5 (hex), not 1. PE/COFF has
         * no per-symbol size field at all (GNU as's ".size" is an ELF-only
         * concept), so objdump -t never prints one there; the [run] check
         * below verifies the encoded length end to end on every target. */
#ifndef _WIN32
        if (!strstr(out, "0000000000000005") || !strstr(out, "mytramp")) {
            printf("FAIL: [size] expected mytramp size 5, got:\n%s\n", out);
            ok = 0;
        }
#else
        if (!strstr(out, "mytramp")) {
            printf("FAIL: [size] mytramp symbol missing:\n%s\n", out);
            ok = 0;
        }
#endif
    }

    /* Full link + run: the trampoline must actually jump to the right
     * place at runtime, not just have plausible-looking bytes. */
    {
        char srcf[128], exef[128], cmd[512];
        snprintf(srcf, sizeof(srcf), "%s/test_ppa_run_%d.c", td, pid);
        snprintf(exef, sizeof(exef), "%s/test_ppa_run_%d", td, pid);
        static const char src[] =
            "int hit;\n"
            "void real_target(void) { hit = 42; }\n"
            "asm(\".globl mytramp\\n\"\n"
            "    \"mytramp:\\n\"\n"
            "    \".byte 0xe9\\n\"\n"
            "    \".long real_target - (. + 4)\\n\"\n"
            "    \".size mytramp, . - mytramp\\n\");\n"
            "void mytramp(void);\n"
            "int main(void) { mytramp(); return hit == 42 ? 0 : 1; }\n";
        FILE *f = fopen(srcf, "w");
        if (!f) { printf("FAIL: cannot write %s\n", srcf); return 3; }
        fputs(src, f);
        fclose(f);
        snprintf(cmd, sizeof(cmd), "%s -o %s %s " NULL_REDIRECT, rcc, exef, srcf);
        int rc = system(cmd);
        remove(srcf);
        if (rc != 0) {
            printf("FAIL: [run] compile/link failed (rc=%d)\n", rc);
            remove(exef);
            return 4;
        }
        snprintf(cmd, sizeof(cmd), "%s", exef);
        rc = system(cmd);
        remove(exef);
        if (rc != 0) {
            printf("FAIL: [run] trampoline did not jump to real_target "
                   "(exit=%d)\n", rc);
            ok = 0;
        }
    }

    if (!ok) return 1;
    printf("OK PC-relative-to-here with a parenthesized dot-addend\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
