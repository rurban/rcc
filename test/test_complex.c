// Exhaustive _Complex coverage: every base type supported on all three
// targets (int/long/long long/unsigned/short/char + float/double/long
// double), every binary op (+ - * /) complex×complex and scalar×complex
// in both operand orders, unary minus, equality, __real__/__imag__ as
// lvalue and rvalue (also on expressions), casts in every direction,
// and function parameter/return round-trips.
//
// Regression tests for bugs found while making this exhaustive:
// - integer imaginary literals (`1i`, `2+3i`, `0x10i`, `010i`) folded
//   to real+0i because the lexer only parsed the 0b/0o/0-octal forms
//   (src/lexer.c parse_int_literal)
// - x86 integer-complex multiply computed the b*d term as a*d
//   (src/codegen.c ND_MUL)
// - int-complex -> float casts emitted cvtsi2ss for double targets
//   (src/codegen.c)
// - float-complex -> int-complex casts read components as doubles
//   (src/codegen.c)
// - _Complex long double component offsets: casts/assignments wrote or
//   read the imag part at 8 instead of 16 (src/codegen.c)
// - 4-byte integer-complex function returns were moved as 8 bytes via
//   xmm0, corrupting the neighboring slot and violating the SysV ABI
//   (src/codegen.c; returns now go in RAX/RDX at exact width)
//
// Integer complex division uses denominators with a zero component:
// gcc's integer complex division (Smith's method with truncation)
// diverges from the naive formula otherwise, e.g. (-5+10i)/(1+2i).

#include <stdio.h>
#include <stdbool.h>

static int nfail;
#define CHECK(cond, ...) do { \
    if (!(cond)) { nfail++; printf("FAIL %s:%d: ", __FILE__, __LINE__); \
                   printf(__VA_ARGS__); printf("\n"); } \
} while (0)

static int feq(double a, double b) {
    double d = a - b;
    if (d < 0) d = -d;
    return d < 1e-6;
}

// Constructors via __real__/__imag__ lvalue writes (also tests those).
static _Complex char mkc(char r, char im) {
    _Complex char x; __real__ x = r; __imag__ x = im; return x;
}
static _Complex short mks(short r, short im) {
    _Complex short x; __real__ x = r; __imag__ x = im; return x;
}
static _Complex int mki(int r, int im) {
    _Complex int x; __real__ x = r; __imag__ x = im; return x;
}
static _Complex unsigned mku(unsigned r, unsigned im) {
    _Complex unsigned x; __real__ x = r; __imag__ x = im; return x;
}
static _Complex long mkl(long r, long im) {
    _Complex long x; __real__ x = r; __imag__ x = im; return x;
}
static _Complex long long mkll(long long r, long long im) {
    _Complex long long x; __real__ x = r; __imag__ x = im; return x;
}
static _Complex float mkf(float r, float im) {
    _Complex float x; __real__ x = r; __imag__ x = im; return x;
}
static _Complex double mkd(double r, double im) {
    _Complex double x; __real__ x = r; __imag__ x = im; return x;
}
static _Complex long double mkld(long double r, long double im) {
    _Complex long double x; __real__ x = r; __imag__ x = im; return x;
}

// Parameter/return round-trips (SysV ABI: integer complex in GP regs,
// float complex in SSE regs).
static _Complex int ci_add(_Complex int a, _Complex int b) { return a + b; }
static _Complex short cs_add(_Complex short a, _Complex short b) { return a + b; }
static _Complex long cl_mul(_Complex long a, _Complex long b) { return a * b; }
static _Complex double cd_div(_Complex double a, _Complex double b) { return a / b; }

int main(void) {
    // ---- integer imaginary literals (lexer) ----
    _Complex int l1 = 1i;
    CHECK(__real__ l1 == 0 && __imag__ l1 == 1, "1i = %d+%di", __real__ l1, __imag__ l1);
    _Complex int l2 = 2 + 3i;
    CHECK(__real__ l2 == 2 && __imag__ l2 == 3, "2+3i = %d+%di", __real__ l2, __imag__ l2);
    _Complex int l3 = 0x10i;
    CHECK(__real__ l3 == 0 && __imag__ l3 == 16, "0x10i = %d+%di", __real__ l3, __imag__ l3);
    _Complex int l4 = 010i;
    CHECK(__real__ l4 == 0 && __imag__ l4 == 8, "010i = %d+%di", __real__ l4, __imag__ l4);
    _Complex int l5 = -1i;
    CHECK(__real__ l5 == 0 && __imag__ l5 == -1, "-1i = %d+%di", __real__ l5, __imag__ l5);
    _Complex double lf = 1.5 + 2.5i;
    CHECK(feq(__real__ lf, 1.5) && feq(__imag__ lf, 2.5), "1.5+2.5i");
    _Complex float lff = 2.0f + 3.0fi;
    CHECK(feq(__real__ lff, 2.0) && feq(__imag__ lff, 3.0), "2.0f+3.0fi");
    _Complex long double lld = 1.0L + 2.0Li;
    CHECK(feq(__real__ lld, 1.0) && feq(__imag__ lld, 2.0), "1.0L+2.0Li");

    // ---- _Complex int: all ops, literal and computed operands ----
    _Complex int a = mki(2, 3), b = mki(1, 2);
    _Complex int r;
    r = a + b;  CHECK(__real__ r == 3 && __imag__ r == 5, "int add %d+%di", __real__ r, __imag__ r);
    r = a - b;  CHECK(__real__ r == 1 && __imag__ r == 1, "int sub %d+%di", __real__ r, __imag__ r);
    r = a * b;  CHECK(__real__ r == -4 && __imag__ r == 7, "int mul %d+%di", __real__ r, __imag__ r);
    r = mki(6, 8) / mki(2, 0);  CHECK(__real__ r == 3 && __imag__ r == 4, "int div(real) %d+%di", __real__ r, __imag__ r);
    r = mki(6, 8) / mki(0, 2);  CHECK(__real__ r == 4 && __imag__ r == -3, "int div(imag) %d+%di", __real__ r, __imag__ r);
    r = -a;     CHECK(__real__ r == -2 && __imag__ r == -3, "int neg %d+%di", __real__ r, __imag__ r);
    // scalar mixing, both orders
    r = 5 + a;  CHECK(__real__ r == 7 && __imag__ r == 3, "5+a %d+%di", __real__ r, __imag__ r);
    r = a + 5;  CHECK(__real__ r == 7 && __imag__ r == 3, "a+5 %d+%di", __real__ r, __imag__ r);
    r = 5 - a;  CHECK(__real__ r == 3 && __imag__ r == -3, "5-a %d+%di", __real__ r, __imag__ r);
    r = a - 5;  CHECK(__real__ r == -3 && __imag__ r == 3, "a-5 %d+%di", __real__ r, __imag__ r);
    r = 5 * a;  CHECK(__real__ r == 10 && __imag__ r == 15, "5*a %d+%di", __real__ r, __imag__ r);
    r = a * 5;  CHECK(__real__ r == 10 && __imag__ r == 15, "a*5 %d+%di", __real__ r, __imag__ r);
    r = mki(10, 15) / 5; CHECK(__real__ r == 2 && __imag__ r == 3, "(10+15i)/5 %d+%di", __real__ r, __imag__ r);
    r = 5 / 1i; CHECK(__real__ r == 0 && __imag__ r == -5, "5/1i %d+%di", __real__ r, __imag__ r);
    // literal-driven constants (folded at compile time)
    r = 5 * 1i; CHECK(__real__ r == 0 && __imag__ r == 5, "5*1i %d+%di", __real__ r, __imag__ r);
    r = 1i * 2i; CHECK(__real__ r == -2 && __imag__ r == 0, "1i*2i %d+%di", __real__ r, __imag__ r);
    r = 5 + 1i; CHECK(__real__ r == 5 && __imag__ r == 1, "5+1i %d+%di", __real__ r, __imag__ r);
    r = 5 - 1i; CHECK(__real__ r == 5 && __imag__ r == -1, "5-1i %d+%di", __real__ r, __imag__ r);
    r = 1i + 5; CHECK(__real__ r == 5 && __imag__ r == 1, "1i+5 %d+%di", __real__ r, __imag__ r);
    // equality
    _Complex int e1 = mki(1, 2), e2 = mki(1, 2), e3 = mki(1, 3);
    CHECK(e1 == e2 && e1 != e3 && !(e1 == e3) && !(e1 != e2), "int complex EQ/NE");
    // __real__/__imag__ rvalue on expressions
    CHECK(__real__(a * b) == -4 && __imag__(a * b) == 7, "__real__(a*b)");
    CHECK(__real__(a / mki(2, 0)) == 1 && __imag__(a / mki(2, 0)) == 1, "__real__(a/2)");

    // ---- _Complex char / short / unsigned (extension bases) ----
    // plain char is signed on x86 but unsigned on aarch64, so the
    // negative results are compared via (signed char)
    _Complex char ca = mkc(2, 3), cb = mkc(1, 2), cr;
    cr = ca * cb; CHECK((signed char)__real__ cr == -4 && __imag__ cr == 7, "char mul %d+%di", (int)__real__ cr, (int)__imag__ cr);
    cr = ca + cb; CHECK(__real__ cr == 3 && __imag__ cr == 5, "char add %d+%di", __real__ cr, __imag__ cr);
    cr = ca - cb; CHECK(__real__ cr == 1 && __imag__ cr == 1, "char sub %d+%di", __real__ cr, __imag__ cr);
    _Complex short sa = mks(2, 3), sb = mks(1, 2), sr;
    sr = sa * sb; CHECK(__real__ sr == -4 && __imag__ sr == 7, "short mul %d+%di", __real__ sr, __imag__ sr);
    sr = sa + sb; CHECK(__real__ sr == 3 && __imag__ sr == 5, "short add %d+%di", __real__ sr, __imag__ sr);
    sr = mks(6, 8) / mks(2, 0); CHECK(__real__ sr == 3 && __imag__ sr == 4, "short div %d+%di", __real__ sr, __imag__ sr);
    _Complex unsigned ua = mku(2, 3), ub = mku(1, 2), ur;
    ur = ua * ub; CHECK(__real__ ur == 4294967292u && __imag__ ur == 7u, "uint mul %u+%ui", __real__ ur, __imag__ ur);
    ur = ua + ub; CHECK(__real__ ur == 3 && __imag__ ur == 5, "uint add %u+%ui", __real__ ur, __imag__ ur);

    // ---- _Complex long / long long ----
    _Complex long la = mkl(3, 4), lb = mkl(1, 2), lr;
    lr = la * lb; CHECK(__real__ lr == -5 && __imag__ lr == 10, "long mul %ld+%ldi", (long)__real__ lr, (long)__imag__ lr);
    lr = la + lb; CHECK(__real__ lr == 4 && __imag__ lr == 6, "long add %ld+%ldi", (long)__real__ lr, (long)__imag__ lr);
    lr = la - lb; CHECK(__real__ lr == 2 && __imag__ lr == 2, "long sub %ld+%ldi", (long)__real__ lr, (long)__imag__ lr);
    lr = mkl(6, 8) / mkl(2, 0); CHECK(__real__ lr == 3 && __imag__ lr == 4, "long div %ld+%ldi", (long)__real__ lr, (long)__imag__ lr);
    lr = mkl(2, 0) * la; CHECK(__real__ lr == 6 && __imag__ lr == 8, "long scal %ld+%ldi", (long)__real__ lr, (long)__imag__ lr);
    _Complex long long xa = mkll(3000000, 4000000), xb = mkll(1000000, 2000000), xr;
    xr = xa * xb; CHECK(__real__ xr == -5000000000000LL && __imag__ xr == 10000000000000LL,
                        "llong mul %lld+%lldi", (long long)__real__ xr, (long long)__imag__ xr);
    xr = xa + xb; CHECK(__real__ xr == 4000000 && __imag__ xr == 6000000, "llong add");
    xr = mkll(6000000, 8000000) / mkll(2000000, 0); CHECK(__real__ xr == 3 && __imag__ xr == 4, "llong div");

    // ---- _Complex float / double / long double ----
    _Complex float fa = mkf(1.5f, 2.5f), fb = mkf(2.0f, 1.0f), fr;
    fr = fa + fb; CHECK(feq(__real__ fr, 3.5) && feq(__imag__ fr, 3.5), "float add %f+%fi", __real__ fr, __imag__ fr);
    fr = fa * fb; CHECK(feq(__real__ fr, 0.5) && feq(__imag__ fr, 6.5), "float mul %f+%fi", __real__ fr, __imag__ fr);
    fr = fa / fb; CHECK(feq(__real__ fr, 1.1) && feq(__imag__ fr, 0.7), "float div %f+%fi", __real__ fr, __imag__ fr);
    fr = -fa;     CHECK(feq(__real__ fr, -1.5) && feq(__imag__ fr, -2.5), "float neg");
    fr = 2.0f * fa; CHECK(feq(__real__ fr, 3.0) && feq(__imag__ fr, 5.0), "float scal");
    fr = fa * 2.0f; CHECK(feq(__real__ fr, 3.0) && feq(__imag__ fr, 5.0), "float scal2");
    fr = fa + 0.5f; CHECK(feq(__real__ fr, 2.0) && feq(__imag__ fr, 2.5), "float +scal");
    _Complex float fe1 = mkf(1, 2), fe2 = mkf(1, 2), fe3 = mkf(1, 3);
    CHECK(fe1 == fe2 && fe1 != fe3, "float complex EQ/NE");

    _Complex double da = mkd(1.0, 2.0), db = mkd(3.0, 4.0), dr;
    dr = da + db; CHECK(feq(__real__ dr, 4.0) && feq(__imag__ dr, 6.0), "double add");
    dr = da - db; CHECK(feq(__real__ dr, -2.0) && feq(__imag__ dr, -2.0), "double sub");
    dr = da * db; CHECK(feq(__real__ dr, -5.0) && feq(__imag__ dr, 10.0), "double mul");
    dr = da / db; CHECK(feq(__real__ dr, 0.44) && feq(__imag__ dr, 0.08), "double div");
    dr = -da;     CHECK(feq(__real__ dr, -1.0) && feq(__imag__ dr, -2.0), "double neg");
    dr = 2.0 + da; CHECK(feq(__real__ dr, 3.0) && feq(__imag__ dr, 2.0), "double +scal");
    dr = da + 2.0; CHECK(feq(__real__ dr, 3.0) && feq(__imag__ dr, 2.0), "double scal+");
    dr = 2.0 * da; CHECK(feq(__real__ dr, 2.0) && feq(__imag__ dr, 4.0), "double *scal");
    dr = da * 2.0; CHECK(feq(__real__ dr, 2.0) && feq(__imag__ dr, 4.0), "double scal*");
    dr = mkd(6.0, 8.0) / 2.0; CHECK(feq(__real__ dr, 3.0) && feq(__imag__ dr, 4.0), "double /scal");
    dr = 2.0 / (0.0 + 1.0i); CHECK(feq(__real__ dr, 0.0) && feq(__imag__ dr, -2.0), "double scal/");
    _Complex double de1 = mkd(1, 2), de2 = mkd(1, 2), de3 = mkd(1, 3);
    CHECK(de1 == de2 && de1 != de3, "double complex EQ/NE");

    _Complex long double xd = mkld(1.0L, 2.0L), yd = mkld(3.0L, 4.0L), ldr;
    ldr = xd + yd; CHECK(feq(__real__ ldr, 4.0) && feq(__imag__ ldr, 6.0), "ldouble add");
    ldr = xd * yd; CHECK(feq(__real__ ldr, -5.0) && feq(__imag__ ldr, 10.0), "ldouble mul");
    ldr = xd / yd; CHECK(feq(__real__ ldr, 0.44) && feq(__imag__ ldr, 0.08), "ldouble div");
    ldr = 2.0L * xd; CHECK(feq(__real__ ldr, 2.0) && feq(__imag__ ldr, 4.0), "ldouble scal");

    // ---- casts ----
    // complex -> scalar (real part)
    CHECK((int)(2 + 3i) == 2, "(int)(2+3i)");
    CHECK((long)(2 + 3i) == 2, "(long)(2+3i)");
    CHECK((long long)(2 + 3i) == 2, "(ll)(2+3i)");
    CHECK((double)(2 + 3i) == 2.0, "(double)(2+3i)");
    CHECK(feq((float)(2 + 3i), 2.0f), "(float)(2+3i)");
    CHECK(feq((double)mki(-5, 10), -5.0), "(double)(-5+10i)");
    CHECK(feq((float)mkd(1.5, 2.5), 1.5f), "(float)(1.5+2.5i)");
    CHECK((long long)mkd(2.5, 3.5) == 2, "(ll)(2.5+3.5i)");
    // complex -> bool (nonzero test)
    CHECK((0 + 0i) == 0, "(0+0i)==0");
    CHECK((0 + 1i) != 0, "(0+1i)!=0");
    CHECK((5 + 0i) != 0, "(5+0i)!=0");
    // scalar -> complex
    _Complex double s2c = 5.0;
    CHECK(feq(__real__ s2c, 5.0) && feq(__imag__ s2c, 0.0), "5.0 -> _Complex double");
    _Complex float s2cf = 5.0f;
    CHECK(feq(__real__ s2cf, 5.0) && feq(__imag__ s2cf, 0.0), "5.0f -> _Complex float");
    _Complex int s2ci = 7;
    CHECK(__real__ s2ci == 7 && __imag__ s2ci == 0, "7 -> _Complex int");
    // complex -> complex, same kind, different width
    _Complex double wid = (_Complex double)(2 + 3i);
    CHECK(feq(__real__ wid, 2.0) && feq(__imag__ wid, 3.0), "(cd)(2+3i)");
    _Complex long lwid = (_Complex long)(2 + 3i);
    CHECK(__real__ lwid == 2 && __imag__ lwid == 3, "(cl)(2+3i)");
    _Complex long long llwid = (_Complex long long)(2 + 3i);
    CHECK(__real__ llwid == 2 && __imag__ llwid == 3, "(cll)(2+3i)");
    _Complex float fwid = (_Complex float)(2 + 3i);
    CHECK(feq(__real__ fwid, 2.0) && feq(__imag__ fwid, 3.0), "(cf)(2+3i)");
    _Complex long double ldwid = (_Complex long double)(2 + 3i);
    CHECK(feq(__real__ ldwid, 2.0) && feq(__imag__ ldwid, 3.0), "(cld)(2+3i)");
    // float complex -> integer complex (truncation)
    _Complex int t1 = (_Complex int)(2.0 + 3.0i);
    CHECK(__real__ t1 == 2 && __imag__ t1 == 3, "(ci)(2+3i)");
    _Complex int t2 = (_Complex int)(1.5 + 2.5i);
    CHECK(__real__ t2 == 1 && __imag__ t2 == 2, "(ci)(1.5+2.5i)");
    _Complex int t3 = (_Complex int)(-1.5 + 2.5i);
    CHECK(__real__ t3 == -1 && __imag__ t3 == 2, "(ci)(-1.5+2.5i)");
    _Complex int t4 = (_Complex int)(2.0f + 3.0fi);
    CHECK(__real__ t4 == 2 && __imag__ t4 == 3, "(ci)(2f+3fi)");
    _Complex int t5 = (_Complex int)(2.0L + 3.0Li);
    CHECK(__real__ t5 == 2 && __imag__ t5 == 3, "(ci)(2L+3Li)");
    _Complex int t6 = (_Complex int)(1.5f + 2.5fi);
    CHECK(__real__ t6 == 1 && __imag__ t6 == 2, "(ci)(1.5f+2.5fi)");
    // integer complex -> float complex (and mixed-width assignments)
    _Complex float ft = (_Complex float)(2 + 3i);
    CHECK(feq(__real__ ft, 2.0) && feq(__imag__ ft, 3.0), "(cf)(2+3i)");
    _Complex double mxd = mkl(2, 3);          // _Complex long -> _Complex double
    CHECK(feq(__real__ mxd, 2.0) && feq(__imag__ mxd, 3.0), "L->D assign");
    _Complex int mxi = mxd;                    // _Complex double -> _Complex int
    CHECK(__real__ mxi == 2 && __imag__ mxi == 3, "D->I assign");
    _Complex long double mxld = mkl(2, 3);     // _Complex long -> _Complex long double
    CHECK(feq(__real__ mxld, 2.0) && feq(__imag__ mxld, 3.0), "L->LD assign");
    _Complex float mxf = mkl(2, 3);            // _Complex long -> _Complex float
    CHECK(feq(__real__ mxf, 2.0) && feq(__imag__ mxf, 3.0), "L->F assign");
    _Complex long mxl = mxd;                   // _Complex double -> _Complex long
    CHECK(__real__ mxl == 2 && __imag__ mxl == 3, "D->L assign");
    _Complex int mxi2 = (2.0L + 3.0Li);        // ldouble complex -> int complex
    CHECK(__real__ mxi2 == 2 && __imag__ mxi2 == 3, "LD->I assign");
    _Complex double mxd2 = (2.0L + 3.0Li);     // ldouble complex -> double complex
    CHECK(feq(__real__ mxd2, 2.0) && feq(__imag__ mxd2, 3.0), "LD->D assign");
    _Complex double mxd3 = mki(2, 3);          // _Complex int -> _Complex double
    CHECK(feq(__real__ mxd3, 2.0) && feq(__imag__ mxd3, 3.0), "I->D assign");
    _Complex float mxf2 = mki(2, 3);           // _Complex int -> _Complex float
    CHECK(feq(__real__ mxf2, 2.0) && feq(__imag__ mxf2, 3.0), "I->F assign");
    _Complex long double mxl2 = mki(2, 3);     // _Complex int -> _Complex long double
    CHECK(feq(__real__ mxl2, 2.0) && feq(__imag__ mxl2, 3.0), "I->LD assign");

    // ---- function parameter/return round-trips ----
    _Complex int ri = ci_add(mki(2, 3), mki(4, 5));
    CHECK(__real__ ri == 6 && __imag__ ri == 8, "ci_add");
    _Complex short rs = cs_add(mks(2, 3), mks(1, 2));
    CHECK(__real__ rs == 3 && __imag__ rs == 5, "cs_add");
    _Complex long rl = cl_mul(mkl(3, 4), mkl(1, 2));
    CHECK(__real__ rl == -5 && __imag__ rl == 10, "cl_mul");
    _Complex double rd = cd_div(mkd(1, 2), mkd(3, 4));
    CHECK(feq(__real__ rd, 0.44) && feq(__imag__ rd, 0.08), "cd_div");
    // constructor round-trips already exercise the return path (mk_*)
    _Complex char rc = mkc(-1, -2);
    CHECK((signed char)__real__ rc == -1 && (signed char)__imag__ rc == -2, "mkc return");
    _Complex short rsc = mks(3, -4);
    CHECK(__real__ rsc == 3 && __imag__ rsc == -4, "mks return");

    if (nfail) {
        printf("%d FAILURES\n", nfail);
        return 1;
    }
    printf("OK\n");
    return 0;
}
