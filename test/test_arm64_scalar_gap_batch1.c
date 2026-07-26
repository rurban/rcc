/* First batch of ARM64 scalar/GPR instruction-coverage fixes, plus a much
 * bigger bug this batch's own testing surfaced: standalone .S files run
 * through the C preprocessor's "asm-with-cpp" mode (needed so #include/
 * #define work inside real .S files), which treats a '#' that isn't the
 * first token on its line as GAS's own end-of-line comment marker — correct
 * for x86 AT&T syntax (which uses '$' for immediates, never '#'), but on
 * ARM64 '#' IS the immediate-value prefix ("movz x0, #1234", "add x0, x0,
 * #10"). Every '#imm' operand in any hand-written ARM64 .S file (or the
 * cross-compiler's own output) was silently truncated away as a "comment",
 * corrupting the operand rather than erroring — "add x0, x0, #10" silently
 * became "add x0, x0, " (missing operand). Fixed by gating that heuristic
 * on the same ARCH_ARM64 compile-time macro that already distinguishes
 * encode_arm64() from encode_x86() throughout this codebase.
 *
 * Also found and fixed while wiring up adc/sbc/movn/sbfx/ldrsb/ldrsh/
 * ldurb/ldurh/sturb/sturh/bti (existing encoder functions that were never
 * dispatched from their mnemonic text): ubfx/sbfx's width parameter had a
 * pre-existing off-by-one (extracted one bit more than requested), and
 * ldurb/ldurh/sturb/sturh were being silently mis-encoded as full-width
 * ldur/stur by the generic "ldur"/"stur" prefix dispatch, which computed
 * the byte/half flags but never used them.
 *
 * This test only does anything on a real (or qemu-emulated, via
 * run_tests_arm64) ARM64 target — find_rcc() picks "./rcc-arm64" (the
 * cross-compiler under test) when running under qemu-aarch64 emulation,
 * matching test_arm64_asm.c's own pattern; on a native x86_64 host running
 * the ordinary `make check-all`, it just reports "OK skipped".
 */
#include <stdio.h>
#ifdef __aarch64__
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "test_common.h"

int main(void)
{
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();

    char srcf[128], exef[160], cmd[1024];
    snprintf(srcf, sizeof(srcf), "%s/test_a64g1_%d.S", td, pid);
    snprintf(exef, sizeof(exef), "%s/test_a64g1_%d", td, pid);

    /* Mach-O (macOS's native format — and macOS-latest CI runners are real
     * Apple Silicon, so this path is exercised for real, not just cross-
     * compiled) requires a leading underscore on any C-visible symbol; the
     * runtime startup code looks for "_main" specifically as the entry
     * point, unlike ELF's bare "main". */
#ifdef __APPLE__
#define TEST_MAIN_SYM "_main"
#else
#define TEST_MAIN_SYM "main"
#endif
    static const char src[] =
        ".text\n"
        ".globl " TEST_MAIN_SYM "\n"
        TEST_MAIN_SYM ":\n"
        "\tstp x29, x30, [sp, #-16]!\n"

        /* The headline bug: a plain '#imm' arithmetic operand in a real
         * .S file. If this silently lost its immediate, x0 stays 0. */
        "\tmov x0, #0\n"
        "\tadd x0, x0, #10\n"
        "\tcmp x0, #10\n"
        "\tbne fail\n"

        /* movn: x0 = ~(5 << 0) = -6. CMP's immediate form is unsigned
         * imm12 only (no negative literals in real GAS), so compare via
         * CMN (x0 == -6  <=>  x0 + 6 == 0). */
        "\tmovn x0, #5\n"
        "\tcmn x0, #6\n"
        "\tbne fail\n"

        /* adc/adcs/sbc/sbcs: split a 128-bit add/sub across two 64-bit
         * halves and compare against doing it directly in one register
         * pair's worth of headroom (values chosen to force a carry/borrow
         * out of the low half). */
        "\tmov x0, #-1\n"        /* low = 0xffffffffffffffff */
        "\tmov x1, #1\n"         /* low2 = 1 -> carry out */
        "\tadds x0, x0, x1\n"    /* x0 = 0, carry set */
        "\tmov x2, #5\n"
        "\tmov x3, #3\n"
        "\tadcs x2, x2, x3\n"    /* x2 = 5+3+carry(1) = 9 */
        "\tcmp x2, #9\n"
        "\tbne fail\n"

        "\tmov x0, #0\n"
        "\tmov x1, #1\n"
        "\tsubs x0, x0, x1\n"    /* 0-1 borrows: x0=-1, carry clear */
        "\tmov x2, #10\n"
        "\tmov x3, #3\n"
        "\tsbcs x2, x2, x3\n"    /* x2 = 10-3-borrow(1) = 6 */
        "\tcmp x2, #6\n"
        "\tbne fail\n"

        /* sbfx: extract 8 signed bits starting at bit 4 from a value
         * whose bit 11 (top of that field) is set, so sign-extension is
         * actually exercised (result must come out negative). */
        "\tmov x1, #0xff0\n"     /* bits [11:4] = 0xff -> field is 0xff */
        "\tsbfx x0, x1, #4, #8\n" /* sign-extend 8-bit 0xff -> -1 */
        "\tcmn x0, #1\n"
        "\tbne fail\n"

        /* ldrsb/ldrsh round trip through a stack byte/halfword whose top
         * bit is set, verifying real sign-extension (not zero-extension,
         * which the pre-fix code silently substituted). */
        "\tmov x1, #-1\n"
        "\tstrb w1, [sp, #-16]\n"
        "\tldrsb x0, [sp, #-16]\n"
        "\tcmn x0, #1\n"
        "\tbne fail\n"
        "\tmov x1, #-1\n"
        "\tstrh w1, [sp, #-16]\n"
        "\tldrsh x0, [sp, #-16]\n"
        "\tcmn x0, #1\n"
        "\tbne fail\n"

        /* ldurb/ldurh/sturb/sturh round trip (must not silently become a
         * full-width ldur/stur, which would clobber neighboring bytes). */
        "\tmov x1, #0x55\n"
        "\tsturb w1, [sp, #-16]\n"
        "\tldurb w0, [sp, #-16]\n"
        "\tcmp x0, #0x55\n"
        "\tbne fail\n"
        "\tmov x1, #0x1234\n"
        "\tsturh w1, [sp, #-16]\n"
        "\tldurh w0, [sp, #-16]\n"
        "\tmov x2, #0x1234\n" /* imm12 can't hold 0x1234; compare via register */
        "\tcmp x0, x2\n"
        "\tbne fail\n"

        "\tbti c\n" /* just needs to assemble+execute as a landing pad */

        "\tmov x0, #42\n"
        "\tldp x29, x30, [sp], #16\n"
        "\tret\n"
        "fail:\n"
        "\tmov x0, #1\n"
        "\tldp x29, x30, [sp], #16\n"
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
    int exit_code = (status >= 0 && WIFEXITED(status)) ? WEXITSTATUS(status) : -1;

    if (exit_code != 42) {
        printf("FAIL: expected exit code 42 (all sub-checks passed), got %d\n", exit_code);
        return 1;
    }

    printf("OK arm64 '#imm' operands, adc/adcs/sbc/sbcs, movn, sbfx, "
           "ldrsb/ldrsh, ldurb/ldurh/sturb/sturh, bti c all correct\n");
    return 0;
}
#else
int main(void)
{
    printf("OK skipped arm64\n");
    return 0;
}
#endif
