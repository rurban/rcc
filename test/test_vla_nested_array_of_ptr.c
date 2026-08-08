/* `int (*p[f(2)])[f(3)];` declares p as an array of f(2) pointers, each to
 * a VLA-array of f(3) ints: an array-of-pointers whose own length AND whose
 * pointee element type's length are both runtime (VM) dimensions from two
 * distinct call-once expressions. rcc built the right Type shape but never
 * froze either dimension expression into a temp: the outer array's own
 * vla_len_expr got embedded directly into the ALLOCA size computation *and*
 * separately re-read (and thus re-evaluated) by `sizeof p`; the inner
 * (pointee) dimension was never evaluated for the allocation at all and
 * only ran (possibly more than once) whenever something like `sizeof **p`
 * read the raw unevaluated expression back out of the type. Fixed by
 * extending the vla_freeze_dims()-style dimension freeze (previously only
 * applied when declaring a plain pointer-to-VLA local) to also apply when
 * the declarator's own outermost type is itself a VLA - every dimension
 * expression reachable through the type's pointer/VLA chain is captured
 * into a hidden local exactly once, ahead of the array's stack allocation,
 * and every later use (sizeof, dereference) reads the frozen capture
 * instead of re-running the expression. From michaelforney/cproc's
 * test/vla-nested.c. */
int l;
int f(int x) {
    l += x;
    return x;
}
int main(void) {
    int r = 0;
    int (*p[f(2)])[f(3)];
    r += l != 5; /* f(2) + f(3), each evaluated exactly once: 0+2+3 */
    r += sizeof p != sizeof(int (*[2])[3]);
    r += sizeof **p != sizeof(int[3]);
    return r;
}
