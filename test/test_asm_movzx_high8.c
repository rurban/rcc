/* movzbl/movsbl with a legacy high-byte source (%ah..%bh) emitted REX.B
 * (the pseudo-register number is > RDI), turning %ah into %r12b. Seen in
 * nettle's camellia-crypt-internal.asm (`movzbl %ah, %ebp`).
 */
#include <stdio.h>
#if defined(__x86_64__) || defined(_M_X64)

int main(void) {
    unsigned a = 0x00f234u, b = 0x00ee56u, c = 0x007f78u, d = 0x0080abu;
    unsigned za, zb, zc, zd;
    int sc, sd;
    __asm__("movzbl %%ah, %0" : "=S"(za) : "a"(a));
    __asm__("movzbl %%bh, %0" : "=D"(zb) : "b"(b));
    __asm__("movzbl %%ch, %0" : "=S"(zc) : "c"(c));
    __asm__("movzbl %%dh, %0" : "=D"(zd) : "d"(d));
    __asm__("movsbl %%ch, %0" : "=S"(sc) : "c"(c));
    __asm__("movsbl %%dh, %0" : "=D"(sd) : "d"(d));
    if (za != 0xf2 || zb != 0xee || zc != 0x7f || zd != 0x80 || sc != 0x7f || sd != -128) {
        printf("FAIL: %x %x %x %x %d %d\n", za, zb, zc, zd, sc, sd);
        return 1;
    }
    printf("OK\n");
    return 0;
}
#else
int main(void) {
    printf("OK (not x86-64)\n");
    return 0;
}
#endif
