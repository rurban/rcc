// Regression: an implicitly-declared libc pointer function (memmove, memcpy,
// memset, ...) must be assumed to return a pointer, not int. When such a call
// is one branch of a ?: whose value is returned/used as a pointer, typing the
// result as int (the C implicit-declaration default) both emits a spurious
// "pointer/integer mismatch in conditional expression" warning AND truncates
// the pointer to 32 bits on LP64, corrupting it.
//
// This mirrors glibc's fortified <strings.h>, which defines bcopy()/bzero()
// via memmove()/memset() BEFORE <string.h>'s prototypes are in scope
// (gortex: libredwg dwg_api.c -> bits/strings_fortified.h).

int printf(const char *, ...);

// memmove used BEFORE its prototype below -> implicit declaration here.
static void *pick_mm(void *d, const void *s, unsigned long n, int use_dest)
{
    return use_dest ? d : memmove(d, s, n);
}

static void *pick_ms(void *d, unsigned long n, int use_dest)
{
    return use_dest ? d : memset(d, 'Z', n);
}

// Real prototypes appear only afterwards, exactly like the header ordering
// (<strings.h> pulled in before <string.h>) that provokes the bug.
void *memmove(void *, const void *, unsigned long);
void *memset(void *, int, unsigned long);

int main(void)
{
    char dst[8] = {0};
    char src[8] = "abcdefg";

    // memmove() branch: the returned pointer must survive the ?: intact.
    // If the ?: were int-typed, dst (a >4GB stack address on x86-64) would be
    // truncated and this comparison would fail.
    void *r = pick_mm(dst, src, sizeof dst, 0);
    if (r != (void *)dst) return 1;
    if (dst[0] != 'a') return 2; // memmove actually copied

    // pointer branch stays correct too
    void *r2 = pick_mm(dst, src, sizeof dst, 1);
    if (r2 != (void *)dst) return 3;

    // same for memset()
    void *r3 = pick_ms(dst, sizeof dst, 0);
    if (r3 != (void *)dst) return 4;
    if (dst[0] != 'Z') return 5;

    printf("PASS\n");
    return 0;
}
