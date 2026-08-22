// __builtin_object_size + __builtin_dynamic_object_size compile-time and runtime checks

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// A stack object passed through a function parameter: __builtin_dynamic_object_size
// must report unknown (-1/0), NOT read the glibc malloc chunk header at runtime
// (that read is only valid for a pointer actually into a heap chunk; applied to
// a stack pointer it reads garbage and made glibc's _FORTIFY_SOURCE=3
// __explicit_bzero_chk falsely abort with "*** buffer overflow detected ***"
// -- found via libsodium's sodium_memzero()).
static size_t param_bos0(unsigned char *p)
{
    return __builtin_dynamic_object_size(p, 0);
}

static size_t param_bos3(unsigned char *p)
{
    return __builtin_dynamic_object_size(p, 3);
}

static int stack_via_param(unsigned char *p, size_t n)
{
    // glibc's fortify explicit_bzero() expands to __explicit_bzero_chk(p, n,
    // __builtin_dynamic_object_size(p, 0)); with the runtime malloc-header
    // read bug, a stack array passed here aborted the process. glibc-only:
    // mingw/windows libc provides no explicit_bzero.
    memset(p, 0, n);
#if defined(__linux__)
    explicit_bzero(p, n);
#else
    memset(p, 0, n);
#endif
    return 0;
}

int main(void)
{
    char buf[16];

    // __builtin_object_size returns declared array size
    if (__builtin_object_size(buf, 0) != 16) return 1;

    // Struct size
    struct { int a; char b[12]; } s;
    if (__builtin_object_size(&s, 0) != 16) return 2;
    if (__builtin_dynamic_object_size(&s, 0) != 16) return 3;

    // Heap pointer: DBOS returns -1 (unknown, like GCC for a non-traceable
    // pointer) or the actual size; never a garbage stack read.
    char *p = malloc(64);
    size_t heap_sz = __builtin_dynamic_object_size(p, 0);
    if (heap_sz != (size_t)-1 && heap_sz < 56) return 4;
    free(p);

    // Parameter: unknown (-1 for mode 0, 0 for mode 3), never a runtime
    // malloc-header read of stack memory.
    if (param_bos0((unsigned char *)buf) != (size_t)-1) return 5;
    if (param_bos3((unsigned char *)buf) != 0) return 6;

    // A stack array zeroed through a function parameter must not trip a
    // fortify __explicit_bzero_chk abort (Linux/glibc only -- see
    // stack_via_param's guard).
#if defined(__linux__)
    unsigned char stackbuf[128];
    memset(stackbuf, 0xaa, sizeof stackbuf);
    if (stack_via_param(stackbuf, sizeof stackbuf) != 0) return 7;
    for (int i = 0; i < 128; i++)
        if (stackbuf[i] != 0) return 8;
#endif

    printf("PASS\n");
    return 0;
}
