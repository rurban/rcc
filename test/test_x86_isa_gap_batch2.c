/* Second batch from the ongoing x86-64 instruction-coverage audit against
 * binutils' opcodes/i386-opc.tbl (see test_x86_isa_gap_batch1.c for the
 * methodology). This batch covers descriptor-table/system instructions
 * (cmc, clts, invd, wbnoinvd, wait/fwait, xgetbv, xsetbv, serialize, verr,
 * lar, smsw, lmsw, sldt, ud0, ud1) and the FXSAVE/FXRSTOR/XSAVE family
 * (fxsave[64], fxrstor[64], xsave[64], xrstor[64], xsaveopt[64],
 * xsavec[64], xsaves[64], xrstors[64]) used by the kernel's FPU
 * context-switch code (arch/x86/kernel/fpu/core.c, fpu/internal.h's
 * inline-asm alternatives between the plain and "64" REX.W-forced forms).
 *
 * Instructions safe to execute in ordinary user-mode CI (not
 * privileged, don't depend on GDT/LDT contents, don't need a CPU feature
 * this test can't probe for) are actually run and checked. Privileged
 * instructions (clts, invd, wbnoinvd, xsetbv, lmsw — all CPL0-only),
 * deliberate #UD traps (ud0, ud1), and instructions whose correctness
 * depends on live GDT/LDT/XCR0 state this test has no portable way to
 * predict (verr, lar, and the full XSAVE-family beyond plain
 * fxsave/fxrstor) are placed in unreached code instead — proving the
 * assembler accepts and encodes them without ever running them.
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

    char srcf[128], exef[160], cmd[1024];
    snprintf(srcf, sizeof(srcf), "%s/test_gap2_%d.S", td, pid);
    snprintf(exef, sizeof(exef), "%s/test_gap2_%d", td, pid);
#ifdef _WIN32
    strcat(exef, ".exe");
#endif

#ifdef __APPLE__
#define TEST_MAIN_SYM "_main"
#else
#define TEST_MAIN_SYM "main"
#endif
    static const char src[] =
        ".text\n"
        ".globl " TEST_MAIN_SYM "\n"
        TEST_MAIN_SYM ":\n"
        "\tpush %rbp\n"
        "\tmovq %rsp, %rbp\n"

        /* cmc: flips CF */
        "\tstc\n"
        "\tcmc\n"
        "\tjc fail\n"
        "\tclc\n"
        "\tcmc\n"
        "\tjnc fail\n"

        /* wait/fwait: no pending FPU exception, must just fall through */
        "\twait\n"
        "\tfwait\n"

        /* xgetbv: XCR0 (ecx=0) always has bit0 (x87 state) set */
        "\txor %ecx, %ecx\n"
        "\txgetbv\n"
        "\ttestl $1, %eax\n"
        "\tjz fail\n"

        /* smsw: CR0 bit0 (PE, protected-mode enable) is always set on any
         * running x86-64 OS */
        "\tsmsw %ax\n"
        "\ttestw $1, %ax\n"
        "\tjz fail\n"

        /* sldt: just needs to execute without faulting; the LDTR selector
         * value itself (commonly 0, LDT unused) isn't asserted */
        "\tsldt %ax\n"

        /* fxsave/fxrstor: not privileged — any process may save/restore
         * its own FPU/SSE state. FXSAVE requires a 16-byte-aligned
         * 512-byte area; carve one out of the stack (avoids the separate,
         * already-known "leaq sym(%rip)" cross-section addressing gap
         * that a .bss buffer would hit). */
        "\tandq $-64, %rsp\n"
        "\tsubq $512, %rsp\n"
        "\tfxsave (%rsp)\n"
        "\tfxrstor (%rsp)\n"

        "\tjmp done\n"
        /* Unreachable: privileged instructions (would #GP in user mode),
         * deliberate #UD traps, and GDT/LDT/XCR0-dependent forms this test
         * has no portable way to predict the outcome of. */
        "dead_code:\n"
        "\tclts\n"
        "\tinvd\n"
        "\twbnoinvd\n"
        "\txsetbv\n"
        "\tlmsw %ax\n"
        "\tud0 %eax, %ecx\n"
        "\tud1 %eax, %ecx\n"
        "\tverr %ax\n"
        "\tlar %ax, %eax\n"
        "\tfxsave64 (%rsp)\n"
        "\tfxrstor64 (%rsp)\n"
        "\txsave (%rsp)\n"
        "\txsave64 (%rsp)\n"
        "\txrstor (%rsp)\n"
        "\txrstor64 (%rsp)\n"
        "\txsaveopt (%rsp)\n"
        "\txsaveopt64 (%rsp)\n"
        "\txsavec (%rsp)\n"
        "\txsavec64 (%rsp)\n"
        "\txsaves (%rsp)\n"
        "\txsaves64 (%rsp)\n"
        "\txrstors (%rsp)\n"
        "\txrstors64 (%rsp)\n"
        "\tserialize\n"

        "done:\n"
        "\tmovl $42, %eax\n"
        "\tmovq %rbp, %rsp\n"
        "\tpop %rbp\n"
        "\tret\n"
        "fail:\n"
        "\tmovl $1, %eax\n"
        "\tmovq %rbp, %rsp\n"
        "\tpop %rbp\n"
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
        printf("FAIL: expected exit code 42 (all sub-checks passed), got %d\n", exit_code);
        return 1;
    }

    printf("OK cmc/wait/xgetbv/smsw/sldt/fxsave/fxrstor behave correctly; "
           "clts/invd/wbnoinvd/xsetbv/lmsw/ud0/ud1/verr/lar and the full "
           "xsave-family all assemble\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
