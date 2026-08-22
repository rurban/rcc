/* Two distinct inline-asm codegen bugs, both rooted in the SAME mistake:
 * confusing X86Reg's own physical-register numbering (RAX=0, RCX=1,
 * RDX=2, RBX=3, RSI=6, RDI=7) with the virtual-register bitmask
 * `used_regs` is indexed by (0..7 mapping to R10,R11,RBX,R12,R13,R14,
 * R15,RSI -- see codegen_asm.h's cg_x86_reg[]). A "b"(RBX) constraint's
 * physical value 3 was OR'd directly into `used_regs` as bit 3, which
 * protects virtual register 3 (R12) -- not virtual register 2, which is
 * the one that actually maps to RBX. RBX's real virtual slot stayed
 * unprotected and available for alloc_reg() to hand to something else.
 */

/* Bug 1: a "b"(RBX) input alongside a memory ("m") operand. codegen.c's
 * x86_reserved_mask correctly computed bit 3 (RBX's raw X86Reg value)
 * but wrongly OR'd it straight into `used_regs`, "reserving" virtual
 * register 3 (%r12) instead of virtual register 2 (the one %rbx
 * actually occupies). The unprotected %rbx slot was then handed to the
 * memory operand's own address computation (gen_addr() -> alloc_reg()),
 * which silently overwrote the "b" input's value before "mulq %rbx"
 * read it. Found reducing mbedtls's bignum_core.c Montgomery
 * multiplication (MULADDC_X1_CORE's "mulq %%rbx" with a "S"-constrained
 * memory pointer), which manifested as an infinite loop in
 * mbedtls_mpi_core_montmul's carry propagation once the corrupted
 * partial product started producing meaningless output.
 */
#include <stdint.h>

/* Bug 2: a multi-output asm() where the "op_saved" capture loop (which
 * reads every x86-physical-register output into a scratch vreg before
 * the address-register restore can clobber it) itself called
 * alloc_reg() for each scratch temp WITHOUT protecting the *other*
 * outputs' still-unread physical registers. alloc_reg() could then
 * (correctly, per the buggy wider reservation this fix replaced)
 * receive virtual register 2 (%rbx) as a scratch temp for capturing
 * the "=a" output, clobbering the "=b" output's still-unread cpuid
 * result before it was captured. Found via busybox's/mbedtls's own
 * cpuid_eax_ebx_ecx-style 4-output/3-matched-input pattern.
 */
#if !defined(__aarch64__) && !defined(_M_ARM64)
static void cpuid_4out(unsigned *eax, unsigned *ebx, unsigned *ecx, unsigned *edx) {
    __asm__("cpuid"
            : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
            : "0"(*eax), "1"(*ebx), "2"(*ecx));
}
#endif

int main(void) {
#if !defined(__aarch64__) && !defined(_M_ARM64)
    uint64_t d[2] = {0x5d81111e00000000ULL, 0};
    uint64_t s[1] = {0x5d81111e00000000ULL};
    uint64_t b = 0x3a488fdceULL;
    uint64_t c = 0;
    uint64_t *dp = d, *sp = s;

    __asm__(
        "xorq   %%r8, %%r8\n"
        "movq   (%%rsi), %%rax\n"
        "mulq   %%rbx\n"
        "addq   $8, %%rsi\n"
        "addq   %%rcx, %%rax\n"
        "movq   %%r8, %%rcx\n"
        "adcq   $0, %%rdx\n"
        "addq   %%rax, (%%rdi)\n"
        "adcq   %%rdx, %%rcx\n"
        "addq   $8, %%rdi\n"
        : "+c" (c), "+D" (dp), "+S" (sp), "+m" (*(uint64_t (*)[16]) dp)
        : "b" (b), "m" (*(const uint64_t (*)[16]) sp)
        : "rax", "rdx", "r8"
    );
    /* A clobbered "b" (multiplier) input multiplies by garbage instead
     * of 0x3a488fdce, producing a different, wrong product/carry. */
    if (c != 0x1549beb96ULL || d[0] != 0x79377d4200000000ULL || d[1] != 0)
        return 1;

    unsigned eax = 7, ebx = 0, ecx = 0, edx;
    cpuid_4out(&eax, &ebx, &ecx, &edx);
    /* leaf 7 subleaf 0 always sets at least one feature bit somewhere
     * on any real x86-64 CPU; ebx==0 specifically flags the "=a" output
     * clobbering %rbx before it was captured. */
    if (eax == 0 && ebx == 0 && ecx == 0 && edx == 0) return 2;
#endif
    return 0;
}
