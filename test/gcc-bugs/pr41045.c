/* GCC Bug #41045 - Extended asm with C operands doesn’t work at top level
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=41045
 */


int a[2], b;
    enum { E1, E2, E3, E4, E5 };
    struct S { int a; char b; long long c; };
    asm (".section blah; .quad %P0, %P1, %P2, %P3, %P4; .previous"
//          : : "m" (a), "m" (b), "i" (42), "i" (E4), "i" (sizeof (struct S)));
//     Even for non-LTO, that could be useful e.g. for getting enumerators from
//     C/C++ as integers into the toplevel asm, or sizeof/offsetof etc.
//     The restrictions I've implemented are:
//     1) asm qualifiers aren't still allowed, so asm goto or asm inline can't be
//        specified at toplevel, asm volatile has the volatile ignored for C++ with
//        a warning and is an error in C like before
//     2) I see good use for mainly input operands, output maybe to make it clear
//        that the inline asm may write some memory, I don't see a good use for
//        clobbers, so the patch doesn't allow those (and of course labels because
//        asm goto can't be specified)
//     3) the patch allows only constraints which don't allow registers, so
//        typically "m" or "i" or other memory or immediate constraints; for
//        memory, it requires that the operand is addressable and its address
//        could be used in static var initializer (so that no code actually
//        needs to be emitted for it), for others that they are constants usable
//        in the static var initializers
//     4) the patch disallows + (there is no reload of the operands, so I don't
//        see benefits of tying some operands together), nor % (who cares if
//        something is commutative in this case), or & (again, no code is emitted
//        around the asm), nor the 0-9 constraints
//     Right now there is no way to tell the compiler that the inline asm defines
//     some symbol, that is implemented in a later patch, as : constraint.
//     Similarly, the c modifier doesn't work in all cases and the cc modifier
//     is implemented separately.
//     2024-12-05  Jakub Jelinek  <<a href="mailto:jakub@redhat.com">jakub@redhat.com</a>>
//             <a class="bz_bug_link 
//           bz_status_NEW "
//    title="NEW - Extended asm with C operands doesn’t work at top level"
//    href="show_bug.cgi?id=41045">PR c/41045</a>
//     gcc/
//             * output.h (insn_noperands): Declare.
//             * final.cc (insn_noperands): No longer static.
//             * varasm.cc (assemble_asm): Handle ASM_EXPR.
//             * lto-streamer-out.cc (lto_output_toplevel_asms): Add sorry_at
            for non-STRING_CST toplevel asm for now.
            * doc/extend.texi (Basic @code{asm}, Extended @code{asm}): Document
//             that extended asm is now allowed outside of functions with certain
//             restrictions.
//     gcc/c/
//             * c-parser.cc (c_parser_asm_string_literal): Add forward declaration.
//             (c_parser_asm_definition): Parse also extended asm without
//             clobbers/labels.
//             * c-typeck.cc (build_asm_expr): Allow extended asm outside of
//             functions and check extra restrictions.
//     gcc/cp/
//             * cp-tree.h (finish_asm_stmt): Add TOPLEV_P argument.
//             * parser.cc (cp_parser_asm_definition): Parse also extended asm
//             without clobbers/labels outside of functions.
//             * semantics.cc (finish_asm_stmt): Add TOPLEV_P argument, if set,
//             check extra restrictions for extended asm outside of functions.
//             * pt.cc (tsubst_stmt): Adjust finish_asm_stmt caller.
//     gcc/testsuite/
//             * c-c++-common/toplevel-asm-1.c: New test.
//             * c-c++-common/toplevel-asm-2.c: New test.
//             * c-c++-common/toplevel-asm-3.c: New test.


