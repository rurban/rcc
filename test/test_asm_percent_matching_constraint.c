/* GCC extended inline asm: a `%` prefix on an input operand's constraint
 * (e.g. `"%0"(x)`) marks it commutative with the following operand --
 * purely a hint the compiler may use to swap operand order for a better
 * instruction encoding. It does NOT change the constraint's own meaning:
 * `"%0"` is still a matching constraint tying this input to output
 * operand 0 (same register), exactly like plain `"0"(x)` -- the
 * assembler must load `x`'s value into that register before the asm
 * block runs.
 *
 * Regression: every place rcc skipped an operand constraint's leading
 * modifier characters (`=`, `+`, `&`) before checking for a matching-
 * constraint digit skipped `%` from the *allowed prefix* set at parse
 * time, but not before the *digit-detection* check itself -- so a `%0`
 * constraint's `%` was never stripped, `*c` was `%` (not a digit) at
 * the point of the digit test, and the matching constraint was silently
 * missed entirely. The input operand was then treated as an ordinary,
 * unrelated operand: nothing loaded the input's value into the shared
 * register before the asm ran, and the instruction operated on
 * whatever garbage happened to already be there.
 *
 * Found via a real PHP build: Zend/zend_multiply.h's hand-written
 * x86-64 zend_safe_address() (used by every ecalloc()/emalloc() size
 * computation) uses exactly this shape --
 * `__asm__("mulq %3\n\tadc $0,%1" : "=&a"(res), "=&d"(overflow) :
 * "%0"(res), "rm"(size))` -- to compute nmemb*size+offset with overflow
 * detection. Every single call always returned 0 instead of the real
 * product, because `%0`'s `mulq` multiplicand was never loaded from
 * `res` -- corrupting the size of every heap allocation project-wide
 * and immediately corrupting PHP's own memory manager.
 */
#include <stdio.h>

#if defined(__x86_64__) || defined(_M_X64)
int main(void)
{
    /* Exact shape of zend_safe_address()'s first branch: "%0" input
     * matching an early-clobber fixed-register output, feeding an
     * unsigned multiply. */
    unsigned long res = 7;
    unsigned long overflow = 0;

    __asm__("mulq %3\n\t"
            "adc $0,%1"
            : "=&a"(res), "=&d"(overflow)
            : "%0"(res), "rm"(6UL)
            : "cc");

    if (res != 42 || overflow != 0) {
        printf("FAIL: res=%lu overflow=%lu (want res=42 overflow=0)\n", res, overflow);
        return 1;
    }

    /* Same shape without early-clobber, to isolate the % handling from
     * the early-clobber attribute. */
    unsigned long res2 = 8, hi2;
    __asm__("mulq %3" : "=a"(res2), "=d"(hi2) : "%0"(res2), "rm"(9UL));
    if (res2 != 72 || hi2 != 0) {
        printf("FAIL: res2=%lu hi2=%lu (want res2=72 hi2=0)\n", res2, hi2);
        return 2;
    }

    printf("OK percent-prefixed matching constraint\n");
    return 0;
}
#else
int main(void) { return 0; }
#endif
