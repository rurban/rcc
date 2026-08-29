/* A GCC "label as value" (&&label) address wrapped in a cast, used as
 * one element of a computed-goto dispatch table's static array
 * initializer -- the classic threaded-interpreter idiom (PHP's Zend VM,
 * CPython's ceval.c, ...).
 *
 * Regression: read_global_label_initializer() (the hand-rolled mini
 * parser for pointer-typed global/static-local initializers) recognized
 * a bare `&&label`, but checked for it BEFORE stripping any leading
 * cast -- so `(void*)&&label` (the shape every real dispatch table
 * actually uses, since the array's element type is `void *` or a
 * handler typedef, not the label's own implicit type) fell through
 * unhandled, failing with "expected constant expression in initializer".
 *
 * Found via a real PHP build: Zend/zend_vm_execute.h's HYBRID VM
 * dispatch table:
 *   static zend_vm_opcode_handler_t const labels[] = {
 *       (void*)&&ZEND_NOP_SPEC_LABEL,
 *       (void*)&&ZEND_ADD_SPEC_CONST_CONST_LABEL,
 *       ...
 *   };
 */
#if defined(__GNUC__) || defined(__clang__)
#include <stdio.h>

typedef void *handler_t;

static int dispatch(int op)
{
    static handler_t const labels[] = {
        (void *)&&L_NOP,
        (void *)&&L_ADD,
        (void *)&&L_SUB,
    };

    goto *labels[op];
L_NOP:
    return 0;
L_ADD:
    return 100;
L_SUB:
    return -100;
}

int main(void)
{
    if (dispatch(0) != 0) return 1;
    if (dispatch(1) != 100) return 2;
    if (dispatch(2) != -100) return 3;
    printf("OK computed-goto dispatch table\n");
    return 0;
}
#else
int main(void) { return 0; }
#endif
