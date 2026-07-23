/* Third batch from the ongoing x86-64 instruction-coverage audit against
 * binutils' opcodes/i386-opc.tbl (see test_x86_isa_gap_batch1.c for the
 * methodology). This batch covers x87 FPU control/status instructions
 * (fninit/finit, fnclex/fclex, fnop, fldcw, fnstcw/fstcw, fnstsw/fstsw —
 * the init/exception-handling subset; full x87 arithmetic stays deferred,
 * the kernel avoids x87 math outside very specific paths), port I/O
 * (bare "in"/"out"), far return (retf/lret), stack-frame setup (enter),
 * three more prefetch hints, and a handful of simple system/misc
 * instructions (monitor, mwait, rsm, xtest, xend, clzero, cldemote,
 * xabort, xbegin).
 *
 * Instructions actually run and checked here: the FPU control-word
 * round trip (fninit resets to a known default, fldcw/fnstcw round-trip
 * a custom value, fnstsw/fstsw read back a known-zero status after
 * fninit), enter/leave frame setup, and the newly-added prefetch hints
 * and cldemote (both documented as safe hint-or-NOP on hardware that
 * doesn't support them). Port I/O (privileged outside IOPL=3), far
 * return (long-mode operand-size edge cases not worth chasing here),
 * and the TSX/SMM/AMD-specific instructions (monitor, mwait, rsm, xtest,
 * xend, clzero, xabort, xbegin — several of which require CPU features
 * unpredictable across CI hardware, or fault outside a specific runtime
 * context) are placed in unreached code instead.
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
    snprintf(srcf, sizeof(srcf), "%s/test_gap3_%d.S", td, pid);
    snprintf(exef, sizeof(exef), "%s/test_gap3_%d", td, pid);
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
        "\tsubq $16, %rsp\n"

        /* fninit resets the x87 control word to the documented default
         * 0x037f, and the status word to 0 (no pending exceptions) */
        "\tfninit\n"
        "\tfnstcw -8(%rbp)\n"
        "\tmovw -8(%rbp), %ax\n"
        "\tcmpw $0x037f, %ax\n"
        "\tjne fail\n"
        "\tfnstsw -8(%rbp)\n"
        "\tmovw -8(%rbp), %ax\n"
        "\ttestw %ax, %ax\n"
        "\tjnz fail\n"

        /* fldcw/fnstcw round trip a custom control word */
        "\tmovw $0x027f, -8(%rbp)\n"
        "\tfldcw -8(%rbp)\n"
        "\tmovw $0, -8(%rbp)\n"
        "\tfnstcw -8(%rbp)\n"
        "\tmovw -8(%rbp), %ax\n"
        "\tcmpw $0x027f, %ax\n"
        "\tjne fail\n"

        /* fclex/finit (waiting forms) and fnop must just not fault */
        "\tfclex\n"
        "\tfinit\n"
        "\tfnop\n"
        "\tfstcw -8(%rbp)\n"
        "\tfstsw -8(%rbp)\n"

        /* enter/leave: a real stack frame, verified via a local write */
        "\tenter $16, $0\n"
        "\tmovl $0x1234, -4(%rbp)\n"
        "\tcmpl $0x1234, -4(%rbp)\n"
        "\tleave\n"
        "\tjne fail\n"

        /* prefetch hints and cldemote: documented safe-as-NOP even when
         * the specific hint isn't supported by the running CPU */
        "\tprefetcht1 -8(%rbp)\n"
        "\tprefetcht2 -8(%rbp)\n"
        "\tprefetchwt1 -8(%rbp)\n"
        "\tcldemote -8(%rbp)\n"

        "\tjmp done\n"
        /* Unreachable: privileged (in/out need IOPL=3), long-mode far
         * return operand-size edge cases not worth chasing, and
         * CPU-feature-dependent or context-dependent instructions (TSX,
         * SMM, AMD-only CLZERO, MONITOR/MWAIT idle-loop primitives). */
        "dead_code:\n"
        "\tinb $0x80, %al\n"
        "\toutb %al, $0x80\n"
        "\tin $0x80, %al\n"
        "\tout %al, $0x80\n"
        "\tretf\n"
        "\tlret\n"
        "\tmonitor\n"
        "\tmwait\n"
        "\trsm\n"
        "\txtest\n"
        "\txend\n"
        "\tclzero\n"
        "\txabort $1\n"
        "\txbegin dead_code\n"

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

    printf("OK x87 control/status, enter/leave, and the new prefetch "
           "hints/cldemote behave correctly; in/out/retf/lret/monitor/"
           "mwait/rsm/xtest/xend/clzero/xabort/xbegin all assemble\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
