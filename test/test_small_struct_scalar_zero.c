/* `s = 0;` (a scalar RHS assigned directly to a struct/union LHS --
 * rcc accepts this permissively; a `(struct S){0}` compound literal
 * does NOT reach this path, it goes through the ordinary byte-copy
 * loop instead) triggers a codegen fast path that stores the scalar
 * in one register-sized mov instead of copying byte by byte -- see
 * codegen.c's ND_ASSIGN handling of a scalar RHS against a
 * struct/union LHS of size <= 8. It miscomputed the store width for
 * sizes that aren't already a valid x86 mov width: `int st_sz = ...;
 * if (st_sz < 4) st_sz = st_sz;` was a no-op typo. For sizes 1 and 2
 * asm_mov_reg_mem already emits movb/movw (harmless coincidence), but
 * sizes 5, 6 and 7 (an all-char struct/union, whose alignment-1
 * layout doesn't round sizeof up) silently encoded the same as size
 * 4 -- a plain movl -- leaving the trailing bytes unzeroed (stale
 * stack/bss content exposed instead of the requested zero-init).
 * Size 3 happened to already encode as a movl too (the encoder's
 * opsize()/size16_pfx() only special-case 1, 2 and 8), so it was
 * accidentally fine either way; covered anyway for symmetry. */
#include <stdio.h>
#include <string.h>

struct S3 {
    char a, b, c;
};
struct S5 {
    char a, b, c, d, e;
};
struct S6 {
    char a, b, c, d, e, f;
};
struct S7 {
    char a, b, c, d, e, f, g;
};

static int fail;
#define CHECK_ZERO(s)                                                              \
    do {                                                                           \
        for (size_t _i = 0; _i < sizeof(s); _i++)                                  \
            if (((unsigned char *)&(s))[_i] != 0) {                                \
                fprintf(stderr, "FAIL line %d: byte %zu of " #s " not zero\n",     \
                        __LINE__, _i);                                             \
                fail++;                                                            \
            }                                                                      \
    } while (0)

int main(void) {
    struct S3 s3;
    struct S5 s5;
    struct S6 s6;
    struct S7 s7;

    memset(&s3, 0xAA, sizeof(s3));
    s3 = 0; // scalar RHS -> struct LHS: the buggy codegen branch
    CHECK_ZERO(s3);

    memset(&s5, 0xAA, sizeof(s5));
    s5 = 0;
    CHECK_ZERO(s5);

    memset(&s6, 0xAA, sizeof(s6));
    s6 = 0;
    CHECK_ZERO(s6);

    memset(&s7, 0xAA, sizeof(s7));
    s7 = 0;
    CHECK_ZERO(s7);

    return fail ? 1 : 0;
}
