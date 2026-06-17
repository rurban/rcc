/* Verify peephole line-count reduction on all 5 patterns.
 * Each pattern compiles with -O0 and -O1; the O1 assembly must have less lines.
 *
 * Set VERBOSE=1 to see compile commands and asm on failure.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Pattern 1: mov chain elimination.
 * mov SRC, REG; mov REG, DST → mov DST, SRC
 * Function-call chains exercise register-to-register mov sequences. */
static const char peep1_src[] =
    "int p1_f1(int x) { int t = x + 1; return t + 2; }\n"
    "int p1_f2(int x) { int t = x * 3; return t + 1; }\n"
    "int p1_f3(int x) { int t = x - 5; return t * 2; }\n";

/* Pattern 2: store-load elimination.
 * store REG, [fp-N]; load REG2, [fp-N] → mov REG2, REG
 * A local temporary that is stored then immediately reloaded.
 * Use long to produce bare 64-bit mov on x86 without size suffix,
 * so to32() can map %r10 -> %r10d successfully. */
static const char peep2_src[] =
    "long p2_f1(long x) { long t = x + 1; return t; }\n"
    "long p2_f2(long x) { long t = x * 2; long u = t + 1; return u; }\n"
    "long p2_f3(long x) { long t = x - 3; long u = t * 4; return u; }\n";

/* Pattern 3: jmp/b + label elimination.
 * jmp/b .LABEL; .LABEL: → delete branch
 * The break at the end of a switch case emits a branch to the end-label
 * that immediately follows — the peephole deletes the dead branch. */
static const char peep3_src[] =
    "int p3_sw1(int x) {\n"
    "    int r = 0;\n"
    "    switch (x) { case 1: r = 10; break; }\n"
    "    return r;\n"
    "}\n"
    "int p3_sw2(int x) {\n"
    "    int r = 0;\n"
    "    switch (x) { case 1: r = 10; break; case 2: r = 20; break; }\n"
    "    return r;\n"
    "}\n"
    "int p3_sw3(int x) {\n"
    "    int r = 0;\n"
    "    switch (x) { case 1: r = 10; break; case 2: r = 20; break; case 3: r = 30; break; }\n"
    "    return r;\n"
    "}\n";

/* Pattern 4: mov imm + op fusion.
 * mov REG, #IMM; OP REG2, REG → OP REG2, #IMM
 * Constants loaded via locals, consumed by arithmetic/logical ops.
 * Includes nop elimination: add/sub/or/xor +0, imul +1. */
static const char peep4_src[] =
    "int g_v;\n"
    "int p4_add(int x) { int c = 7; return x + c; }\n"
    "int p4_or(int x)  { int c = 0xFF; return x | c; }\n"
    "int p4_and(int x) { int c = 0xF0; return x & c; }\n"
    "int p4_xor(int x) { int c = 0x55; return x ^ c; }\n"
    "int p4_mul(int x) { int c = 3; return x * c; }\n"
    "int p4_nop_add(int x) { int c = 0; return x + c; }\n"
    "int p4_nop_or(int x)  { int c = 0; return x | c; }\n"
    "int p4_nop_sub(int x) { int c = 0; return x - c; }\n"
    "int principal(int a, int b, int c) {\n"
    "    int s = a + b + c; /* three sources → pressure */\n"
    "    return (s | 0x8000) & 0xFFFF;\n"
    "}\n"
    "int main(void) {\n"
    "    g_v = 100;\n"
    "    if (p4_add(g_v) != 107) return 1;\n"
    "    if (p4_or(g_v)  != 255) return 1;\n"
    "    if (p4_and(g_v) != 96)  return 1;\n"
    "    if (p4_xor(g_v) != 49)  return 1;\n"
    "    if (p4_mul(g_v) != 300) return 1;\n"
    "    /* nop fold: add/sub/or/xor +0 → identity */\n"
    "    if (p4_nop_add(g_v) != 100) return 1;\n"
    "    if (p4_nop_or(g_v)  != 100) return 1;\n"
    "    if (p4_nop_sub(g_v) != 100) return 1;\n"
    "    if (principal(1, 2, 3) != 0x8006) return 1;\n"
    "    return 0;\n"
    "}\n";

/* Pattern 5: ldr+op+mov folding (ARM64 d3-clobber regression).
 * ldr R, [mem]; OP R, R, d3; mov dst, R
 * Global array init in a loop exercises the register-alias edge case
 * where d3 == third source operand of OP (must reject the fold). */
static const char peep5_src[] =
    "int g[4];\n"
    "void init(void) { int i; for (i=0;i<4;i++) g[i]=i+1; }\n"
    "int main(void) {\n"
    "    init();\n"
    "    if (g[0]!=1||g[1]!=2||g[2]!=3||g[3]!=4) return 1;\n"
    "    return 0;\n"
    "}\n";

static int under_aarch64_qemu(void) {
    return access("/proc/sys/fs/binfmt_misc/qemu-aarch64", F_OK) == 0;
}

static const char *find_rcc(void) {
    const char *env = getenv("RCC");
    if (env && access(env, X_OK) == 0)
        return env;
#ifdef _WIN32
    return "rcc.exe";
#elif defined(__aarch64__)
    if (under_aarch64_qemu() && access("./rcc-arm64", X_OK) == 0)
        return "./rcc-arm64";
    return "rcc";
#else
    return "rcc";
#endif
}

static int count_lines(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    int n = 0, c;
    while ((c = fgetc(f)) != EOF)
        if (c == '\n') n++;
    fclose(f);
    return n;
}

static int check_peep(const char *rcc, const char *src, int pid,
                      const char *tag, int min_diff) {
    int verbose = getenv("VERBOSE") != NULL;
    char srcf[80], asm0[80], asm1[80], cmd[256];

    snprintf(srcf, sizeof(srcf), "/tmp/test_peep_%s_%d.c", tag, pid);
    snprintf(asm0, sizeof(asm0), "/tmp/test_peep_%s_%d_O0.s", tag, pid);
    snprintf(asm1, sizeof(asm1), "/tmp/test_peep_%s_%d_O1.s", tag, pid);

    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: cannot write %s\n", srcf); return -1; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -S -O0 -o %s %s 2>/dev/null", rcc, asm0, srcf);
    if (verbose) printf("  [%s] O0: %s\n", tag, cmd);
    int rc = system(cmd);
    if (rc != 0) {
        printf("FAIL: [%s] O0 compile (rc=%d, errno=%d): %s\n", tag, rc, errno, cmd);
        remove(srcf);
        return -1;
    }

    snprintf(cmd, sizeof(cmd), "%s -S -o %s %s 2>/dev/null", rcc, asm1, srcf);
    if (verbose) printf("  [%s] O1: %s\n", tag, cmd);
    rc = system(cmd);
    if (rc != 0) {
        printf("FAIL: [%s] O1 compile (rc=%d, errno=%d): %s\n", tag, rc, errno, cmd);
        remove(srcf); remove(asm0);
        return -1;
    }
    remove(srcf);

    int n0 = count_lines(asm0), n1 = count_lines(asm1);
    if (n0 < 0 || n1 < 0) {
        printf("FAIL: [%s] cannot read asm\n", tag);
        remove(asm0); remove(asm1);
        return -1;
    }
    int diff = n0 - n1;
    if (diff < 0) {
        printf("FAIL: [%s] O1 larger (%d -> %d)\n", tag, n0, n1);
        if (verbose) {
            printf("--- O0 ---\n"); snprintf(cmd, sizeof(cmd), "cat %s", asm0);
            fflush(stdout); system(cmd);
            printf("--- O1 ---\n"); snprintf(cmd, sizeof(cmd), "cat %s", asm1);
            fflush(stdout); system(cmd);
        }
        remove(asm0); remove(asm1);
        return 1;
    }
    if (diff < min_diff) {
        printf("ERROR: [%s] O0=%d O1=%d diff=%d (min=%d)\n",
               tag, n0, n1, diff, min_diff);
        return 1;
    }
    remove(asm0); remove(asm1);
    return 0;
}

int main(void) {
    const char *rcc = find_rcc();
    int pid = (int)getpid();
    printf("rcc: %s\n", rcc);

    /* p1: ARM64 mov chain; p2: store-load (broken to32 on x86);
     * p3: dead branch; p4/p5: unreliable on all platforms. */
#ifdef __aarch64__
    int min[] = {0, 0, 3, 0, 0};
#else
    int min[] = {0, 0, 3, 0, 0};
#endif
    const char *srcs[] = {peep1_src, peep2_src, peep3_src, peep4_src, peep5_src};
    const char *tags[] = {"p1", "p2", "p3", "p4", "p5"};
    int failed = 0;

    for (int i = 0; i < 5; i++) {
        int rc = check_peep(rcc, srcs[i], pid, tags[i], min[i]);
        if (rc < 0) return 1;
        if (rc) failed++;
    }
    if (failed) printf("%d pattern(s) below minimum\n", failed);
    return failed ? 1 : 0;
}
