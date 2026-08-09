/* rcc's assembler (assemble_inline(), src/asm.c) had no handling at all
 * for GAS-style C block comments -- only #, ;, and // line comments
 * were recognized. A block comment landed on its own physical line was
 * parsed as a real instruction whose mnemonic was literally the
 * comment's opening slash-star ("warning: unknown x86 instruction:
 * slash-star"), silently dropping every instruction after it in that
 * inline-asm block; a trailing block comment glued onto a real
 * instruction's operands (e.g. "movl $1, %eax" followed by a comment)
 * was fed straight into the operand parser as if it were part of the
 * operand text, corrupting or silently dropping the instruction.
 *
 * Found via test/third_party/test_qbe_simplecc: QBE's own generated
 * assembly annotates blocks with end-of-function block comments; every
 * function containing one lost or miscoded the instruction(s)
 * immediately following it.
 *
 * Fixed by stripping block comments (as a lexer pre-pass, before macro
 * expansion, mirroring real GAS's own pipeline order) in a new
 * strip_block_comments() helper, blanking commented bytes to spaces
 * while preserving embedded newlines so line-number tracking in the
 * per-line assembler loop stays accurate. The fix itself lives in the
 * shared, arch-independent part of assemble_inline() (it runs before
 * any x86/ARM64-specific instruction dispatch), but this test's own
 * inline-asm bodies are x86 AT&T syntax, so it's gated x86-only below
 * (matching every other x86-syntax inline-asm test in this suite,
 * e.g. test_asm_multi_output_clobber.c) and is a no-op on ARM64.
 */
int main(void) {
#if !defined(__aarch64__) && !defined(_M_ARM64)
    int x = 0;

    /* A block comment on its own line between two real instructions:
     * the instruction after it must not be dropped. */
    __asm__(
        "movl $1, %0\n"
        "/* a comment on its own line */\n"
        "addl $41, %0\n"
        : "+r"(x)
    );
    if (x != 42) return 1;

    /* A trailing block comment glued onto a real instruction's operands
     * must not corrupt the operand parse. */
    x = 0;
    __asm__(
        "movl $42, %0  /* trailing comment */\n"
        : "=r"(x)
    );
    if (x != 42) return 2;

    /* A block comment spanning multiple physical lines: everything
     * inside gets blanked, but code sharing the closing-delimiter's
     * line still executes, and line-number tracking survives the
     * embedded newlines. */
    x = 0;
    __asm__(
        "movl $1, %0\n"
        "/* multi\n"
        "   line\n"
        "   comment */ addl $41, %0\n"
        : "+r"(x)
    );
    if (x != 42) return 3;

#endif
    return 0;
}
