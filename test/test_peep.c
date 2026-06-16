/* Verify that the inline peephole optimizer eliminates at least 2 lines
 * compared to -O0 (no peephole).
 *
 * Pattern 3 (jmp/b + label elimination): the break at the end of the last
 * switch case emits an unconditional branch to the end-label, which is the
 * very next thing emitted.  The peephole detects the jmp+label pair and
 * deletes the dead branch, shrinking the .s output by one line per switch.
 *
 * Three switch functions → 3 dead branches → line count must drop by ≥ 2.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Self-contained C source that reliably triggers peephole pattern 3 (dead
 * branch elimination) in every switch function's last case. */
static const char peep_src[] =
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

/* Pattern 5 ARM64 regression: ldr R,[base]; add R,R,d3; mov d3,R
 * where d3 == third source operand of add.
 * Buggy transform: ldr d3,[base]; add d3,d3,d3  (self-add destroys offset)
 * A global array initialised in a loop exercises this: the loop index
 * register and the computed element-address register alias. */
static const char peep5_src[] =
    "int g[4];\n"
    "void init(void) { int i; for (i=0;i<4;i++) g[i]=i+1; }\n"
    "int main(void) {\n"
    "    init();\n"
    "    if (g[0]!=1||g[1]!=2||g[2]!=3||g[3]!=4) return 1;\n"
    "    return 0;\n"
    "}\n";

static int count_lines(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    int n = 0, c;
    while ((c = fgetc(f)) != EOF)
        if (c == '\n') n++;
    fclose(f);
    return n;
}


static const char *find_rcc(void) {
    const char *env = getenv("RCC");
    if (env && access(env, X_OK) == 0) return env;
#ifdef _WIN32
    if (access("./rcc.exe", X_OK) == 0) return "./rcc.exe";
    return "rcc.exe";
#elif defined(__aarch64__)
    if (access("./rcc-arm64", X_OK) == 0) return "./rcc-arm64";
    if (access("./rcc", X_OK) == 0) return "./rcc";
    return "rcc";
#else
    if (access("./rcc", X_OK) == 0) return "./rcc";
    if (access("./rcc-arm64", X_OK) == 0) return "./rcc-arm64";
    return "rcc";
#endif
}

int main(void) {
    const char *rcc = find_rcc();
    int pid = (int)getpid();
    char src[80], asm_O0[80], asm_O1[80], cmd[256];
    snprintf(src,    sizeof(src),    "/tmp/test_peep_%d.c",    pid);
    snprintf(asm_O0, sizeof(asm_O0), "/tmp/test_peep_%d_O0.s", pid);
    snprintf(asm_O1, sizeof(asm_O1), "/tmp/test_peep_%d_O1.s", pid);

    FILE *f = fopen(src, "w");
    if (!f) { printf("FAIL: cannot write %s\n", src); return 1; }
    fputs(peep_src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -S -O0 -o %s %s 2>/dev/null", rcc, asm_O0, src);
    if (system(cmd) != 0) { printf("FAIL: %s -S -O0 failed\n", rcc); return 1; }

    snprintf(cmd, sizeof(cmd), "%s -S -o %s %s 2>/dev/null", rcc, asm_O1, src);
    if (system(cmd) != 0) { printf("FAIL: %s -S failed\n", rcc); return 1; }

    int n0 = count_lines(asm_O0);
    int n1 = count_lines(asm_O1);

    remove(src);
    remove(asm_O0);
    remove(asm_O1);

    if (n0 < 0 || n1 < 0) { printf("FAIL: cannot read assembly output\n"); return 1; }

    int merged = n0 - n1;
    if (merged < 2) {
        printf("FAIL: peephole merged only %d line(s) (need >=2), O0=%d O1=%d\n",
               merged, n0, n1);
        return 1;
    }

    /* Pattern 5 correctness: global array init must store distinct values */
    {
        char src5[80], bin5[80];
        snprintf(src5, sizeof(src5), "/tmp/test_peep5_%d.c", pid);
        snprintf(bin5, sizeof(bin5), "/tmp/test_peep5_%d", pid);
        FILE *f5 = fopen(src5, "w");
        if (!f5) {
            printf("FAIL: cannot write %s\n", src5);
            return 1;
        }
        fputs(peep5_src, f5);
        fclose(f5);
        snprintf(cmd, sizeof(cmd), "%s -o %s %s 2>/dev/null", rcc, bin5, src5);
        int rc = system(cmd);
        if (rc != 0) {
            remove(src5);
            printf("FAIL:peep5 compile\n");
            return 1;
        }
        rc = system(bin5);
        remove(src5);
        remove(bin5);
        if (rc != 0) {
            printf("FAIL: peep5 global-array init miscompile (pattern 5 ARM64 d3-clobber)\n");
            return 1;
        }
    }

    /* Pattern 2: store-load elimination.  A function that stores a value
     * to the stack and immediately reloads it from the same slot. */
    {
        static const char peep2_src[] =
            "long f(long a, long b) {\n"
            "    long tmp = a + b;\n"
            "    return tmp;\n"
            "}\n";
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
        /* Pattern 2 folds a stack reload into a register move.
         * On x86: movq -N(%%rbp),%%reg → mov %%reg,%%reg
         * On ARM64: ldr reg,[x29,#-N] → mov reg,reg
         * Count stack-relative loads; O1 should have fewer. */
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

    return 0;
}
