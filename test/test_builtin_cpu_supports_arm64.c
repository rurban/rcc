/* __builtin_cpu_supports() on AArch64 (the ACLE Function-Multi-Versioning
 * names clang accepts; GCC does not implement the builtin there). rcc had
 * no definition at all on AArch64, so any user of it failed to link. On
 * Linux the answer is the AT_HWCAP/AT_HWCAP2 bit, cross-checked here against
 * the kernel's own <asm/hwcap.h> HWCAP and HWCAP2 constants.
 */
#include <stdio.h>

#if defined(__aarch64__) && defined(__linux__)
#include <sys/auxv.h>
#include <asm/hwcap.h>

static unsigned long h1, h2;
static int failed;

#define CHECK(name, h, cap)                                                                \
    do {                                                                                   \
        int want = (h & (cap)) ? 1 : 0, got = __builtin_cpu_supports(name) ? 1 : 0;        \
        if (got != want) {                                                                 \
            printf("FAIL: %s got %d want %d\n", name, got, want);                          \
            failed = 1;                                                                    \
        }                                                                                  \
    } while (0)
#define CHECK1(name, cap) CHECK(name, h1, cap)
#define CHECK2(name, cap) CHECK(name, h2, cap)

int main(void) {
    __builtin_cpu_init();
    h1 = getauxval(AT_HWCAP);
    h2 = getauxval(AT_HWCAP2);
    CHECK1("fp", HWCAP_FP);
    CHECK1("simd", HWCAP_ASIMD);
    CHECK1("aes", HWCAP_AES);
    CHECK1("sha2", HWCAP_SHA2);
    CHECK1("crc", HWCAP_CRC32);
    CHECK1("lse", HWCAP_ATOMICS);
    CHECK1("fp16", HWCAP_FPHP);
    CHECK1("rdm", HWCAP_ASIMDRDM);
    CHECK1("jscvt", HWCAP_JSCVT);
    CHECK1("fcma", HWCAP_FCMA);
    CHECK1("rcpc", HWCAP_LRCPC);
    CHECK1("dpb", HWCAP_DCPOP);
    CHECK1("sha3", HWCAP_SHA3);
    CHECK1("sm4", HWCAP_SM4);
    CHECK1("dotprod", HWCAP_ASIMDDP);
    CHECK1("sve", HWCAP_SVE);
    CHECK1("fp16fml", HWCAP_ASIMDFHM);
    CHECK1("dit", HWCAP_DIT);
    CHECK1("rcpc2", HWCAP_ILRCPC);
    CHECK1("flagm", HWCAP_FLAGM);
    CHECK1("ssbs", HWCAP_SSBS);
    CHECK1("sb", HWCAP_SB);
    CHECK2("dpb2", HWCAP2_DCPODP);
    CHECK2("sve2", HWCAP2_SVE2);
    CHECK2("sve2-aes", HWCAP2_SVEAES);
    CHECK2("sve2-bitperm", HWCAP2_SVEBITPERM);
    CHECK2("sve2-sha3", HWCAP2_SVESHA3);
    CHECK2("sve2-sm4", HWCAP2_SVESM4);
    CHECK2("flagm2", HWCAP2_FLAGM2);
    CHECK2("frintts", HWCAP2_FRINT);
    CHECK2("f32mm", HWCAP2_SVEF32MM);
    CHECK2("f64mm", HWCAP2_SVEF64MM);
    CHECK2("i8mm", HWCAP2_I8MM);
    CHECK2("bf16", HWCAP2_BF16);
    CHECK2("rng", HWCAP2_RNG);
    CHECK2("bti", HWCAP2_BTI);
    CHECK2("memtag", HWCAP2_MTE);
    CHECK2("sme", HWCAP2_SME);
    CHECK2("sme-i16i64", HWCAP2_SME_I16I64);
    CHECK2("sme-f64f64", HWCAP2_SME_F64F64);
    CHECK2("wfxt", HWCAP2_WFXT);
    CHECK2("cssc", HWCAP2_CSSC);
    CHECK2("sme2", HWCAP2_SME2);
    CHECK2("mops", HWCAP2_MOPS);
    CHECK2("rcpc3", HWCAP2_LRCPC3);
    /* Every ARMv8-A core has FP and Advanced SIMD. */
    if (!__builtin_cpu_supports("fp") || !__builtin_cpu_supports("simd")) {
        printf("FAIL: fp/simd must be supported\n");
        failed = 1;
    }
    /* The result must not depend on evaluation context (call arguments). */
    int a = __builtin_cpu_supports("aes");
    if (printf("%d\n", __builtin_cpu_supports("aes")) < 0 || a != ((h1 & HWCAP_AES) ? 1 : 0)) {
        printf("FAIL: aes inconsistent\n");
        failed = 1;
    }
    if (failed) return 1;
    printf("OK\n");
    return 0;
}
#else
int main(void) {
    printf("OK (not aarch64 linux)\n");
    return 0;
}
#endif
