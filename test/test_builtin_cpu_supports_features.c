/* __builtin_cpu_supports() knew only sse2/avx/avx2/avx512f/avx512bw (later a
 * few more); every other GCC feature name (pclmul, aes, popcnt, sha, adx,
 * lzcnt, x86-64-v3, ...) silently returned 0, so runtime dispatch (nettle,
 * zlib-style PCLMUL/AES/SSE4.2 paths) never picked them. rcc now accepts the
 * full set GCC does (common/config/i386/i386-isas.h), with GCC's semantics:
 * OS support (XCR0) for the vector ISAs, and the x86-64 levels as
 * combinations.
 *
 * Also covers a codegen bug the macro exposed: the cpuid asm clobbered a live
 * %rbx (an earlier call argument), and its output capture could be clobbered
 * when the register pool was full, so `f(g(), cpu("a"), cpu("b"))` returned
 * garbage for some arguments, differently on every run.
 */
#include <stdio.h>

#if defined(__x86_64__) || defined(_M_X64)

static unsigned cpuid_reg(unsigned leaf, unsigned sub, int reg) {
    unsigned a = leaf, b = 0, c = sub, d = 0;
    __asm__("cpuid" : "+a"(a), "=b"(b), "+c"(c), "=d"(d));
    return reg == 'a' ? a : reg == 'b' ? b : reg == 'c' ? c : d;
}

static int failed;

#define CHECK_BIT(name, leaf, sub, reg, bit)                                                       \
    do {                                                                                           \
        int want = (cpuid_reg(leaf, sub, reg) >> (bit)) & 1;                                       \
        int got = __builtin_cpu_supports(name) ? 1 : 0;                                            \
        if (got != want) {                                                                         \
            printf("FAIL: %s got %d want %d\n", name, got, want);                                  \
            failed = 1;                                                                            \
        }                                                                                          \
    } while (0)

/* Names whose answer is a plain CPUID bit, independent of XCR0. */
static void check_plain_bits(void) {
    unsigned max_leaf = cpuid_reg(0, 0, 'a');
    unsigned max_ext = cpuid_reg(0x80000000u, 0, 'a');
    CHECK_BIT("cmov", 1, 0, 'd', 15);
    CHECK_BIT("mmx", 1, 0, 'd', 23);
    CHECK_BIT("sse", 1, 0, 'd', 25);
    CHECK_BIT("sse2", 1, 0, 'd', 26);
    CHECK_BIT("cmpxchg8b", 1, 0, 'd', 8);
    CHECK_BIT("fxsave", 1, 0, 'd', 24);
    CHECK_BIT("sse3", 1, 0, 'c', 0);
    CHECK_BIT("pclmul", 1, 0, 'c', 1);
    CHECK_BIT("ssse3", 1, 0, 'c', 9);
    CHECK_BIT("cmpxchg16b", 1, 0, 'c', 13);
    CHECK_BIT("sse4.1", 1, 0, 'c', 19);
    CHECK_BIT("sse4.2", 1, 0, 'c', 20);
    CHECK_BIT("movbe", 1, 0, 'c', 22);
    CHECK_BIT("popcnt", 1, 0, 'c', 23);
    CHECK_BIT("aes", 1, 0, 'c', 25);
    CHECK_BIT("xsave", 1, 0, 'c', 26);
    CHECK_BIT("osxsave", 1, 0, 'c', 27);
    CHECK_BIT("rdrnd", 1, 0, 'c', 30);
    if (max_leaf >= 7) {
        CHECK_BIT("fsgsbase", 7, 0, 'b', 0);
        CHECK_BIT("bmi", 7, 0, 'b', 3);
        CHECK_BIT("bmi2", 7, 0, 'b', 8);
        CHECK_BIT("rdseed", 7, 0, 'b', 18);
        CHECK_BIT("adx", 7, 0, 'b', 19);
        CHECK_BIT("clflushopt", 7, 0, 'b', 23);
        CHECK_BIT("clwb", 7, 0, 'b', 24);
        CHECK_BIT("sha", 7, 0, 'b', 29);
        CHECK_BIT("gfni", 7, 0, 'c', 8);
        CHECK_BIT("rdpid", 7, 0, 'c', 22);
        CHECK_BIT("serialize", 7, 0, 'd', 14);
    }
    if (max_ext >= 0x80000001u) {
        CHECK_BIT("lahf_lm", 0x80000001u, 0, 'c', 0);
        CHECK_BIT("abm", 0x80000001u, 0, 'c', 5);
        CHECK_BIT("lzcnt", 0x80000001u, 0, 'c', 5);
        CHECK_BIT("sse4a", 0x80000001u, 0, 'c', 6);
        CHECK_BIT("prfchw", 0x80000001u, 0, 'c', 8);
        CHECK_BIT("lm", 0x80000001u, 0, 'd', 29);
    }
}

static int pick4(int a, int b, int c, int d) { return a * 10000 + b * 1000 + c * 10 + d; }
static int id(int x) { return x; }

/* Standalone result vs the same query evaluated inside a call whose other
 * arguments are live around it. */
#define STABLE(name)                                                                               \
    do {                                                                                           \
        int s = __builtin_cpu_supports(name) ? 1 : 0;                                              \
        int t = pick4(id(7), __builtin_cpu_supports(name) ? 1 : 0, id(9), __builtin_cpu_supports(name) ? 1 : 0); \
        if (t != 70000 + s * 1000 + 90 + s) {                                                      \
            printf("FAIL: %s unstable in call arguments: %d (alone %d)\n", name, t, s);            \
            failed = 1;                                                                            \
        }                                                                                          \
    } while (0)

static void check_stable(void) {
    STABLE("cmov"); STABLE("mmx"); STABLE("popcnt"); STABLE("sse"); STABLE("sse2");
    STABLE("sse3"); STABLE("ssse3"); STABLE("sse4.1"); STABLE("sse4.2"); STABLE("avx");
    STABLE("avx2"); STABLE("sse4a"); STABLE("fma4"); STABLE("xop"); STABLE("fma");
    STABLE("avx512f"); STABLE("bmi"); STABLE("bmi2"); STABLE("aes"); STABLE("pclmul");
    STABLE("avx512vl"); STABLE("avx512bw"); STABLE("avx512dq"); STABLE("avx512cd");
    STABLE("avx512vbmi"); STABLE("avx512ifma"); STABLE("avx512vpopcntdq"); STABLE("avx512vbmi2");
    STABLE("gfni"); STABLE("vpclmulqdq"); STABLE("avx512vnni"); STABLE("avx512bitalg");
    STABLE("avx512bf16"); STABLE("avx512vp2intersect"); STABLE("3dnow"); STABLE("3dnowp");
    STABLE("adx"); STABLE("abm"); STABLE("cldemote"); STABLE("clflushopt"); STABLE("clwb");
    STABLE("clzero"); STABLE("cmpxchg16b"); STABLE("cmpxchg8b"); STABLE("enqcmd"); STABLE("f16c");
    STABLE("fsgsbase"); STABLE("fxsave"); STABLE("hle"); STABLE("ibt"); STABLE("lahf_lm");
    STABLE("lm"); STABLE("lwp"); STABLE("lzcnt"); STABLE("movbe"); STABLE("movdir64b");
    STABLE("movdiri"); STABLE("mwaitx"); STABLE("osxsave"); STABLE("pconfig"); STABLE("pku");
    STABLE("prfchw"); STABLE("ptwrite"); STABLE("rdpid"); STABLE("rdrnd"); STABLE("rdseed");
    STABLE("rtm"); STABLE("serialize"); STABLE("sgx"); STABLE("sha"); STABLE("shstk");
    STABLE("tbm"); STABLE("tsxldtrk"); STABLE("vaes"); STABLE("waitpkg"); STABLE("wbnoinvd");
    STABLE("xsave"); STABLE("xsavec"); STABLE("xsaveopt"); STABLE("xsaves"); STABLE("amx-tile");
    STABLE("amx-int8"); STABLE("amx-bf16"); STABLE("uintr"); STABLE("hreset"); STABLE("kl");
    STABLE("aeskle"); STABLE("widekl"); STABLE("avxvnni"); STABLE("avx512fp16"); STABLE("x86-64");
    STABLE("x86-64-v2"); STABLE("x86-64-v3"); STABLE("x86-64-v4"); STABLE("avxifma");
    STABLE("avxvnniint8"); STABLE("avxneconvert"); STABLE("cmpccxadd"); STABLE("amx-fp16");
    STABLE("prefetchi"); STABLE("raoint"); STABLE("amx-complex"); STABLE("avxvnniint16");
    STABLE("sm3"); STABLE("sha512"); STABLE("sm4"); STABLE("apxf"); STABLE("usermsr");
    STABLE("avx10.1"); STABLE("avx10.2"); STABLE("amx-avx512"); STABLE("amx-tf32");
    STABLE("amx-fp8"); STABLE("movrs"); STABLE("amx-movrs");
}

#define IMPLIES(a, b)                                                                              \
    do {                                                                                           \
        if (__builtin_cpu_supports(a) && !__builtin_cpu_supports(b)) {                             \
            printf("FAIL: %s supported but %s not\n", a, b);                                       \
            failed = 1;                                                                            \
        }                                                                                          \
    } while (0)

static void check_levels(void) {
    __builtin_cpu_init();
    IMPLIES("x86-64", "sse2");
    IMPLIES("x86-64", "lm");
    IMPLIES("x86-64-v2", "x86-64");
    IMPLIES("x86-64-v2", "sse4.2");
    IMPLIES("x86-64-v2", "popcnt");
    IMPLIES("x86-64-v2", "cmpxchg16b");
    IMPLIES("x86-64-v2", "lahf_lm");
    IMPLIES("x86-64-v3", "x86-64-v2");
    IMPLIES("x86-64-v3", "avx2");
    IMPLIES("x86-64-v3", "bmi");
    IMPLIES("x86-64-v3", "bmi2");
    IMPLIES("x86-64-v3", "f16c");
    IMPLIES("x86-64-v3", "fma");
    IMPLIES("x86-64-v3", "lzcnt");
    IMPLIES("x86-64-v3", "movbe");
    IMPLIES("x86-64-v4", "x86-64-v3");
    IMPLIES("x86-64-v4", "avx512bw");
    IMPLIES("x86-64-v4", "avx512cd");
    IMPLIES("x86-64-v4", "avx512dq");
    IMPLIES("x86-64-v4", "avx512vl");
    IMPLIES("avx512f", "avx2");
    IMPLIES("avx2", "avx");
    /* Every x86-64 CPU runs this program. */
    if (!__builtin_cpu_supports("x86-64") || !__builtin_cpu_supports("sse2")) {
        printf("FAIL: x86-64 baseline not reported\n");
        failed = 1;
    }
}

int main(void) {
    check_plain_bits();
    check_stable();
    check_levels();
    if (failed) return 1;
    printf("OK\n");
    return 0;
}
#else
int main(void) {
    printf("OK (not x86-64)\n");
    return 0;
}
#endif
