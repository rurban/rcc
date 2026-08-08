/* Regression test for the SysV x86-64 small-aggregate return ABI
 * (codegen.c): a struct/union <=16 bytes with no floating member
 * (div_t, ldiv_t, lldiv_t, imaxdiv_t, and most small integer-only PODs)
 * must return in RAX:RDX with no hidden return pointer -- not always via
 * a hidden pointer regardless of size, which is what rcc did before this
 * fix.
 *
 * Root cause: rcc's has_hidden_retbuf / param_index / has_retbuf
 * classification (codegen.c, several near-duplicated sites) treated
 * *every* struct/union return type as needing a hidden pointer argument,
 * regardless of size. Calling an *external* (non-rcc-compiled) function
 * with a small struct return -- div(), ldiv(), lldiv(), imaxdiv() from
 * libc chief among them -- silently passed a phantom hidden-pointer
 * argument the real (glibc) function never expects, shifting every real
 * argument into the wrong register and reading the "return value" out
 * of an address the callee never wrote to. Found via bash's own
 * arithmetic evaluator (expr.c), which computes every `%`/`/` inside
 * `$(( ))` via imaxdiv(): every modulo/division came back 0 or a fixed
 * garbage value, and the resulting infinite/wrong-bound loops hung or
 * mis-evaluated a good chunk of bash's own test suite (arith, arith-for,
 * and several others whose tests happen to exercise arithmetic
 * internally) -- not just a niche libc corner.
 *
 * Fixed by classifying struct/union return types <=16 bytes with no
 * float/double/_Complex member anywhere (recursively) as GP-register
 * returns (RAX, plus RDX if >8 bytes), matching the real ABI, while
 * anything bigger or containing a float still uses the pre-existing
 * hidden-pointer path unchanged.
 */
#include <assert.h>
#include <stdlib.h>
#include <inttypes.h>

/* The real-world trigger: libc's div-family functions returning a small
 * struct by value. */
static void test_libc_div_family(void) {
    div_t d = div(17, 5);
    assert(d.quot == 3 && d.rem == 2);

    ldiv_t ld = ldiv(-17L, 5L);
    assert(ld.quot == -3 && ld.rem == -2);

    lldiv_t lld = lldiv(100LL, 7LL);
    assert(lld.quot == 14 && lld.rem == 2);

    imaxdiv_t imd = imaxdiv((intmax_t)9, (intmax_t)9);
    assert(imd.quot == 1 && imd.rem == 0);
}

/* User-defined 16-byte, all-integer struct: exercises the definition
 * side (ND_RETURN emitting into RAX:RDX) as well as the call site. */
typedef struct {
    long a;
    long b;
} Pair16;

static Pair16 mk16(long a, long b) {
    Pair16 p;
    p.a = a;
    p.b = b;
    return p;
}

/* A pointer member is still INTEGER-class -- must also return in regs. */
typedef struct {
    char *p;
    int x;
} PtrPair;

static PtrPair mk_ptrpair(char *p, int x) {
    PtrPair r;
    r.p = p;
    r.x = x;
    return r;
}

/* Nested small all-integer struct, still <=16 bytes overall. */
typedef struct {
    int a;
    int b;
} Inner;
typedef struct {
    Inner i;
    int c;
    int d;
} Nested;

static Nested mk_nested(int a, int b, int c, int d) {
    Nested n;
    n.i.a = a;
    n.i.b = b;
    n.c = c;
    n.d = d;
    return n;
}

/* >16 bytes: must keep using the hidden-pointer path unchanged. */
typedef struct {
    long a, b, c;
} Triple;

static Triple mk_triple(long a, long b, long c) {
    Triple t;
    t.a = a;
    t.b = b;
    t.c = c;
    return t;
}

/* Contains a float member: must keep using the hidden-pointer path
 * unchanged (this fix only implements the all-integer classification). */
typedef struct {
    double d;
    int x;
} FloatMix;

static FloatMix mk_floatmix(double d, int x) {
    FloatMix r;
    r.d = d;
    r.x = x;
    return r;
}

static void test_user_defined_structs(void) {
    Pair16 p = mk16(100, 200);
    assert(p.a == 100 && p.b == 200);

    /* Assignment from a call directly into an existing variable. */
    Pair16 q;
    q = mk16(7, 8);
    assert(q.a == 7 && q.b == 8);

    /* Through a function pointer. */
    Pair16 (*fp)(long, long) = mk16;
    Pair16 r = fp(9, 10);
    assert(r.a == 9 && r.b == 10);

    PtrPair pp = mk_ptrpair("hello", 42);
    assert(pp.x == 42 && pp.p[0] == 'h');

    Nested n = mk_nested(1, 2, 3, 4);
    assert(n.i.a == 1 && n.i.b == 2 && n.c == 3 && n.d == 4);

    Triple t = mk_triple(11, 22, 33);
    assert(t.a == 11 && t.b == 22 && t.c == 33);

    FloatMix fm = mk_floatmix(3.5, 7);
    assert(fm.d == 3.5 && fm.x == 7);
}

int main(void) {
    test_libc_div_family();
    test_user_defined_structs();
    return 0;
}
