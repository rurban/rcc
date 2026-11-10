/* x86 assembler gaps found via OpenSSL 3.5's BN mont reductions
 * (test/third_party/test_openssl, test/exptest rc=139, RIP=0):
 *
 * 1. CMOV with an explicit GAS size suffix ("cmovcq" == cmovc at 64-bit,
 *    "cmovgl" == cmovg at 32-bit): the condition string was passed to
 *    parse_x86_cc() with the suffix letter still attached ("cq", "gl"),
 *    which matches no condition code and silently fell through to the
 *    X86_E default -- every suffixed cmov assembled as CMOVZ, corrupting
 *    the stack alignment in x86_64-mont.s and the reduction scale loads
 *    in x86_64-mont5.s. The suffix is now stripped when the remainder is
 *    a complete condition code; the unsuffixed "cmovb" (below) and
 *    "cmovl" (less) must keep their final letter and still work.
 *
 * 2. Two-operand IMUL with a memory source ("imulq 32+8(%rsp),%rbx" in
 *    x86_64-mont5.s) and with an immediate: the dispatch in asm.c had no
 *    arm for either form and the instruction was silently dropped
 *    (return true without emitting), corrupting the 8x Montgomery
 *    reduction. x86_imul_rm (0F AF /r, mem) was added.
 *
 * 3. CMP with an immediate and a memory destination ("cmpb $0, 7(%rbp)",
 *    the K-constant-table loop terminator in OpenSSL's sha512/sha256 asm):
 *    the dispatch had no arm for the (imm, mem) form and the instruction
 *    was silently dropped, so the following "jne" tested stale flags and
 *    the 16-round loop ran until the K pointer walked off the table
 *    (segfault in sha_test / wrong digests). x86_cmp_im (0x80/0x81/0x83
 *    /op with the memory in the r/m field) was added.
 *
 * 4. BT with an immediate bit index ("bt $0x1c, %edx", the CPUID
 *    feature probe in x86_64cpuid.s): the dispatch had no immediate arm
 *    and fell into the register form with R(0) parsing the immediate as
 *    a garbage register -- every feature probe tested a random bit of a
 *    random register. x86_bt_ri (0F BA /4 ib) was added.
 *
 * 64-bit values use uint64_t, not "unsigned long": on Windows (LLP64)
 * "long" is 32-bit, so a 0x1111...UL constant widens the comparison to a
 * 64-bit value the 32-bit variable can never equal and "1UL << 63" is
 * undefined. (CI caught case 2 on windows-latest.)
 */
#include <stdint.h>

#if !defined(__aarch64__) && !defined(_M_ARM64)

static int check_cmov(void)
{
    /* The sub's destination must be a read-write "+r" operand: the asm
     * contract forbids clobbering a plain "r" input, and gcc -O2 reuses
     * the stale register value across asm blocks (it broke case 2). */
    /* cmovcq -- carry set: 5 - 9 borrows */
    {
        uint64_t r = 0x1111111111111111ULL, s = 42, d = 5;
        __asm__ __volatile__(
            "subq %3, %1\n\t"
            "cmovcq %2, %0\n\t"
            : "+r"(r), "+r"(d)
            : "r"(s), "r"(9ULL));
        if (r != 42) return 1;
    }
    /* cmovcq -- carry clear: 9 - 5 */
    {
        uint64_t r = 0x1111111111111111ULL, s = 42, d = 9;
        __asm__ __volatile__(
            "subq %3, %1\n\t"
            "cmovcq %2, %0\n\t"
            : "+r"(r), "+r"(d)
            : "r"(s), "r"(5ULL));
        if (r != 0x1111111111111111ULL) return 2;
    }
    /* cmovb (below) -- unsuffixed, CF set: 5 - 9 */
    {
        int r = 7, e = 10, f = 5, g = 9;
        __asm__ __volatile__(
            "cmpl %3, %2\n\t"
            "cmovb %1, %0\n\t"
            : "+r"(r)
            : "r"(e), "r"(f), "r"(g));
        if (r != 10) return 3;
    }
    /* cmovl (less) -- unsuffixed, -5 < 3 */
    {
        int r = 7, a = -5, b = 3;
        __asm__ __volatile__(
            "cmpl %2, %1\n\t"
            "cmovl %1, %0\n\t"
            : "+r"(r)
            : "r"(a), "r"(b));
        if (r != -5) return 4;
    }
    /* cmovgl -- 32-bit form (l suffix), signed greater: 100 > 10 */
    {
        unsigned int r = 0, src = 100, dst = 10;
        __asm__ __volatile__(
            "cmpl %2, %1\n\t"
            "cmovgl %1, %0\n\t"
            : "+r"(r)
            : "r"(src), "r"(dst));
        if (r != 100) return 5;
    }
    /* cmovgeq -- ge with q suffix: -5 > -10 */
    {
        uint64_t r = 0, src = 1000;
        int a = -5, b = -10;
        __asm__ __volatile__(
            "cmpl %3, %2\n\t"
            "cmovgeq %1, %0\n\t"
            : "+r"(r)
            : "r"(src), "r"(a), "r"(b));
        if (r != 1000) return 6;
    }
    /* cmovnzq -- nz with q suffix: 5 != 9 */
    {
        uint64_t r = 0, src = 77;
        int a = 5, b = 9;
        __asm__ __volatile__(
            "cmpl %3, %2\n\t"
            "cmovnzq %1, %0\n\t"
            : "+r"(r)
            : "r"(src), "r"(a), "r"(b));
        if (r != 77) return 7;
    }
    return 0;
}

static int check_imul(void)
{
    /* memory source: r = 6 * 7 */
    {
        int64_t r = 6, m = 7;
        __asm__ __volatile__("imulq %1, %0\n\t" : "+r"(r) : "m"(m));
        if (r != 42) return 1;
    }
    /* memory source, negative: r = 3 * -11 */
    {
        int64_t r = 3, m = -11;
        __asm__ __volatile__("imulq %1, %0\n\t" : "+r"(r) : "m"(m));
        if (r != -33) return 2;
    }
    /* 2-operand immediate form: r = -6 * -7 */
    {
        int64_t r = -6;
        __asm__ __volatile__("imulq $-7, %0\n\t" : "+r"(r));
        if (r != 42) return 3;
    }
    /* reg-reg sanity (pre-existing path) */
    {
        int64_t r = 2, s = 21;
        __asm__ __volatile__("imulq %1, %0\n\t" : "+r"(r) : "r"(s));
        if (r != 42) return 4;
    }
    return 0;
}

static int check_cmp_immem(void)
{
    /* cmpb $0, mem (0x80 /0): byte zero -> ZF set */
    {
        unsigned char arr[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        unsigned char taken = 255;
        __asm__ __volatile__("cmpb $0, %1\n\tsete %0\n\t" : "=r"(taken) : "m"(arr[7]));
        if (taken != 1) return 1;
    }
    /* cmpb $0, mem: byte nonzero -> ZF clear */
    {
        unsigned char arr[8] = {0, 0, 0, 0, 0, 0, 0, 1};
        unsigned char taken = 255;
        __asm__ __volatile__("cmpb $0, %1\n\tsete %0\n\t" : "=r"(taken) : "m"(arr[7]));
        if (taken != 0) return 2;
    }
    /* cmpl $5, mem (0x83 /3, disp8): equal -> ZF set */
    {
        int arr[16];
        arr[8] = 5;
        unsigned char taken = 255;
        __asm__ __volatile__("cmpl $5, %1\n\tsete %0\n\t" : "=r"(taken) : "m"(arr[8]));
        if (taken != 1) return 3;
    }
    /* cmpl $5, mem: not equal -> ZF clear */
    {
        int arr[16];
        arr[8] = 6;
        unsigned char taken = 255;
        __asm__ __volatile__("cmpl $5, %1\n\tsete %0\n\t" : "=r"(taken) : "m"(arr[8]));
        if (taken != 0) return 4;
    }
    /* cmpl $100000, mem (0x81 /1, imm32): equal -> ZF set */
    {
        int arr[16];
        arr[8] = 100000;
        unsigned char taken = 255;
        __asm__ __volatile__("cmpl $100000, %1\n\tsete %0\n\t" : "=r"(taken) : "m"(arr[8]));
        if (taken != 1) return 5;
    }
    return 0;
}

static int check_bt_immem(void)
{
    /* "c" pins %1 to ecx (rcc's generic "r" pinning is unreliable here);
     * "q" keeps %0 an 8-bit register for the setc; "+c"(v) in the output
     * list because bt clears the tested bit (clobbering a plain "c"
     * input would violate the asm contract). */
    /* bt $28, %ecx: bit 28 set -> CF=1 */
    {
        uint32_t v = 1u << 28;
        uint8_t cf = 0;
        __asm__ __volatile__("bt $28, %1\n\tsetc %0\n\t" : "=q"(cf), "+c"(v));
        if (cf != 1) return 1;
    }
    /* bt $28, %ecx: bit 28 clear -> CF=0 */
    {
        uint32_t v = 1u << 3;
        uint8_t cf = 1;
        __asm__ __volatile__("bt $28, %1\n\tsetc %0\n\t" : "=q"(cf), "+c"(v));
        if (cf != 0) return 2;
    }
    /* 64-bit form: btq $63, %rcx */
    {
        uint64_t v = 1ULL << 63;
        uint8_t cf = 0;
        __asm__ __volatile__("btq $63, %1\n\tsetc %0\n\t" : "=q"(cf), "+c"(v));
        if (cf != 1) return 3;
    }
    return 0;
}

#endif

int main(void)
{
#if !defined(__aarch64__) && !defined(_M_ARM64)
    int r = check_cmov();
    if (r) return r;
    r = check_imul();
    if (r) return 10 + r;
    r = check_cmp_immem();
    if (r) return 20 + r;
    r = check_bt_immem();
    if (r) return 30 + r;
#endif
    return 0;
}
