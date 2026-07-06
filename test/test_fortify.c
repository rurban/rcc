// Test _FORTIFY_SOURCE support: __builtin___*_chk wrappers
// rcc uses __builtin_object_size for bounds checking
#define _FORTIFY_SOURCE 2
#include <string.h>
#include <stdio.h>

int main(void)
{
    char buf[32];

    // memcpy_chk — within bounds
    memcpy(buf, "hello", 6);
    if (buf[0] != 'h') return 1;

    // memset_chk — within bounds
    memset(buf, 'A', 10);
    if (buf[0] != 'A') return 2;

    // memmove_chk — within bounds
    memmove(buf + 1, buf, 5);
    if (buf[1] != 'A') return 3;

    // memcmp_chk — within bounds
    if (memcmp(buf, "AAAAA", 5) != 0) return 4;

    // strcpy_chk — within bounds
    strcpy(buf, "test");
    if (buf[0] != 't') return 5;

    // strncpy_chk — within bounds
    strncpy(buf, "xyz", 4);
    if (buf[0] != 'x') return 6;

    // strlen_chk
    if (strlen(buf) != 3) return 7;

    // strcat_chk — within bounds
    strcpy(buf, "abc");
    strcat(buf, "def");
    if (buf[3] != 'd') return 8;

    // snprintf_chk
    snprintf(buf, 32, "%d", 42);
    if (buf[0] != '4') return 9;

    printf("PASS\n");
    return 0;
}
