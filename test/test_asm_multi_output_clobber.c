/* Inline-asm codegen bug: a multi-output asm() using x86 fixed-register
 * constraints (e.g. "=a"/"=b"/"=c"/"=d" for cpuid) permanently lost one
 * output's value whenever *another* output's address happened to be
 * register-allocated into the very same physical register that this
 * operand's constraint names.
 *
 * Root cause: codegen.c saves/restores an output's address register
 * around the asm only to protect it from being clobbered *by the asm
 * itself* (cpuid always writes eax/ebx/ecx/edx). But since address
 * registers are allocated independently of each operand's own fixed-reg
 * constraint, operand j's address can land in the exact physical
 * register operand i's "=b" constraint targets. Popping operand j's
 * saved address back into %rbx after the asm executed silently
 * overwrote operand i's still-unread cpuid ebx result before the
 * store-back loop could read it -- with no diagnostic, just a wrong
 * value at runtime. Found building blosc2 (blosc_get_cpu_features()'s
 * four-output cpuid queries via __builtin_cpu_supports()).
 */
int main(void) {
#if !defined(__aarch64__) && !defined(_M_ARM64)
    unsigned a, b, c, d;
    __asm__("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(1), "c"(0));
    /* leaf 1 edx bit 26 is SSE2 -- universally present on any x86-64 CPU
     * (SSE2 is part of the baseline x86-64 ABI), so this is a portable,
     * deterministic assertion rather than depending on the exact bit
     * pattern of the host CPU. */
    if (!((d >> 26) & 1)) return 1;
    unsigned b_saved = b;

    /* The actual regression: a second cpuid call re-using eax/ebx/ecx/edx
     * as outputs must not corrupt the first call's already-stored `b`
     * (its address is the one most likely to collide with the second
     * call's own "=b" constraint register during address save/restore). */
    unsigned a2, b2, c2, d2;
    __asm__("cpuid" : "=a"(a2), "=b"(b2), "=c"(c2), "=d"(d2) : "a"(0), "c"(0));
    if (b != b_saved) return 2; /* first call's stored output got clobbered */
    if (a2 == 0 && b2 == 0 && c2 == 0 && d2 == 0) return 3; /* leaf 0 always returns something */

#endif
    return 0;
}
