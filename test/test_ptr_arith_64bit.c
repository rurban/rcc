/* 64-bit pointer arithmetic: offsets and differences >= 2^31.
 *
 * Bug 1 (type.c new_scale_mul): the pointer-offset multiply
 * `offset * elemsize` was hardcoded ty_int, so `char *p + long n` with
 * n >= 2^31 became `p + (long)(n * 1)` where `n * 1` was computed in
 * 32 bits, truncating the offset before the sign-extending cast.
 *
 * Bug 2 (type.c ptr-ptr ND_SUB): `p - q` was typed ty_int instead of
 * ptrdiff_t, so a difference >= 2^31 was computed in 32 bits.
 *
 * Both were found via ksh93's `printf -v v "%4000000000d"` (sfio
 * string-stream growth: `f->next - f->data` position arithmetic
 * corrupted the stream fields once the buffer passed 2 GB, segfaulting
 * at exactly 2^31).
 */
#include <stddef.h>
#include <stdio.h>

int main(void) {
    char buf[64];
    char *p = buf;
    long long n = 4000000000LL; /* LLP64-safe: long long is 64-bit everywhere */

    /* ptr + long long: exact 64-bit offset, no 32-bit truncation. */
    char *q = p + n;
    if ((unsigned long long)(q - p) != 4000000000ULL) {
        printf("FAIL: q-p = %lld, want 4000000000\n", (long long)(q - p));
        return 1;
    }
    if (q <= p) { /* a truncated negative offset would land below p */
        printf("FAIL: q <= p\n");
        return 1;
    }
    /* round trip */
    if (q - n != p) {
        printf("FAIL: q-n != p\n");
        return 1;
    }

    /* scaled: long* + 500M elements == 4e9 byte offset */
    long *pl = (long *)buf;
    long long m = 500000000LL;
    long *ql = pl + m;
    if (ql - pl != m) {
        printf("FAIL: ql-pl = %lld, want %lld\n", (long long)(ql - pl), m);
        return 1;
    }

    /* ptr - ptr typed ptrdiff_t: difference >= 2^31 (sfio pattern:
     * f->next - f->data with a >2 GB write position). */
    char *data = buf;
    char *next = buf + 2200000000LL;
    ptrdiff_t wrote = next - data;
    if ((unsigned long long)wrote != 2200000000ULL) {
        printf("FAIL: next-data = %lld, want 2200000000\n", (long long)wrote);
        return 1;
    }

    printf("OK\n");
    return 0;
}
