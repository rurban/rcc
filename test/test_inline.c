// Unit test for -finline (enabled at -O2/-O3). Every function below is a
// candidate the inliner may replace at its call sites; inlining must never
// change the observed result. The asserts hold at any optimization level,
// so the test is meaningful whether or not inlining actually fires.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "test_common.h"

static int add(int a, int b) { return a + b; }
static inline int sq(int x) { return x * x; }
int maxi(int a, int b) { return a > b ? a : b; }
static int getx(int *p) { return *p; }
static int usemul(int a) { return a + a + a; } // param used 3x
static int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
static int deref2(int *p) { return *p + *p; } // read-through-pointer twice
static unsigned char trunc8(int x) { return (unsigned char)x; } // return-type cast
static double fscale(double x, double k) { return x * k + 1.0; }

// Directly recursive: must NOT be inlined, but must still compute correctly.
static int fib(int n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }

// Writes its by-value parameter: must NOT be unsoundly inlined (the caller's
// argument must be unaffected).
static int inc(int x) { return ++x; }

// A tiny "return EXPR;" function whose return expression contains a GNU
// statement expression `({ p; })` is a candidate for -finline's deep-clone
// path (clone_expr() in opt.c). `stmt_expr_result` is a pointer *alias*
// into the statement expression's own `body` list (see parser.c); codegen
// locates the value-producing statement via pointer equality against that
// alias. clone_expr() used to clone `body` and `stmt_expr_result`
// independently, breaking the aliasing on every inlined statement
// expression - codegen then never found a value, producing a bogus
// register result (silently wrong code) or an "lvalue required" error in
// an lvalue context like the `->x` member access below.
//
// This can only be exercised with -finline actually enabled, which the
// unit-test harness's default `-O1` doesn't turn on (see rccflags in
// run_tests.c) - so drive rcc as a subprocess with an explicit -O2,
// independent of the flags this test binary itself was compiled with.
// Found via a real Linux kernel build: kernel/cgroup/namespace.c's
// inc_cgroup_namespaces() returns inc_ucount(ns, current_euid(), ...),
// where current_euid() expands through nested statement expressions
// ending in a ->euid member access; inlined from an assignment in
// copy_cgroup_ns(), rcc reported a bogus "lvalue required as left operand
// of assignment" for a line with no assignment anywhere near it.
static void check_finline_stmt_expr_clone(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();

    char srcf[128], exef[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_finline_se_%d.c", td, pid);
    snprintf(exef, sizeof(exef), "%s/test_finline_se_%d", td, pid);

    static const char src[] =
        "struct S { int x; };\n"
        "static int f(struct S *p) {\n"
        "    return ({ p; })->x;\n"
        "}\n"
        "int main(int argc, char **argv) {\n"
        "    struct S s;\n"
        "    s.x = argc + 41;\n"
        "    return f(&s) == 42 ? 0 : 1;\n"
        "}\n";

    FILE *f = fopen(srcf, "w");
    assert(f && "cannot write temp source for finline stmt-expr check");
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -O2 -o %s %s " NULL_REDIRECT, rcc, exef, srcf);
    int rc = system(cmd);
    assert(rc == 0 && "-O2 compile of inlined statement-expression member access failed");

    snprintf(cmd, sizeof(cmd), "%s " NULL_REDIRECT, exef);
    rc = system(cmd);
    remove(exef);
    remove(srcf);
    assert(rc == 0 && "inlined statement-expression member access produced wrong code");
}

int main(void) {
    int y = 3, z = 4, v = 5;
    int arr[3] = {10, 20, 30};

    assert(add(y, z) == 7);
    assert(add(y, z) * 2 == 14);
    assert(sq(6) == 36);
    assert(maxi(y, z) == 4);
    assert(getx(&v) == 5);
    assert(usemul(y) == 9);
    assert(clampi(y, 0, 5) == 3);
    assert(clampi(9, 0, 5) == 5);
    assert(deref2(&arr[2]) == 60);
    assert(trunc8(300) == 44); // 300 & 0xff
    assert(fscale(2.0, 3.0) == 7.0);
    assert(fib(10) == 55);

    // inc() reads a copy; y must remain 3 after the call.
    assert(inc(y) == 4);
    assert(y == 3);

    check_finline_stmt_expr_clone();

    return 0;
}
