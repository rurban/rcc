/* rcc's built-in x86-64 assembler only ever hand-implemented the subset of
 * mnemonics needed by the test suite and a handful of real kernel files —
 * a systematic audit against binutils' own opcode table (opcodes/i386-opc.tbl)
 * found 413 mnemonics from "full base ISA + kernel-relevant extensions" that
 * rcc's assembler rejected outright with "unknown x86 instruction". This is
 * the first batch of fixes from that audit: the sign-extension family (both
 * Intel and AT&T alt-spellings, e.g. cwde/cwtl), flag instructions (lahf/
 * sahf/clc/stc/std), CET landing pads (endbr32/endbr64 — present at the top
 * of essentially every function GCC emits once -fcf-protection is on, which
 * the kernel enables by default), breakpoint/trap opcodes (int3/int1),
 * syscall/sysenter/sysexit/sysret, rdrand/rdseed, crc32, lods, and the
 * repz/repnz rep-prefix spellings.
 *
 * Compiles a real function exercising each one (where safe to execute) and
 * checks the actual runtime effect — sign-extended values, flag state via
 * lahf/pushfq, a real CRC32C test vector cross-checked against a native
 * gcc-compiled build using the same instruction, a real repz/repnz string
 * scan, and a real syscall — rather than just checking that assembly
 * succeeds. A handful of instructions that are unsafe to execute outside
 * their real context (endbr32/endbr64 as mid-function bytes, int1/int3
 * traps, ud2b's deliberate #UD, sysenter/sysexit/sysret's ring-transition
 * semantics) are placed in unreachable code so the test still proves the
 * assembler accepts and encodes them without crashing the test process.
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

/* crc32l %ecx, %eax with eax=0, ecx=0x12345678 -> 0xfa745634, cross-checked
 * at test-write time against a native gcc build issuing the real hardware
 * instruction via inline asm (__asm__("crc32l %1,%0")), not a software model. */

int main(void)
{
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();

    char srcf[128], exef[160], cmd[1024];
    snprintf(srcf, sizeof(srcf), "%s/test_gap1_%d.S", td, pid);
    snprintf(exef, sizeof(exef), "%s/test_gap1_%d", td, pid);
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
        "\tpush %rbx\n"

        /* cbw/cbtw: al=0x80 -> ax=0xff80 */
        "\tmovb $0x80, %al\n"
        "\tcbw\n"
        "\tcmpw $0xff80, %ax\n"
        "\tjne fail\n"

        /* cwde/cwtl: ax=0x8000 -> eax=0xffff8000 */
        "\tmovw $0x8000, %ax\n"
        "\tcwde\n"
        "\tcmpl $0xffff8000, %eax\n"
        "\tjne fail\n"

        /* cdqe/cltq: eax=0x80000000 -> rax=0xffffffff80000000 */
        "\tmovl $0x80000000, %eax\n"
        "\tcdqe\n"
        "\tmovq $0xffffffff80000000, %rcx\n"
        "\tcmpq %rcx, %rax\n"
        "\tjne fail\n"

        /* cwd/cwtd: ax=0x8000 -> dx=0xffff */
        "\tmovw $0x8000, %ax\n"
        "\tcwd\n"
        "\tcmpw $0xffff, %dx\n"
        "\tjne fail\n"

        /* cltd (AT&T alias for cdq) */
        "\tmovl $0x80000000, %eax\n"
        "\tcltd\n"
        "\tcmpl $0xffffffff, %edx\n"
        "\tjne fail\n"

        /* cqto (AT&T alias for cqo) */
        "\tmovq $0x8000000000000000, %rax\n"
        "\tcqto\n"
        "\tcmpq $-1, %rdx\n"
        "\tjne fail\n"

        /* lahf: stc then lahf -> AH bit0 (CF) set, observable as bit 8 of
         * AX (rcc's assembler doesn't parse the legacy 8-bit high-byte
         * registers %ah/%bh/%ch/%dh at all yet — a separate, real gap this
         * test steers around rather than silently mis-testing). */
        "\tmovb $0, %al\n"
        "\tstc\n"
        "\tlahf\n"
        "\ttestw $0x100, %ax\n"
        "\tjz fail\n"

        "\tmovb $0, %al\n"
        "\tclc\n"
        "\tlahf\n"
        "\ttestw $0x100, %ax\n"
        "\tjnz fail\n"

        /* sahf: ah=1 (CF bit) -> CF set after sahf */
        "\tmovw $0x0100, %ax\n"
        "\tsahf\n"
        "\tjnc fail\n"

        /* std: direction flag (bit 10) observable via pushfq */
        "\tstd\n"
        "\tpushfq\n"
        "\tpopq %rax\n"
        "\ttestq $0x400, %rax\n"
        "\tjz fail\n"
        "\tcld\n"

        /* crc32l: real CRC32C computation, cross-checked value */
        "\txor %eax, %eax\n"
        "\tmovl $0x12345678, %ecx\n"
        "\tcrc32l %ecx, %eax\n"
        "\tcmpl $0xfa745634, %eax\n"
        "\tjne fail\n"

        /* syscall: getpid (__NR_getpid=39 on x86_64 Linux) — real syscall,
         * checked to actually return a positive pid rather than crashing. */
        "\tmovq $39, %rax\n"
        "\tsyscall\n"
        "\tcmpq $0, %rax\n"
        "\tjle fail\n"

        /* repz cmpsb / repnz scasb: two identical 4-byte buffers built on
         * the stack (sidesteps a separate, real gap this test steers around
         * rather than silently mis-testing: rcc's hand-written-.S assembler
         * doesn't yet emit a relocation for a RIP-relative reference to a
         * symbol in a different section, e.g. a .text "leaq foo(%rip),..."
         * where foo is defined in .data — parse_x86_mem() silently drops
         * the symbol name and encodes a bogus zero displacement instead). */
        "\tsubq $16, %rsp\n"
        "\tmovl $0x44434241, (%rsp)\n"
        "\tmovl $0x44434241, 8(%rsp)\n"
        "\tleaq (%rsp), %rsi\n"
        "\tleaq 8(%rsp), %rdi\n"
        "\tmovq $4, %rcx\n"
        "\trepz cmpsb\n"
        "\tjne fail_unwind\n"

        /* repnz scasb: scan the stack buffer ("ABCD") for a byte that
         * isn't present — the scan must run to completion (ZF=0 at end). */
        "\tleaq (%rsp), %rdi\n"
        "\tmovb $0xff, %al\n"
        "\tmovq $4, %rcx\n"
        "\trepnz scasb\n"
        "\tjz fail_unwind\n"
        "\taddq $16, %rsp\n"

        "\tjmp done\n"
        /* Unreachable: exercises instructions that are unsafe to actually
         * execute here (ring transitions, deliberate traps/#UD) — proves
         * the assembler accepts and encodes them without ever running them. */
        "dead_code:\n"
        "\tendbr32\n"
        "\tendbr64\n"
        "\tint3\n"
        "\tint1\n"
        "\tud2b\n"
        "\trdrand %eax\n"
        "\trdseed %eax\n"
        "\tsysenter\n"
        "\tsysexit\n"
        "\tsysret\n"

        "done:\n"
        "\tmovl $42, %eax\n"
        "\tpop %rbx\n"
        "\tret\n"
        "fail_unwind:\n"
        "\taddq $16, %rsp\n"
        "fail:\n"
        "\tmovl $1, %eax\n"
        "\tpop %rbx\n"
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

    printf("OK sign-extension family, flags, endbr32/64, int1/int3/ud2b, "
           "syscall family, rdrand/rdseed, crc32, repz/repnz all assemble "
           "and (where safely executable) behave correctly\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
