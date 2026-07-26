/* A pointer/integer-typed global initializer of the shape
 * "&global + CONST1 - CONST2" (an address, plus a constant, minus
 * another constant) must resolve via a real relocation with the net
 * addend -- not fail with "unsupported global initializer".
 *
 * Regression: extract_reloc()'s ND_ADD case already recognized
 * "label + const" / "const + label" on either side, but ND_SUB was
 * grouped with several purely-arithmetic operators (SHL, BITAND, DIV,
 * ...) whose shared fallback just calls eval_const_expr() on the whole
 * subtraction node -- which can't fold an address at all, so it always
 * failed the moment *either* side of a "-" held a label, even when the
 * right-hand side was a plain constant offset (a perfectly valid address
 * constant expression, same as "+"). Subtraction isn't commutative like
 * addition, so only "label - const" (not "const - label") is meaningful.
 *
 * Found via a real Linux kernel build: arch/x86/kernel/cpu/common.c's
 *   #define TOP_OF_INIT_STACK ((unsigned long)&init_stack + \
 *                              sizeof(init_stack) - \
 *                              TOP_OF_KERNEL_STACK_PADDING)
 *   DEFINE_PER_CPU_CACHE_HOT(unsigned long, cpu_current_top_of_stack) =
 *       TOP_OF_INIT_STACK;
 */
#include <stdint.h>

struct big {
    char buf[64];
};
static struct big blob;

#define PADDING 8

/* Integer-typed target (matches the real per-cpu case exactly, modulo
 * "unsigned long" vs uintptr_t -- the real kernel idiom is LP64-only
 * (Linux never targets LLP64), where "unsigned long" already is
 * pointer-width; uintptr_t keeps this test meaningful on LLP64 hosts
 * too, since the fix under test -- extract_reloc()'s ND_SUB handling --
 * isn't itself LP64-specific). */
uintptr_t top = (uintptr_t)&blob + sizeof(blob) - PADDING;

/* Pointer-typed target, and a chained "+const-const+const" shape. */
static char *ptop = (char *)&blob + sizeof(blob) - PADDING;
static char *ptop2 = (char *)&blob + 10 - 3 + 1;

int main(void) {
    if (top != (uintptr_t)((char *)&blob + sizeof(blob) - PADDING)) return 1;
    if (ptop != (char *)&blob + sizeof(blob) - PADDING) return 2;
    if (ptop2 != (char *)&blob + 8) return 3;
    return 0;
}
