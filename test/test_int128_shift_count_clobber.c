/* Regression test: a `__int128 << expr` / `__int128 >> expr` whose
 * shift-count `expr` is itself a register-hungry compile-time-constant
 * subexpression (e.g. involving unsigned division, which the x86-64
 * ISA mandates run through the `%rax`/`%rdx` register pair) silently
 * clobbered the __int128 operand's already-loaded value, producing a
 * WRONG result -- while the identical shift count read from a plain
 * `int` variable (or written as a literal) worked correctly.
 *
 * `gen_int128()`'s x86-64 SHL/SHR codegen (codegen.c) loaded the
 * 128-bit operand into the PHYSICAL registers `%rax`/`%rdx` (via
 * `asm_mov_mem_rax`/`asm_mov_mem8_rdx`, bypassing the VReg allocator's
 * own tracking entirely) BEFORE evaluating the shift-count RHS via
 * `gen(node->rhs)`. Nothing told the allocator those two physical
 * registers were "busy": if the shift count's own evaluation needed
 * `%rax`/`%rdx` as scratch -- most notably an x86 `div`/`idiv`
 * instruction, whose dividend the ISA REQUIRES to sit in `%rdx:%rax`
 * -- it silently overwrote the just-loaded 128-bit value before the
 * actual shift instructions ever consumed it. A plain `int` variable
 * or literal shift count never triggers this (no `gen()` call needed,
 * or a single simple load unlikely to need rax/rdx specifically), so
 * the bug stayed latent until a genuinely division-based shift count
 * was exercised. Fixed by evaluating the shift-count RHS and parking
 * it in `%ecx` FIRST, before loading the 128-bit operand into
 * `%rax`/`%rdx`.
 *
 * Found via a wide-`_BitInt`/`__int128` GCC torture test
 * (test/torture/pr85582-3.c) once `sizeof`'s own type was fixed to be
 * unsigned `size_t` (see test_sizeof_unsigned_type.c): the fix made a
 * `sizeof(...) * N / 2` shift-count subexpression use x86 unsigned
 * `div` for the first time, exposing this pre-existing latent
 * clobbering bug.
 */
#include <assert.h>
#include <stdio.h>

typedef unsigned __int128 U;

int main(void) {
    /* Minimal reproduction of the exact shape that broke: an inline
     * shift-count expression requiring a runtime unsigned division
     * (not foldable to a bare literal at parse time without -O1, since
     * opt.c's constant-fold pass only runs at -O1+), directly as the
     * `<<` operator's RHS -- as opposed to the identical value read
     * back from a separately-computed `int` variable, or a literal. */
    unsigned long n = 16, half_bits = 8; /* sizeof(__int128) * 8, /2 */
    U direct = (U)0x101 << (n * half_bits / 2 - 7);

    int shift_var = (int)(n * half_bits / 2 - 7);
    U via_var = (U)0x101 << shift_var;

    U via_literal = (U)0x101 << 57;

    assert(direct == via_var);
    assert(direct == via_literal);
    assert(via_var == via_literal);

    /* Same for right-shift. */
    U big = (U)1 << 100;
    U rdirect = big >> (n * half_bits / 2 - 7);
    int rshift_var = (int)(n * half_bits / 2 - 7);
    U rvia_var = big >> rshift_var;
    assert(rdirect == rvia_var);

    printf("OK\n");
    return 0;
}
