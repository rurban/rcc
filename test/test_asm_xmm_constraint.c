/* Inline-asm "x"/"=x"/"+x" (XMM register class) constraints were
 * entirely unhandled in the x86-64 GNU inline-asm operand-binding
 * codegen: the constraint-matching switch only recognized the fixed
 * single-letter GP register constraints ("a"/"b"/"c"/"d"/"S"/"D") and
 * "m"/"i"/"n"; anything else -- including "x" -- fell through to the
 * plain "r" (GP register) path, which called alloc_reg() (an ordinary
 * integer virtual register) and substituted a GP register name (e.g.
 * "%eax") into the template. A real SSE instruction like
 * `__asm__("movaps %1, %0" : "=x"(dst) : "x"(src))` between two
 * `vector_size(16)` locals then fed the assembler garbage operand
 * text; the "movaps"/"movdqa"/etc. dispatch's parse_x86_xmm() falls
 * back to XMM0 for anything not literally "%xmmN", so the instruction
 * silently became a self-copy that never touched src or dst at all --
 * no error anywhere.
 *
 * Fixed by recognizing the "x" constraint and binding it to a scratch
 * register from xmm8-xmm15 (a range never touched by this compiler's
 * own vector codegen, which always keeps vector_size values in memory
 * and only ever uses xmm0/xmm1 as its own ad-hoc scratch), loading the
 * C-level vector value into it before the asm executes (for input/
 * read-write operands) and storing it back after (for output/
 * read-write operands) -- mirroring the existing "m"/"=r"/"+r" binding
 * shapes. Found while writing an earlier session's PSHUFLW/PSHUFHW
 * regression test; also exposed a second, separate pre-existing bug
 * along the way: "movaps"/"movups" had no exclusion from the generic
 * "mov"-prefix dispatch (same class of bug as MOVDQA/MOVDQU/MOVD/MOVQ
 * fixed in the prior session), so even a *raw* `.S`-file "movaps
 * %xmm9,%xmm8" -- unrelated to inline-asm -- silently mis-assembled
 * as "mov %rdi,%rdi".
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <string.h>

typedef long long v2di __attribute__((vector_size(16)));
typedef int v4si __attribute__((vector_size(16)));

/* "=x"/"x": plain register-to-register vector copy through an asm
 * instruction that only exists in the xmm domain (no scalar
 * equivalent), so a mis-bound GP-register substitution can't
 * accidentally "work" by coincidence. */
static int check_movaps_copy(void)
{
    v2di src = {42, 99};
    v2di dst;
    __asm__ __volatile__("movaps %1, %0" : "=x"(dst) : "x"(src));
    if (dst[0] != 42 || dst[1] != 99) return 1;
    return 0;
}

/* "+x": read-modify-write, exercises both the pre-asm load and the
 * post-asm store-back path for the same operand. */
static int check_paddd_rw(void)
{
    v4si acc = {5, 10, 15, 20};
    v4si addend = {3, 3, 3, 3};
    __asm__ __volatile__("paddd %1, %0" : "+x"(acc) : "x"(addend));
    if (acc[0] != 8 || acc[1] != 13 || acc[2] != 18 || acc[3] != 23)
        return 1;
    return 0;
}

/* A real SSE2 shuffle through "x" operands, matching the shape that
 * originally surfaced this bug. */
static int check_pshuflw(void)
{
    v2di p = {0x0004000300020001LL, 0x0008000700060005LL};
    v2di lo;
    __asm__ __volatile__("pshuflw $0x1b, %1, %0" : "=x"(lo) : "x"(p));
    unsigned short w[8];
    memcpy(w, &lo, sizeof(w));
    if (w[0] != 4 || w[1] != 3 || w[2] != 2 || w[3] != 1) return 1;
    return 0;
}

/* Multiple independent "x" operands live at once (exercises the
 * scratch-register counter allocating distinct xmm8/xmm9/... per
 * operand rather than colliding). */
static int check_multiple_operands(void)
{
    v4si a = {1, 2, 3, 4};
    v4si b = {10, 20, 30, 40};
    v4si ra, rb;
    __asm__ __volatile__(
        "movaps %2, %0\n\t"
        "movaps %3, %1\n\t"
        : "=x"(ra), "=x"(rb)
        : "x"(a), "x"(b));
    if (ra[0] != 1 || ra[3] != 4) return 1;
    if (rb[0] != 10 || rb[3] != 40) return 2;
    return 0;
}
#endif

int main(void)
{
#if defined(__x86_64__) || defined(_M_X64)
    int r = check_movaps_copy();
    if (r) return r;
    r = check_paddd_rw();
    if (r) return 10 + r;
    r = check_pshuflw();
    if (r) return 20 + r;
    r = check_multiple_operands();
    if (r) return 30 + r;
#endif
    return 0;
}
