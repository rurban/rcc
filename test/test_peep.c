/* Verify the inline peephole optimizer across all five patterns.
 *
 * Patterns 1, 4, 5: compile+execute correctness tests.
 * Pattern 2: grep count of stack-relative loads (O1 vs O0).
 * Pattern 3: line count reduction (O1 vs O0, ≥2 lines).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* === Pattern source texts — ordered 1–5 === */

/* Pattern 1: mov chain elimination.
 * mov SRC, REG; mov REG, DST → mov DST, SRC
 * Function-call chains exercise register-to-register mov sequences. */
static const char peep1_src[] =
    "int g_a, g_b, g_c;\n"
    "int p1_f1(int x) { return x + 1; }\n"
    "int p1_f2(int x) { int t = p1_f1(x); return t * 2; }\n"
    "int p1_f3(int x) { int t = p1_f2(x); return t - 3; }\n"
    "int main(void) {\n"
    "    g_a = 10; g_b = 7; g_c = 20;\n"
    "    if (p1_f1(g_a) != 11) return 1;\n"
    "    if (p1_f2(g_b) != 16) return 1;\n"
    "    if (p1_f3(g_c) != 39) return 1;\n"
    "    return 0;\n"
    "}\n";

/* Pattern 2: store-load elimination.
 * store REG, [fp-N]; load REG2, [fp-N] → mov REG2, REG
 * A local temporary that is stored then immediately reloaded. */
static const char peep2_src[] =
    "long f(long a, long b) {\n"
    "    long tmp = a + b;\n"
    "    return tmp;\n"
    "}\n";

/* Pattern 3: jmp/b + label elimination.
 * jmp/b .LABEL; .LABEL: → delete branch
 * The break at the end of a switch case emits a branch to the end-label
 * that immediately follows — the peephole deletes the dead branch. */
static const char peep3_src[] =
    "int fib(int n) {\n"
    "    if (n <= 1) return n;\n"
    "    return fib(n-1) + fib(n-2);\n"
    "}\n"
    "int sw1(int x) {\n"
    "    int r = 0;\n"
    "    switch (x) {\n"
    "    case 1: r = 10; break;\n"
    "    }\n"
    "    return r;\n"
    "}\n"
    "int sw2(int x) {\n"
    "    int r = 0;\n"
    "    switch (x) {\n"
    "    case 1: r = 10; break;\n"
    "    case 2: r = 20; break;\n"
    "    }\n"
    "    return r;\n"
    "}\n"
    "int sw3(int x) {\n"
    "    int r = 0;\n"
    "    switch (x) {\n"
    "    case 1: r = 10; break;\n"
    "    case 2: r = 20; break;\n"
    "    case 3: r = 30; break;\n"
    "    }\n"
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

/* === Helpers === */

static int count_lines(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    int n = 0, c;
    while ((c = fgetc(f)) != EOF)
        if (c == '\n') n++;
    fclose(f);
    return n;
}

static int under_aarch64_qemu(void) {
    /* QEMU user-mode registers binfmt_misc handlers.
     * Check for aarch64 entry; if present the kernel transparently
     * invokes qemu-aarch64 for ARM64 ELF binaries. */
    return access("/proc/sys/fs/binfmt_misc/qemu-aarch64", F_OK) == 0;
}

static const char *find_rcc(void) {
    const char *env = getenv("RCC");
    if (env && access(env, X_OK) == 0) return env;
#ifdef _WIN32
    if (access("./rcc.exe", X_OK) == 0) return "./rcc.exe";
    return "rcc.exe";
#elif defined(__aarch64__)
    if (under_aarch64_qemu() && access("./rcc-arm64", X_OK) == 0) return "./rcc-arm64";
    if (access("./rcc", X_OK) == 0) return "./rcc";
    return "rcc";
#else
    /* x86_64: prefer native, only try arm64 cross-binary under QEMU */
    if (access("./rcc", X_OK) == 0) return "./rcc";
    return "rcc";
#endif
}

/* Write C source, compile with rcc, execute binary.  Returns exit code,
 * or -1 on compile failure.  Cleans up temp files.
 *
 * Set VERBOSE=1 in the environment to see compile/run commands and
 * stdout/stderr on failure.
 *
 * Under aarch64 QEMU user-mode, nested system() calls need
 * QEMU_LD_PREFIX set so that binfmt_misc can find the ARM64
 * dynamic linker.  If it's missing we try common sysroot paths. */
static int compile_and_run(const char *rcc, const char *src_text,
                           int pid, const char *tag) {
    int verbose = getenv("VERBOSE") != NULL;
    char src[80], bin[80], cmd[256];
    snprintf(src, sizeof(src), "/tmp/test_peep_%s_%d.c", tag, pid);
    snprintf(bin, sizeof(bin), "/tmp/test_peep_%s_%d",   tag, pid);
    FILE *f = fopen(src, "w");
    if (!f) { printf("FAIL: cannot write %s\n", src); return -1; }
    fputs(src_text, f);
    fclose(f);
#ifdef __aarch64__
    if (!getenv("QEMU_LD_PREFIX")) {
        static const char *sysroots[] = {
            "/usr/aarch64-linux-gnu",
            "/usr/aarch64-redhat-linux/sys-root/fc44",
            "/usr/aarch64-redhat-linux/sys-root/fc43",
        };
        for (size_t i = 0; i < sizeof(sysroots)/sizeof(sysroots[0]); i++) {
            char ld_path[256];
            snprintf(ld_path, sizeof(ld_path), "%s/lib/ld-linux-aarch64.so.1", sysroots[i]);
            if (access(ld_path, F_OK) == 0) {
                setenv("QEMU_LD_PREFIX", sysroots[i], 1);
                break;
            }
        }
    }
#endif
    /* redirect stderr only when non-verbose; on failure always show command */
    snprintf(cmd, sizeof(cmd), "%s -o %s %s %s",
             rcc, bin, src, verbose ? "" : "2>/dev/null");
    if (verbose) fprintf(stderr, "  [%s] compile: %s\n", tag, cmd);
    int rc = system(cmd);
    if (rc != 0) {
        fprintf(stderr, "  [%s] compile FAILED (rc=%d): %s\n", tag, rc, cmd);
        remove(src);
        return -1;
    }
    if (verbose) fprintf(stderr, "  [%s] run: %s\n", tag, bin);
    rc = system(bin);
    if (rc != 0) {
        fprintf(stderr, "  [%s] run FAILED (rc=%d): %s\n", tag, rc, bin);
        remove(src);
        remove(bin);
        return rc;
    }
    remove(src);
    remove(bin);
    return 0;
}

/* === Main — one block per pattern, ordered 1–5 === */

int main(void) {
    const char *rcc = find_rcc();
    int pid = (int)getpid();
    char cmd[256];
    int rc;

    /* Pattern 1: mov chain — compile + execute */
    rc = compile_and_run(rcc, peep1_src, pid, "p1");
    if (rc != 0) {
        printf("FAIL: peep1 mov-chain miscompile%s\n",
               rc < 0 ? " (compile)" : "");
        return 1;
    }

    /* Pattern 2: store-load elimination — count stack-relative loads */
    {
        char src2[80], asm2_O0[80], asm2_O1[80];
        snprintf(src2,  sizeof(src2),  "/tmp/test_peep2_%d.c",   pid);
        snprintf(asm2_O0, sizeof(asm2_O0), "/tmp/test_peep2_%d_O0.s", pid);
        snprintf(asm2_O1, sizeof(asm2_O1), "/tmp/test_peep2_%d_O1.s", pid);
        FILE *f2 = fopen(src2, "w");
        if (!f2) { printf("FAIL: cannot write %s\n", src2); return 1; }
        fputs(peep2_src, f2);
        fclose(f2);
        snprintf(cmd, sizeof(cmd), "%s -S -O0 -o %s %s 2>/dev/null", rcc, asm2_O0, src2);
        if (system(cmd) != 0) { remove(src2); printf("FAIL:peep2 O0 compile\n"); return 1; }
        snprintf(cmd, sizeof(cmd), "%s -S -o %s %s 2>/dev/null", rcc, asm2_O1, src2);
        if (system(cmd) != 0) { remove(src2); printf("FAIL:peep2 O1 compile\n"); return 1; }
        remove(src2);
#ifdef __aarch64__
        char *grep_pat = "ldr.*\\[x29.*#-";
#else
        char *grep_pat = "mov[lq].*-[0-9].*(%rbp),";
#endif
        char grep_cmd[256];
        snprintf(grep_cmd, sizeof(grep_cmd), "grep -c '%s' %s", grep_pat, asm2_O0);
        FILE *pp = popen(grep_cmd, "r");
        int n0_ld = 0;
        if (pp) { fscanf(pp, "%d", &n0_ld); pclose(pp); }
        snprintf(grep_cmd, sizeof(grep_cmd), "grep -c '%s' %s", grep_pat, asm2_O1);
        pp = popen(grep_cmd, "r");
        int n1_ld = 0;
        if (pp) { fscanf(pp, "%d", &n1_ld); pclose(pp); }
        remove(asm2_O0);
        remove(asm2_O1);
        if (n1_ld >= n0_ld) {
            printf("FAIL: peep2 store-load not eliminated (O0=%d O1=%d)\n", n0_ld, n1_ld);
            return 1;
        }
    }

    /* Pattern 3: dead branch elimination — line count reduction ≥2 */
    {
        char src3[80], asm3_O0[80], asm3_O1[80];
        snprintf(src3,    sizeof(src3),    "/tmp/test_peep3_%d.c",    pid);
        snprintf(asm3_O0, sizeof(asm3_O0), "/tmp/test_peep3_%d_O0.s", pid);
        snprintf(asm3_O1, sizeof(asm3_O1), "/tmp/test_peep3_%d_O1.s", pid);
        FILE *f3 = fopen(src3, "w");
        if (!f3) { printf("FAIL: cannot write %s\n", src3); return 1; }
        fputs(peep3_src, f3);
        fclose(f3);
        snprintf(cmd, sizeof(cmd), "%s -S -O0 -o %s %s 2>/dev/null", rcc, asm3_O0, src3);
        if (system(cmd) != 0) { printf("FAIL: %s -S -O0 failed\n", rcc); return 1; }
        snprintf(cmd, sizeof(cmd), "%s -S -o %s %s 2>/dev/null", rcc, asm3_O1, src3);
        if (system(cmd) != 0) { printf("FAIL: %s -S failed\n", rcc); return 1; }
        int n0 = count_lines(asm3_O0);
        int n1 = count_lines(asm3_O1);
        remove(src3);
        remove(asm3_O0);
        remove(asm3_O1);
        if (n0 < 0 || n1 < 0) { printf("FAIL: cannot read assembly output\n"); return 1; }
        int merged = n0 - n1;
        if (merged < 2) {
            printf("FAIL: peep3 merged only %d line(s) (need >=2), O0=%d O1=%d\n",
                   merged, n0, n1);
            return 1;
        }
    }

    /* Pattern 4: imm + op fusion — compile + execute */
    rc = compile_and_run(rcc, peep4_src, pid, "p4");
    if (rc != 0) {
        printf("FAIL: peep4 imm+op miscompile%s\n",
               rc < 0 ? " (compile)" : "");
        return 1;
    }

    /* Pattern 5: ldr+op+mov (ARM64 d3-clobber) — compile + execute */
    rc = compile_and_run(rcc, peep5_src, pid, "p5");
    if (rc != 0) {
        printf("FAIL: peep5 d3-clobber miscompile%s\n",
               rc < 0 ? " (compile)" : "");
        return 1;
    }

    return 0;
}
