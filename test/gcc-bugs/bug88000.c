/* GCC Bug #88000 - Warn when different local vars regs order may produce different and so unsupported code [-Wasm-register-var]
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88000
 */


#include <stdio.h>

void bar(int x) {
//     printf("x = %d\n", x);
}

int main() {
//     // if we change the order of those two lines
//     // gcc will emit a proper `mov edi, 11`
//     // otherwise, it fails to do so since gcc 7.2
//     // when compiled with -O1/2/3/g
    register int b asm("r11") = 11;
    register int a asm("r12") = 22;

//     // also, if `b` is any of the registers that are
//     // supposed to be preserved by the called function,
//     // gcc will also emit proper code
//     // (so any of: rbp, rbx, r12-r15)
//     // the `a` register seem to not matter
//     bar(a);
//     bar(b);
}
// ```
// Tested locally on a GCC 7.3. The `main.c` is the program shown above, `main_rev.c` is the one with `b` and `a` variables definitions swapped:
// ```
// x = 22
// x = 582
// x = 22
// x = 11
// ```
// The difference can be seen in the disassembly:
// ```
// Dump of assembler code for function main:
//    0x000000000000068b <+0>:	sub    rsp,0x8
//    0x000000000000068f <+4>:	mov    edi,0x16
//    0x0000000000000694 <+9>:	call   0x66a <bar>
//    0x0000000000000699 <+14>:	mov    edi,r11d
//    0x000000000000069c <+17>:	call   0x66a <bar>
//    0x00000000000006a1 <+22>:	mov    eax,0x0
//    0x00000000000006a6 <+27>:	add    rsp,0x8
//    0x00000000000006aa <+31>:	ret
// End of assembler dump.
// Dump of assembler code for function main:
//    0x000000000000068b <+0>:	push   r12
//    0x000000000000068d <+2>:	mov    edi,0x16
//    0x0000000000000692 <+7>:	call   0x66a <bar>
//    0x0000000000000697 <+12>:	mov    edi,0xb
//    0x000000000000069c <+17>:	call   0x66a <bar>
//    0x00000000000006a1 <+22>:	mov    eax,0x0
//    0x00000000000006a6 <+27>:	pop    r12
//    0x00000000000006a8 <+29>:	ret
// End of assembler dump.
// ```
// As written in the comment, it seems that this change happens only if the `b` is assigned to a register that can be clobbered (and so different than rbp, rbx, r12-r15).


