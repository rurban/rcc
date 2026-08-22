/* GAS numeric local labels ("1:"/"2:", referenced backward as "1b"/"2b"
 * and forward as "1f"/"2f") assembled correctly for JMP/Jcc, but `call
 * 1b`/`call 2b` (a backward reference to an already-defined numeric
 * label) always fell through to a bogus R_X86_64_PLT32 relocation
 * against a literal, never-defined symbol named "1"/"2" -- an
 * undefined-reference link failure on every use.
 *
 * Root cause (src/asm.c, encode_x86()'s CALL handler): unlike JMP/Jcc
 * (which resolve any already-defined same-section backward label
 * unconditionally via `if (toff >= 0 && sec == as->cur_sec)`), CALL adds
 * an extra `is_local_sym` gate -- needed for one specific real case
 * (arch/x86/lib/retpoline.S's "call srso_safe_ret", a `.globl` label
 * that must still take the PLT32-reloc path despite being an
 * already-defined backward reference, since a preemptable global symbol
 * can never be folded to a same-section byte-patch). That gate checks
 * whether `ensure_sym()`'s symbol-table entry for the label's bare name
 * is bound SB_LOCAL. But GAS numeric local labels can never be `.globl`
 * -- they are a purely assembler-internal addressing mechanism with no
 * real entry under their bare digit name in the object's symbol table
 * at all (define_label() deliberately skips creating one for a numeric
 * label -- see its own comment). So `ensure_sym(as, "1")` always created
 * a *fresh* SB_GLOBAL/SEC_UNDEF entry, making is_local_sym unconditionally
 * false for every numeric-label call -- the srso_safe_ret distinction
 * this gate exists for is meaningless for numeric labels and must not
 * apply to them.
 *
 * Found via mbedtls's tf-psa-crypto/drivers/builtin/src/aesni.c: its
 * AES key-schedule inline-asm helpers (aesni_setkey_enc_128/192/256) use
 * exactly this "1: ... ret \n 2: ... call 1b \n call 1b \n ..." shape
 * (a shared subroutine repeatedly `call`ed via a numeric local label,
 * to expand each AES round key) -- every one of them failed to link:
 * "undefined reference to `1'".
 *
 * Fixed by treating a purely-numeric label (after strip_local_label_suffix
 * strips the b/f direction suffix) as always resolvable locally,
 * bypassing the is_local_sym gate -- matching JMP/Jcc's own unconditional
 * same-section-backward-reference handling.
 */
#include <stdio.h>

static int helper(int x) {
    int result;
    asm ("mov %1, %%eax   \n\t"
         "jmp 2f          \n\t"
         "1:              \n\t"
         "add $1, %%eax   \n\t"
         "ret             \n\t"
         "2:              \n\t"
         "call 1b         \n\t"
         "call 1b         \n\t"
         "call 1b         \n\t"
         "mov %%eax, %0   \n\t"
         : "=r" (result)
         : "r" (x)
         : "eax", "cc");
    return result;
}

int main(void) {
    int r = helper(10);
    printf("%d\n", r);
    return r == 13 ? 0 : 1;
}
