/* `return __builtin_bswapN(x);` as the whole body of a non-inlined function
 * was tail-jumped to a bare `__builtin_bswapN` symbol (undefined reference
 * at link) once a system header had already declared the builtin: the
 * ND_RETURN tail-call fast path only excluded __builtin_* by `funcname`,
 * but the callee is then an ND_LVAR. Seen in zstd's legacy MEM_swap32,
 * git's packfile.c, nettle, cgit. Needs <stdlib.h> (declares the builtin
 * through glibc's byteswap.h) and an address-taken wrapper at -O2.
 */
#include <stdio.h>
#include <stdlib.h>

static __inline __attribute__((unused)) unsigned sw32(unsigned in) { return __builtin_bswap32(in); }
static __inline __attribute__((unused)) unsigned long long sw64(unsigned long long in) { return __builtin_bswap64(in); }
static __inline __attribute__((unused)) unsigned short sw16(unsigned short in) { return __builtin_bswap16(in); }
unsigned (*volatile fp32)(unsigned) = sw32;
unsigned long long (*volatile fp64)(unsigned long long) = sw64;
unsigned short (*volatile fp16)(unsigned short) = sw16;

int main(void) {
    if (fp32(0x11223344u) != 0x44332211u || fp64(0x1122334455667788ull) != 0x8877665544332211ull ||
        fp16(0x1122) != 0x2211) {
        printf("FAIL\n");
        return 1;
    }
    printf("OK\n");
    return 0;
}
