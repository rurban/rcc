// __builtin_object_size + __builtin_dynamic_object_size compile-time and runtime checks

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    char buf[16];

    // __builtin_object_size returns declared array size
    if (__builtin_object_size(buf, 0) != 16) return 1;

    // Struct size
    struct { int a; char b[12]; } s;
    if (__builtin_object_size(&s, 0) != 16) return 2;
    if (__builtin_dynamic_object_size(&s, 0) != 16) return 3;

    // Heap pointer: DBOS returns runtime size
    char *p = malloc(64);
    size_t heap_sz = __builtin_dynamic_object_size(p, 0);
    // Accept -1 (unknown, GCC -O0) or actual size (GCC -O2 returns 64, rcc reads chunk)
    if (heap_sz != (size_t)-1 && heap_sz < 56) return 4;
    free(p);

    printf("PASS\n");
    return 0;
}
