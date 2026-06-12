#include <stdio.h>

static int feq(double a, double b) {
    double d = a - b;
    if (d < 0) d = -d;
    return d < 1e-9;
}

_Complex int mk_ci(int r, int im) {
    _Complex int x;
    __real__ x = r;
    __imag__ x = im;
    return x;
}

int main(void) {
    _Complex double a = 1.0 + 2.0i;
    _Complex double b = 3.0 + 4.0i;

    // Multiple complex statements in one function, each followed by a
    // printf of __real__/__imag__: regression test for a register-leak
    // bug where the 4th+ such statement produced wrong values.
    _Complex double s1 = a + b;
    _Complex double s2 = a - b;
    _Complex double s3 = a * b;
    _Complex double s4 = a / b;
    _Complex double s5 = b - a;

    printf("s1: %f %f\n", __real__ s1, __imag__ s1);
    printf("s2: %f %f\n", __real__ s2, __imag__ s2);
    printf("s3: %f %f\n", __real__ s3, __imag__ s3);
    printf("s4: %f %f\n", __real__ s4, __imag__ s4);
    printf("s5: %f %f\n", __real__ s5, __imag__ s5);

    if (!feq(__real__ s1, 4.0) || !feq(__imag__ s1, 6.0)) return 1;
    if (!feq(__real__ s2, -2.0) || !feq(__imag__ s2, -2.0)) return 1;
    if (!feq(__real__ s3, -5.0) || !feq(__imag__ s3, 10.0)) return 1;
    if (!feq(__real__ s4, 0.44) || !feq(__imag__ s4, 0.08)) return 1;
    if (!feq(__real__ s5, 2.0) || !feq(__imag__ s5, 2.0)) return 1;

    // EQ/NE on complex doubles
    _Complex double c1 = 1.0 + 2.0i;
    _Complex double c2 = 1.0 + 2.0i;
    _Complex double c3 = 1.0 + 3.0i;
    if (!(c1 == c2)) return 1;
    if (c1 == c3) return 1;
    if (!(c1 != c3)) return 1;
    if (c1 != c2) return 1;

    // scalar <-> complex conversions
    _Complex double d1 = 5.0;
    if (!feq(__real__ d1, 5.0) || !feq(__imag__ d1, 0.0)) return 1;

    int iv = 7;
    _Complex double d2 = iv + 1.0i;
    if (!feq(__real__ d2, 7.0) || !feq(__imag__ d2, 1.0)) return 1;

    _Complex double d3 = a + 1.0;
    if (!feq(__real__ d3, 2.0) || !feq(__imag__ d3, 2.0)) return 1;

    _Complex double d4 = 1.0 + a;
    if (!feq(__real__ d4, 2.0) || !feq(__imag__ d4, 2.0)) return 1;

    // _Complex float
    _Complex float f1 = 1.0f + 2.0fi;
    _Complex float f2 = 3.0f + 4.0fi;
    _Complex float f3 = f1 + f2;
    if (!feq(__real__ f3, 4.0) || !feq(__imag__ f3, 6.0)) return 1;

    // _Complex int
    _Complex int ci1 = mk_ci(1, 2);
    _Complex int ci2 = mk_ci(3, 4);
    _Complex int ci3 = ci1 + ci2;
    if (__real__ ci3 != 4 || __imag__ ci3 != 6) return 1;
    _Complex int ci4 = ci2 - ci1;
    if (__real__ ci4 != 2 || __imag__ ci4 != 2) return 1;

    printf("OK\n");
    return 0;
}
