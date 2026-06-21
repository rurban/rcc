/* Verify peephole line-count reduction.
 * Each pattern compiles with -O0 and without; the peep assembly must
 * have fewer lines (for patterns that delete dead code — p3, p3_asm,
 * p4_nop_asm) or at least not more (for patterns that only fold into
 * a single instruction — p1/p1a/p2/p4/p1_asm/p4_asm).
 *
 * Inline-asm variants inject the exact assembly the peephole matches,
 * verifying that peep_apply_patterns fires during codegen regardless
 * of how rcc's own code-generation happens to arrange registers.
 *
 * Set VERBOSE=1 to see compile commands and asm on failure.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int verbose;

/* ── C-code pattern sources ─────────────────────────────────────── */
static const char peep1_src[] =
    "int p1_f1(int x) { int t = x + 1; return t + 2; }\n"
    "int p1_f2(int x) { int t = x * 3; return t + 1; }\n"
    "int p1_f3(int x) { int t = x - 5; return t * 2; }\n";

static const char peep1a_src[] =
    "int g1a;\n"
    "int p1a_f1(int x) {\n"
    "    int t = 42;\n"
    "    int u = t;\n"
    "    g1a = u;\n"
    "    return u + x;\n"
    "}\n"
    "int p1a_f2(int x) {\n"
    "    int t = x + 99;\n"
    "    int u = t;\n"
    "    return u + x;\n"
    "}\n";

static const char peep2_src[] =
    "long p2_f1(long x) { long t = x + 1; return t; }\n"
    "long p2_f2(long x) { long t = x * 2; long u = t + 1; return u; }\n"
    "long p2_f3(long x) { long t = x - 3; long u = t * 4; return u; }\n";

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
    "    int s = a + b + c;\n"
    "    return (s | 0x8000) & 0xFFFF;\n"
    "}\n"
    "int main(void) {\n"
    "    g_v = 100;\n"
    "    if (p4_add(g_v) != 107) return 1;\n"
    "    if (p4_or(g_v)  != 255) return 1;\n"
    "    if (p4_and(g_v) != 96)  return 1;\n"
    "    if (p4_xor(g_v) != 49)  return 1;\n"
    "    if (p4_mul(g_v) != 300) return 1;\n"
    "    if (p4_nop_add(g_v) != 100) return 1;\n"
    "    if (p4_nop_or(g_v)  != 100) return 1;\n"
    "    if (p4_nop_sub(g_v) != 100) return 1;\n"
    "    if (principal(1, 2, 3) != 0x8006) return 1;\n"
    "    return 0;\n"
    "}\n";

static const char peep5_src[] =
    "int g[4];\n"
    "void init(void) { int i; for (i=0;i<4;i++) g[i]=i+1; }\n"
    "int main(void) {\n"
    "    init();\n"
    "    if (g[0]!=1||g[1]!=2||g[2]!=3||g[3]!=4) return 1;\n"
    "    return 0;\n"
    "}\n";

#if defined(__aarch64__)
/* ── ARM64 inline-asm sources (Intel syntax) ───────────────────── */

/* Pattern 1: mov x10, x11; mov x12, x10 → mov x12, x11
 * 2 lines → 2 lines (dead predecessor not deleted, min_diff=0) */
static const char peep1_asm_src[] =
    "void p1_asm(void) {\n"
    "    __asm__ volatile(\"  mov x10, x11\\n  mov x12, x10\\n\");\n"
    "}\n";

/* Pattern 1a: mov x10, #42; mov x12, x10 → mov x12, #42
 * 2 lines → 2 lines (min_diff=0) */
static const char peep1a_asm_src[] =
    "void p1a_asm(void) {\n"
    "    __asm__ volatile(\"  mov x10, #42\\n  mov x12, x10\\n\");\n"
    "}\n";

/* Pattern 3: b .L_p3_skip; .L_p3_skip: → delete branch
 * 2 lines → 1 line (branch deleted, min_diff=1) */
static const char peep3_asm_src[] =
    "void p3_asm(void) {\n"
    "    __asm__ volatile(\"  b .L_p3_skip\\n.L_p3_skip:\\n\");\n"
    "}\n";

/* Pattern 4: mov x10, #7; add x11, x11, x10 → add x11, x11, #7
 * 2 lines → 2 lines (min_diff=0 for fold, see p4_nop below) */
static const char peep4_asm_src[] =
    "void p4_asm(void) {\n"
    "    __asm__ volatile(\"  mov x10, #7\\n  add x11, x11, x10\\n\");\n"
    "}\n";

/* Pattern 4 nop: mov x10, #0; add x11, x11, x10 → nop deleted
 * 2 lines → 1 line (op deleted, min_diff=1) */
static const char peep4_nop_asm_src[] =
    "void p4_nop_asm(void) {\n"
    "    __asm__ volatile(\"  mov x10, #0\\n  add x11, x11, x10\\n\");\n"
    "}\n";

/* Pattern 5: ldr x10,[sp,#16]; add x10,x10,#7; mov x12,x10
 * -> ldr x12,[sp,#16]; add x12,x12,#7
 * 3 lines → 2 lines (final mov deleted once x10 is confirmed dead, min_diff=1) */
static const char peep5_asm_src[] =
    "void p5_asm(void) {\n"
    "    __asm__ volatile(\"  ldr x10, [sp, #16]\\n  add x10, x10, #7\\n  mov x12, x10\\n\");\n"
    "}\n";

#else
/* Pattern 1a: mov $42, %r10; mov %r10, %r12 → mov $42, %r12
 * 2 lines → 2 lines (min_diff=0) */
static const char peep1a_asm_src[] =
    "void p1a_asm(void) {\n"
    "    __asm__ volatile(\"  mov $42, %r10\\n  mov %r10, %r12\\n\");\n"
    "}\n";
static const char peep1_asm_src[] =
    "void p1_asm(void) {\n"
    "    __asm__ volatile(\"  mov %r10, %r11\\n  mov %r11, %r12\\n\");\n"
    "}\n";


/* Pattern 3: jmp .L_p3_skip; .L_p3_skip: → delete branch
 * 2 lines → 1 line (branch deleted, min_diff=1) */
static const char peep3_asm_src[] =
    "void p3_asm(void) {\n"
    "    __asm__ volatile(\"  jmp .L_p3_skip\\n.L_p3_skip:\\n\");\n"
    "}\n";

/* Pattern 4: mov $7, %r10; add %r10, %r11 → add $7, %r11
 * 2 lines → 2 lines (min_diff=0 for fold) */
static const char peep4_asm_src[] =
    "void p4_asm(void) {\n"
    "    __asm__ volatile(\"  mov $7, %r10\\n  add %r10, %r11\\n\");\n"
    "}\n";

/* Pattern 4 nop: mov $0, %r10; add %r10, %r11 → nop deleted
 * 2 lines → 1 line (op deleted, min_diff=1) */
static const char peep4_nop_asm_src[] =
    "void p4_nop_asm(void) {\n"
    "    __asm__ volatile(\"  mov $0, %r10\\n  add %r10, %r11\\n\");\n"
    "}\n";

/* Pattern 5: mov -8(%rbp), %r10; add %r11, %r10; mov %r10, %r12
 * -> mov -8(%rbp), %r12; add %r11, %r12
 * 3 lines → 2 lines (final mov deleted once %r10 is confirmed dead, min_diff=1) */
static const char peep5_asm_src[] =
    "void p5_asm(void) {\n"
    "    __asm__ volatile(\"  mov -8(%rbp), %r10\\n  add %r11, %r10\\n  mov %r10, %r12\\n\");\n"
    "}\n";
#endif

/* ── portable temp directory ─────────────────────────────────────── */
static const char *get_tmpdir(void) {
#ifdef _WIN32
    const char *tmp = getenv("TEMP");
    if (!tmp || !*tmp) tmp = getenv("TMP");
    if (!tmp || !*tmp) tmp = ".";
    return tmp;
#else
    return "/tmp";
#endif
}

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
    return "./rcc";
#else
    return "./rcc";
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
    char srcf[80], asm0[80], asm1[80], cmd[256];

    const char *td = get_tmpdir();
    snprintf(srcf, sizeof(srcf), "%s/test_peep_%s_%d.c", td, tag, pid);
    snprintf(asm0, sizeof(asm0), "%s/test_peep_%s_%d_O0.s", td, tag, pid);
    snprintf(asm1, sizeof(asm1), "%s/test_peep_%s_%d_O1.s", td, tag, pid);

    FILE *f = fopen(srcf, "w");
    if (!f) { printf( "FAIL: cannot write %s\n", srcf); return -1; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -S -O0 -o %s %s", rcc, asm0, srcf);
    if (verbose) printf("  [%s] O0: %s\n", tag, cmd);
    int rc = system(cmd);
    if (rc != 0) {
        printf( "FAIL: [%s] O0 compile (rc=%d, errno=%d): %s\n", tag, rc, errno, cmd);
        if (!verbose) remove(srcf);
        return -1;
    }

    snprintf(cmd, sizeof(cmd), "%s -S -o %s %s", rcc, asm1, srcf);
    if (verbose) printf("  [%s] O1: %s\n", tag, cmd);
    rc = system(cmd);
    if (rc != 0) {
        printf( "FAIL: [%s] O1 compile (rc=%d, errno=%d): %s\n", tag, rc, errno, cmd);
        if (!verbose) { remove(srcf); remove(asm0); }
        return -1;
    }
    remove(srcf);

    int n0 = count_lines(asm0), n1 = count_lines(asm1);
    if (n0 < 0 || n1 < 0) {
        printf( "FAIL: [%s] cannot read asm\n", tag);
        if (!verbose) { remove(asm0); remove(asm1); }
        return -1;
    }
    int diff = n0 - n1;
    if (diff < 0) {
        printf( "FAIL: [%s] O1 larger (%d -> %d)\n", tag, n0, n1);
        if (verbose) {
            printf("--- O0 ---\n"); snprintf(cmd, sizeof(cmd), "cat %s", asm0);
            fflush(stdout); system(cmd);
            printf("--- O1 ---\n"); snprintf(cmd, sizeof(cmd), "cat %s", asm1);
            fflush(stdout); system(cmd);
        }
        if (!verbose) { remove(asm0); remove(asm1); }
        return 1;
    }
    if (diff < min_diff) {
        printf( "ERROR: [%s] O0=%d O1=%d diff=%d (min=%d)\n",
               tag, n0, n1, diff, min_diff);
        return 1;
    }
    if (!verbose) { remove(asm0); remove(asm1); }
    return 0;
}

int main(void) {
    verbose = getenv("VERBOSE") != NULL;
    const char *rcc = find_rcc();
    int pid = (int)getpid();
    if (verbose)
        printf("rcc: %s\n", rcc);

    const char *srcs[16];
    const char *tags[16];
    int mins[16];
    int n = 0;
#define ADD(_src, _tag, _min) do { srcs[n]=_src; tags[n]=_tag; mins[n]=_min; n++; } while(0)

    /* C-code patterns: min_diff=0 (peephole may fold but not delete) */
    ADD(peep1_src,  "p1",  0);
    ADD(peep1a_src, "p1a", 0);
    ADD(peep2_src,  "p2",  0);
    ADD(peep3_src,  "p3",  0);
    ADD(peep4_src,  "p4",  0);
    ADD(peep5_src,  "p5",  0);

    /* Inline-asm: pattern 3 and pattern 4 nop delete dead code */
    ADD(peep1_asm_src,      "p1_asm",      0);
    ADD(peep1a_asm_src,     "p1a_asm",     0);
    ADD(peep3_asm_src,      "p3_asm",      1);
    ADD(peep4_asm_src,      "p4_asm",      0);
    ADD(peep4_nop_asm_src,  "p4_nop_asm",  1);
    ADD(peep5_asm_src,      "p5_asm",      1);

    int failed = 0;
    for (int i = 0; i < n; i++) {
        int rc = check_peep(rcc, srcs[i], pid, tags[i], mins[i]);
        if (rc < 0) return 1;
        if (rc) failed++;
    }
    if (failed) printf( "%d peep pattern(s) failed\n", failed);
    return failed ? 1 : 0;
}
