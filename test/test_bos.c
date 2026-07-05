// __builtin_object_size + diagnose_if compile-time buffer checks
// Pattern from safeclib: combine BOS with diagnose_if for overflow detection

#include <stdio.h>
#include <stdlib.h>

// diagnose_if with constant condition demonstrating the pattern
__attribute__((diagnose_if(1 && 16 < 8, "buffer overflow", "warning")))
void checked_copy(char *d, const char *s, size_t n) {
    for (size_t i = 0; i < n; i++) d[i] = s[i];
}

int main(void)
{
    char buf[16];

    // __builtin_object_size returns declared array size
    size_t bos = __builtin_object_size(buf, 0);
    if (bos != 16) return 1;

    // Heap pointer returns (size_t)-1 (unknown)
    char *p = malloc(64);
    if (__builtin_object_size(p, 0) != (size_t)-1) return 2;
    free(p);

    // Struct size
    struct { int a; char b[12]; } s;
    if (__builtin_object_size(&s, 0) != 16) return 3;

    // Call with constant condition that evaluates to false -> no warning
    checked_copy(buf, "hello", 5);
    if (buf[0] != 'h') return 4;

    printf("PASS\n");
    return 0;
}
