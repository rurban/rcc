/* Inline-asm x86 assembler gaps: ADCX/ADOX (ADX extension) and
 * STMXCSR/LDMXCSR were rejected with "error: unknown x86 instruction".
 *
 * ADCX/ADOX: x86_enc.c had no encoder at all; added x86_adcx_{rr,rm}/
 * x86_adox_{rr,rm} (66/F3 [REX.W] 0F 38 F6 /r) and wired them into
 * asm.c's dispatch *before* the ALU_OP("adc", ...) prefix match, which
 * otherwise silently swallowed "adcx" as a bare "adc" (strncmp prefix,
 * not exact match) and mis-encoded it as a 2-byte ADC. Also added
 * "adcx"/"adox" to the operand-size-suffix exemption list so the
 * 32-bit register forms ("%eax") aren't forced to 64-bit ("%rax")
 * width. Verified byte-for-byte against `as`/objdump output. Real
 * usage: OpenSSL/LibreSSL's bn/arch/amd64 bignum multiply loops (*.S).
 *
 * STMXCSR/LDMXCSR: the encoders (x86_stmxcsr_m/x86_ldmxcsr_m) already
 * existed for the __builtin_ia32_{st,ld}mxcsr compiler-intrinsic path
 * but were never wired into the GNU inline-asm mnemonic dispatch. Along
 * the way, fixed a spurious REX prefix byte both encoders emitted
 * whenever the memory operand's base/index register was RSP/RBP/RSI/
 * RDI (maybe_rex()'s B/X parameters there took the raw register number
 * rather than a needs-REX bool, so "(%rdi)" wrongly tripped it) --
 * harmless at runtime (REX with all-zero bits is architecturally a
 * no-op) but non-minimal versus real assemblers. Real usage: libgc's
 * conservative-GC stack scan, rvvm's soft-float fallback.
 */
#if !defined(__aarch64__) && !defined(_M_ARM64)

/* ADCX/ADOX arithmetic (the sum itself, not flag readback -- some AMD
 * Zen/Zen+ CPUs have a documented erratum where ADCX/ADOX's flag output
 * is unreliable even for gcc-compiled code, so only the deterministic
 * arithmetic result is checked here). */
static int check_adx(void)
{
    unsigned long a1 = -1UL, b1 = 2, r1;
    __asm__ __volatile__(
        "clc\n\t"
        "adcx %2, %0\n\t"
        : "=r"(r1)
        : "0"(a1), "r"(b1)
        : "cc");
    if (r1 != 1) return 1; /* 0xFFFFFFFFFFFFFFFF + 2, carry-in 0, mod 2^64 */

    unsigned int a2 = -1U, b2 = 2, r2;
    __asm__ __volatile__(
        "adox %2, %0\n\t"
        : "=r"(r2)
        : "0"(a2), "r"(b2)
        : "cc");
    if (r2 != 1) return 2;

    /* memory source operand form */
    unsigned long mem = 5, r3 = 10;
    __asm__ __volatile__(
        "clc\n\t"
        "adcx %1, %0\n\t"
        : "+r"(r3)
        : "m"(mem)
        : "cc");
    if (r3 != 15) return 3;

    return 0;
}

/* STMXCSR/LDMXCSR round trip through the mem-operand assembler forms. */
static int check_mxcsr(void)
{
    unsigned int mxcsr, mxcsr2;
    __asm__ __volatile__("stmxcsr %0" : "=m"(mxcsr));
    __asm__ __volatile__("ldmxcsr %0" : : "m"(mxcsr));
    __asm__ __volatile__("stmxcsr %0" : "=m"(mxcsr2));
    if (mxcsr != mxcsr2) return 1;
    /* default exception masks (bits 7-12) are set at process start */
    if ((mxcsr & 0x1f80) != 0x1f80) return 2;
    return 0;
}
#endif

int main(void)
{
#if !defined(__aarch64__) && !defined(_M_ARM64)
    int r = check_adx();
    if (r) return r;
    r = check_mxcsr();
    if (r) return 10 + r;
#endif
    return 0;
}
