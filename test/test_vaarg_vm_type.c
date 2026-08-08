/* __builtin_va_arg(ap, TYPE) where TYPE is a variably-modified type (a
 * pointer-to-VLA-array, e.g. `int (*)[++i]`) parsed the type-name via
 * type_name() but never threaded the embedded dimension expression's side
 * effect into the generated code - the VM Type's vla_len_expr just sat
 * unused inside the Type struct, so `++i` silently never ran. Fixed by
 * reusing vla_freeze_dims() (the same mechanism a VM-typed cast uses, e.g.
 * `(int (*)[++i])p`) to fold the side effect in exactly once, ahead of the
 * va_arg dereference. From michaelforney/cproc's test/builtin-vaarg-vm.c. */
int f(int i, ...) {
    int r;
    __builtin_va_list ap;

    __builtin_va_start(ap, i);
    r = **__builtin_va_arg(ap, int (*)[++i]);
    __builtin_va_end(ap);
    return r + i;
}

int main(void) {
    int a[3];

    a[0] = 123;
    /* i starts 3, ++i -> 4 (evaluated exactly once); r = a[0] = 123;
     * r + i = 127. */
    return f(3, &a) != 127;
}
