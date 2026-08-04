/* GCC Bug #116130 - Implement C23 N2956 paper - [[unsequenced]] and [[reproducible]] function type arguments
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=116130
 */


int ok(int *x) [[unsequenced]] {
    *x = 15;
    return 42;
}
int foo(uintptr_t x) [[unsequenced]] {
    *(int *)x = 1;
    return 1;
}
struct S {
    int *p;
};
int bar(struct S x) [[unsequenced]] {
    x.p[0] = 1;
    return 1;
}
int baz(...) [[unsequenced]] {
    va_list ap;
    va_start(ap);
    int *p = va_arg(ap, int *);
    va_end(ap);
    *p = 1;
    return 1;
}
union U {
    int *p;
    long long *q;
};
int qux(int x, union U y) [[unsequenced]] {
    if (x) y.p[0] = 1;
    else
        y.q[1] = 2;
    return 3;
}
int corge(int **x) [[unsequenced]] {
    *x[0] = 42;
    return 5;
}

// ok is valid if actually called, foo is certainly not, bar/baz/qux/corge are not, so it is just pointer type arguments (let's extend that for C++ with references, thoughts on references to pointers) and what they point directly to which can be modified and kept modified, not pointers to pointers, not pointers embedded in structs/unions/arrays or variadic parameters.
// And the directly pointed memory can be read or stored or first stored and then read, but not first read and then stored, at least if it results in observable side-effect.
// So, I guess e.g. memchr would be [[gnu::pure]] and [[unsequenced]], as it only reads from the directly pointed elements and not other global state.
