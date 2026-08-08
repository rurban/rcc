/* GNU typeof(expr) evaluates its operand exactly when the operand's type
 * (after any top-level comma operator's array/VLA-to-pointer decay) is
 * variably modified. rcc's queue_vm_typeof_eval() already classified
 * this correctly, but the queued evaluation was only ever flushed into
 * the statement stream by the declaration() caller's flush point - a
 * bare (non-declaration) statement whose typeof appears inside a cast or
 * compound literal, e.g. `(typeof(c++, p))0;`, silently dropped the
 * queued side effect since the generic statement fallback never checked
 * for a pending capture. From michaelforney/cproc's test/typeof-vm.c. */
int c = 0;
int main(void) {
    int l = 2;
    int (*p)[l] = 0;
    (typeof(c++, p))0;   /* VM (pointer-to-VLA doesn't decay): evaluated */
    (typeof(c++, p)){0}; /* same, inside a compound literal statement */
    return c != 2;
}
