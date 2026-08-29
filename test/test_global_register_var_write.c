/* GCC's "global register variable" extension
 * (https://gcc.gnu.org/onlinedocs/gcc/Global-Register-Variables.html):
 *   register T x __asm__("regname");
 * at file scope pins `x` to a physical hardware register for the whole
 * program -- no memory address, no linker symbol at all.
 *
 * Regression: rcc already supported READING such a variable
 * (LVar.is_global_reg / gen_global_reg_read() in codegen.c), but every
 * WRITE (plain assignment, compound assignment, ++/--) fell through to
 * the ordinary global-variable codegen path, which computes an
 * rip-relative address to a linker symbol -- one that was (correctly)
 * never emitted for a register variable, since it has no storage.
 * Assigning to one therefore compiled cleanly but failed to LINK with
 * "undefined reference to `<name>`".
 *
 * Found via a real PHP build: Zend/zend_execute.c's threaded VM
 * interpreter dispatch pointer,
 *   register const zend_op* volatile opline __asm__("r15");
 * assigned, incremented, and decremented (`opline = ...`, `opline++`,
 * `opline += N`, ...) in roughly 12000 places across
 * Zend/zend_vm_execute.h's generated opcode handlers.
 */
#if (defined(__x86_64__) || defined(_M_X64)) && !defined(_WIN32)
#include <stdio.h>

struct op { int val; };
static struct op prog[10];

register const struct op *volatile opline __asm__("r15");
register unsigned long ctr __asm__("r14");

static void init_prog(void)
{
    for (int i = 0; i < 10; i++)
        prog[i].val = i * 10;
}

static int run(void)
{
    /* Plain assignment. */
    opline = &prog[0];
    if (opline->val != 0) return 1;

    /* Pointer ++/-- must scale by the pointee size, not by 1 byte. */
    opline++;
    if (opline->val != 10) return 2;
    ++opline;
    if (opline->val != 20) return 3;
    opline--;
    if (opline->val != 10) return 4;
    --opline;
    if (opline->val != 0) return 5;

    /* Compound assignment (+=, -=). */
    opline += 3;
    if (opline->val != 30) return 6;
    opline -= 2;
    if (opline->val != 10) return 7;

    /* Same operations on a plain-integer register variable. */
    ctr = 0;
    ctr += 5;
    ctr++;
    if (ctr != 6) return 8;

    return 0;
}

int main(void)
{
    init_prog();
    int rc = run();
    if (rc) return rc;
    printf("OK global register variable write/inc/dec\n");
    return 0;
}
#else
int main(void) { return 0; }
#endif
