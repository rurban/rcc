/* x86-64 LEA/memory addressing with a scaled index but NO base register
 * (GAS AT&T "disp(,index,scale)", e.g. "0x40(,%rcx,8)"): the SIB byte's
 * base field is set to the "no base" encoding (rm=101), but the index
 * and scale fields must still carry the caller's real index register and
 * scale -- computing address = disp + index*scale.
 *
 * Regression: emit_mem()'s base==X86_NOREG path unconditionally emitted
 * a SIB byte with index=100 ("no index"), completely discarding the
 * actual index/scale operands. "lea 0x40(,%rcx,8), %rcx" (rcx=3) always
 * computed 0x40 instead of 0x40 + 3*8 = 88 -- silently dropping the
 * index/scale term whenever a memory operand had no base register.
 *
 * Found via a real PHP build: Zend/zend_string.c's hand-written x86-64
 * zend_string_equal_val() uses exactly this instruction (with %1
 * substituted to a compiler-chosen index/destination register) to build
 * a bit-shift mask for its tail-byte string comparison. The bug made it
 * ignore trailing bytes past a string's real content only when those
 * bytes coincidentally matched between the two strings being compared,
 * which normally holds for freshly-zeroed memory but not for two
 * separately-heap-allocated zend_strings with unrelated trailing
 * padding -- corrupting equal-length string comparisons project-wide,
 * observed as PHP's own class table failing to find its "Exception"
 * class entry via hash lookup even though table iteration found it
 * (different comparison path), crashing PHP at module startup.
 */
#include <stdio.h>

int main(void)
{
    unsigned long x = 3;
    /* "%0" is both the index (inside the parens) and the destination
     * (outside them) -- exactly PHP's own template shape. */
    __asm__("lea 0x40(,%0,8), %0" : "+r"(x));
    if (x != 0x40 + 3 * 8) {
        printf("FAIL: got %lu, want %lu\n", x, 0x40UL + 3UL * 8UL);
        return 1;
    }

    /* Same addressing form but with distinct index/destination registers
     * (compiler is free to allocate them differently), and a scale other
     * than 8 to exercise the SIB scale field independently. */
    unsigned long idx = 5, out;
    __asm__("lea 0x10(,%1,4), %0" : "=r"(out) : "r"(idx));
    if (out != 0x10 + 5 * 4) {
        printf("FAIL: got %lu, want %lu\n", out, 0x10UL + 5UL * 4UL);
        return 2;
    }

    printf("OK lea disp(,index,scale) with no base register\n");
    return 0;
}
