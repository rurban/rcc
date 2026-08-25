#include <assert.h>
#include <stdio.h>

#ifdef __SIZEOF_INT128__
typedef __int128 S;
typedef unsigned __int128 U;

__attribute__((noinline)) static U shl128(U x, int c) {
    return x << (c & -2);
}

__attribute__((noinline)) static U shr128u(U x, int c) {
    return x >> (c & -2);
}

__attribute__((noinline)) static S shr128s(S x, int c) {
    return x >> (c & -2);
}
#endif

int main(void) {
#ifdef __SIZEOF_INT128__
    U x = ((U)5 << 64) | 7;
    assert(shl128(x, 0) == x);
    assert(shr128u(x, 0) == x);
    assert(shr128s((S)x, 0) == (S)x);

    U b = (U)0x101 << (128 / 2 - 7);
    assert(shl128(b, 64) == ((U)0x101 << (128 - 7)));
    assert(shl128(b, 66) == ((U)0x101 << (128 - 5)));

    U top = (U)1 << 127;
    assert(shr128u(top, 64) == ((U)1 << 63));
    assert(shr128u(top, 66) == ((U)1 << 61));
    assert((U)shr128s((S)top, 5) == ((U)0x1f << (128 - 5)));
    assert((U)shr128s((S)top, 64) == ((U)-1 << 63));
    assert((U)shr128s((S)top, 66) == ((U)-1 << 61));
#endif
    printf("OK\n");
    return 0;
}
