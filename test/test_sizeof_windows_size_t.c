#include <assert.h>
#include <stdio.h>

#if defined(_WIN32) && __SIZEOF_LONG__ < __SIZEOF_POINTER__
typedef __SIZE_TYPE__ Size_t;
#define BUFSIZE ((1LL << (8 * sizeof(Size_t) - 2)) - 256)

struct huge_struct {
    short buf[BUFSIZE];
    int a;
    int b;
    int c;
    int d;
};

union huge_union {
    int a;
    char buf[BUFSIZE];
};
#endif

int main(void) {
#if defined(_WIN32) && __SIZEOF_LONG__ < __SIZEOF_POINTER__
    assert(sizeof(sizeof(int)) == sizeof(Size_t));
    assert(sizeof(union huge_union) == (Size_t)BUFSIZE);
    assert(sizeof(struct huge_struct) == sizeof(short) * (Size_t)BUFSIZE + 4 * sizeof(int));
    assert((Size_t)&((struct huge_struct *)0)->a >= sizeof(short) * (Size_t)BUFSIZE);
#endif
    printf("OK\n");
    return 0;
}
