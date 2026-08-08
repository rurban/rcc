/* sizeof on a comma expression only evaluates its operand when the
 * *comma's final-operand* type is itself TY_VLA (C11 6.5.3.4p2 - not
 * merely "variably modified"; a pointer to a VLA doesn't count, only a
 * genuine VLA-typed expression does). rcc had two distinct bugs here:
 *  - sizeof(*(c++, p)) (an expression genuinely of VLA type) computed the
 *    runtime size straight from the type's captured length expr without
 *    ever evaluating the operand node itself, silently dropping the
 *    required side effect.
 *  - a VM array's length expression built from a comma operator, e.g.
 *    int[(c++, 5)], was folded to a compile-time constant by
 *    eval_const_expr ignoring the left operand's side effect entirely
 *    (ND_COMMA there returned eval_const_expr(rhs) only, regardless of
 *    whether lhs was itself side-effect-free), so c++ never ran.
 * From michaelforney/cproc's test/sizeof-vla.c. */
int c = 0;
int main(void) {
    int r = 0;
    int l = 2;
    int (*p)[l] = 0;
    r += sizeof(*(c++, p)) != 2 * sizeof(int); /* VLA: operand evaluated */
    r += c != 1;
    r += sizeof(int[(c++, 5)]) != 5 * sizeof(int); /* VLA type-name w/ comma dim */
    r += c != 2;
    return r;
}
