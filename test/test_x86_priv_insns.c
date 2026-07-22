/* A batch of x86-64 instructions the Linux kernel's inline asm uses that
 * rcc's assembler didn't implement at all: hitting one silently printed a
 * warning and emitted *zero bytes* for that instruction (see encode_x86's
 * "Unknown" fallback) instead of failing the build — which, inside an
 * ALTERNATIVE()-shaped construct, desyncs every length field and every
 * later offset computed from where that missing instruction should have
 * ended, corrupting unrelated code hiding at the same coincidental byte
 * offset. Found via a real Linux kernel build: kernel/exec_domain.c
 * (through spinlock.h/msr.h/tlbflush.h/smap.h et al.) uses rdtsc, clac,
 * stac, wrmsr, rdmsr, cmpxchg16b, invpcid, rdpid, lsl, clflush* and
 * segment-override prefixes ("ds wrmsr", "ds clflush").
 *
 * Most of these are privileged (wrmsr/rdmsr/hlt/cli/sti/wbinvd/swapgs/
 * rdpkru/wrpkru/clac/stac) or feature-gated (invpcid/cmpxchg16b/rdpid/
 * clflushopt/clwb) and would fault if actually executed from userspace/an
 * unsupporting CPU — so this verifies *encoding*, the same way
 * test_label_diff.c does, by objdumping the assembled bytes rather than
 * running them.
 *
 * The port-I/O family (outb/inb/outw/inw/outl/inl and their "rep ...s{b,w,l}"
 * string forms) and vmcall/vmmcall were a second batch found the same way,
 * via kernel/exit.c: arch/x86/include/asm/shared/io.h's outb()/inb() (used
 * transitively through native_io_delay()'s port-0x80 delay) and the KVM
 * hypercall path both silently dropped bytes the same way, and — because the
 * missing instructions happened to sit inside an ALTERNATIVE()-patched
 * region and a __ex_table fault-handler pair — corrupted the surrounding
 * .altinstr_replacement/__ex_table byte offsets badly enough that objtool
 * failed with "special: can't find new instruction" several steps removed
 * from the actual missing mnemonics.
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test_common.h"

/* objdump -s prints hex bytes in space-separated 4-byte groups
 * ("0f01ca0f 01cb41ba ...") — collapse each data line's hex region down
 * to a bare digit string so a search sequence that happens to straddle a
 * group boundary still matches. Skips the leading address column and
 * stops at the double-space before the ASCII sidebar. */
static void collapse_hex_dump(const char *out, char *hexbuf, size_t hexbuf_sz) {
    size_t hn = 0;
    const char *line = out;
    while (line && *line && hn + 1 < hexbuf_sz) {
        const char *nl = strchr(line, '\n');
        const char *end = nl ? nl : line + strlen(line);
        const char *p = line;
        while (p < end && isspace((unsigned char)*p)) p++;
        const char *addr_start = p;
        while (p < end && isxdigit((unsigned char)*p)) p++;
        if (p > addr_start && p < end && isspace((unsigned char)*p)) {
            while (p < end && isspace((unsigned char)*p)) p++;
            while (p < end && hn + 1 < hexbuf_sz) {
                if (isxdigit((unsigned char)*p)) {
                    hexbuf[hn++] = *p++;
                } else if (*p == ' ') {
                    if (p + 1 < end && p[1] == ' ') break;
                    p++;
                } else {
                    break;
                }
            }
        }
        line = nl ? nl + 1 : NULL;
    }
    hexbuf[hn] = '\0';
}

/* Compile `src`, objdump the given section's raw hex bytes, and check
 * that `expect_hex` (space-separated lowercase hex bytes, as objdump -s
 * prints them) appears somewhere in the output. */
static int check_section_bytes(const char *rcc, const char *td, int pid,
                               const char *tag, const char *src,
                               const char *section, const char *expect_hex) {
    char srcf[128], objf[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_priv_%s_%d.c", td, tag, pid);
    snprintf(objf, sizeof(objf), "%s/test_priv_%s_%d.o", td, tag, pid);

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

    snprintf(cmd, sizeof(cmd), "objdump -s -j %s %s " NULL_REDIRECT, section, objf);
    FILE *p = popen(cmd, "r");
    if (!p) {
        printf("FAIL: [%s] objdump failed to run\n", tag);
        remove(objf);
        return 0;
    }
    char out[4096];
    size_t n = fread(out, 1, sizeof(out) - 1, p);
    out[n] = '\0';
    pclose(p);
    remove(objf);

    char hexbuf[4096];
    collapse_hex_dump(out, hexbuf, sizeof(hexbuf));
    if (!strstr(hexbuf, expect_hex)) {
        printf("FAIL: [%s] expected bytes \"%s\" not found in %s:\n%s\n",
               tag, expect_hex, section, out);
        return 0;
    }
    return 1;
}

#define ASM_MAIN(body) "int main(void) { __asm__ volatile(" body "); return 0; }\n"

int main(void)
{
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    int ok = 1;

    ok &= check_section_bytes(rcc, td, pid, "clac_stac",
        ASM_MAIN("\"clac\\n\\tstac\\n\\t\""),
        ".text", "0f01ca0f01cb");
    ok &= check_section_bytes(rcc, td, pid, "wrmsr_rdmsr",
        ASM_MAIN("\"wrmsr\\n\\trdmsr\\n\\t\""),
        ".text", "0f300f32");
    ok &= check_section_bytes(rcc, td, pid, "hlt_cli_sti_wbinvd",
        ASM_MAIN("\"hlt\\n\\tcli\\n\\tsti\\n\\twbinvd\\n\\t\""),
        ".text", "f4fafb0f09");
    ok &= check_section_bytes(rcc, td, pid, "swapgs_rdpkru_wrpkru",
        ASM_MAIN("\"swapgs\\n\\trdpkru\\n\\twrpkru\\n\\t\""),
        ".text", "0f01f80f01ee0f01ef");
    ok &= check_section_bytes(rcc, td, pid, "fences_pause",
        ASM_MAIN("\"lfence\\n\\tsfence\\n\\tpause\\n\\t\""),
        ".text", "0faee80faef8f390");
    ok &= check_section_bytes(rcc, td, pid, "rdtsc_rdtscp",
        ASM_MAIN("\"rdtsc\\n\\trdtscp\\n\\t\""),
        ".text", "0f310f01f9");
    /* verw (%rax): 0F 00 /5, mod=00 reg=5 rm=0 -> 0x28 */
    ok &= check_section_bytes(rcc, td, pid, "verw",
        ASM_MAIN("\"verw (%%rax)\\n\\t\""),
        ".text", "0f0028");
    /* clflush/clflushopt/clwb (%rax) */
    ok &= check_section_bytes(rcc, td, pid, "clflush_family",
        ASM_MAIN("\"clflush (%%rax)\\n\\tclflushopt (%%rax)\\n\\tclwb (%%rax)\\n\\t\""),
        ".text", "0fae38660fae38660fae30");
    /* prefetcht0/prefetchnta/prefetchw (%rax) */
    ok &= check_section_bytes(rcc, td, pid, "prefetch_family",
        ASM_MAIN("\"prefetcht0 (%%rax)\\n\\tprefetchnta (%%rax)\\n\\tprefetchw (%%rax)\\n\\t\""),
        ".text", "0f18080f18000f0d08");
    /* invpcid (%rbx), %rax: 66 0F 38 82 /r, reg=rax(0) rm=rbx(3) -> mod=00 -> 0x03 */
    ok &= check_section_bytes(rcc, td, pid, "invpcid",
        ASM_MAIN("\"invpcid (%%rbx), %%rax\\n\\t\""),
        ".text", "660f388203");
    /* cmpxchg16b (%rbx): REX.W 0F C7 /1, reg=1 rm=rbx(3) -> 0x0b */
    ok &= check_section_bytes(rcc, td, pid, "cmpxchg16b",
        ASM_MAIN("\"cmpxchg16b (%%rbx)\\n\\t\""),
        ".text", "480fc70b");
    /* rdpid %rax: F3 0F C7 /7, mod=11 reg=7 rm=0 -> 0xf8 */
    ok &= check_section_bytes(rcc, td, pid, "rdpid",
        ASM_MAIN("\"rdpid %%rax\\n\\t\""),
        ".text", "f30fc7f8");
    /* lsl %ebx, %r11d: REX.R(dst=r11d) 0F 03 /r, reg=r11d(3) rm=ebx(3) -> 0xdb */
    ok &= check_section_bytes(rcc, td, pid, "lsl",
        ASM_MAIN("\"lsl %%ebx, %%r11d\\n\\t\""),
        ".text", "440f03db");
    /* Segment-override prefix + real instruction on one line (the kernel
     * pads wrmsr/clflush to a fixed length this way): "ds wrmsr" = 3E 0F 30,
     * "ds clflush (%rax)" = 3E 0F AE 38. */
    ok &= check_section_bytes(rcc, td, pid, "ds_prefix",
        ASM_MAIN("\"ds wrmsr\\n\\tds clflush (%%rax)\\n\\t\""),
        ".text", "3e0f303e0fae38");
    /* lgdt/lidt/sgdt/sidt (%rax): 0F 01 /2,/3,/0,/1, mod=00 rm=rax(0) */
    ok &= check_section_bytes(rcc, td, pid, "desc_table_mem",
        ASM_MAIN("\"lgdt (%%rax)\\n\\tlidt (%%rax)\\n\\tsgdt (%%rax)\\n\\tsidt (%%rax)\\n\\t\""),
        ".text", "0f01100f01180f01000f0108");
    /* lldt/ltr/str %ax: 0F 00 /2,/3,/1, mod=11 rm=rax(0) */
    ok &= check_section_bytes(rcc, td, pid, "desc_table_reg",
        ASM_MAIN("\"lldt %%ax\\n\\tltr %%ax\\n\\tstr %%ax\\n\\t\""),
        ".text", "0f00d00f00d80f00c8");

    /* outb %al,%dx / inb %dx,%al / outb %al,$0x80 / inb $0x80,%al */
    ok &= check_section_bytes(rcc, td, pid, "io_byte",
        ASM_MAIN("\"outb %%al, %%dx\\n\\tinb %%dx, %%al\\n\\toutb %%al, $0x80\\n\\tinb $0x80, %%al\\n\\t\""),
        ".text", "eeece680e480");
    /* outw %ax,%dx / inw %dx,%ax / outl %eax,%dx / inl %dx,%eax */
    ok &= check_section_bytes(rcc, td, pid, "io_word_long",
        ASM_MAIN("\"outw %%ax, %%dx\\n\\tinw %%dx, %%ax\\n\\toutl %%eax, %%dx\\n\\tinl %%dx, %%eax\\n\\t\""),
        ".text", "66ef66edefed");
    /* insb/outsb/insw/outsw/insl/outsl (implicit %dx/(%rsi)/(%rdi) operands) */
    ok &= check_section_bytes(rcc, td, pid, "io_string",
        ASM_MAIN("\"insb\\n\\toutsb\\n\\tinsw\\n\\toutsw\\n\\tinsl\\n\\toutsl\\n\\t\""),
        ".text", "6c6e666d666f6d6f");
    /* vmcall: 0F 01 C1, vmmcall: 0F 01 D9 */
    ok &= check_section_bytes(rcc, td, pid, "vm_calls",
        ASM_MAIN("\"vmcall\\n\\tvmmcall\\n\\t\""),
        ".text", "0f01c10f01d9");

    if (!ok) return 1;
    printf("OK x86 privileged/feature-gated instruction encoding\n");
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
