/* GCC Bug #85185 - Wider-than-expected load for struct member used as operand of inline-asm with memory clobber at -Og
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=85185
 */


static inline void foo(unsigned short n)
{
  __asm__("foo %0" :: "r" (n));
}

void bar(void)
{
//   foo(0xfffc);
}
//         .file   "x.c"
//         .option nopic
//         .text
//         .align  2
//         .globl  bar
//         .type   bar, @function
bar:
//         li      a5,-4
 #APP
# 3 "x.c" 1
//         foo a5
# 0 "" 2
 #NO_APP
//         ret
//         .size   bar, .-bar
//         .ident  "GCC: (GNU) 7.2.0"
// ;; Function bar (bar, funcdef_no=1, decl_uid=1412, cgraph_uid=1, symbol_order=1)
// ;; Generating RTL for gimple basic block 2
// try_optimize_cfg iteration 1
// Merging block 3 into block 2...
// Merged blocks 2 and 3.
// Merged 2 and 3 without moving.
// Merging block 4 into block 2...
// Merged blocks 2 and 4.
// Merged 2 and 4 without moving.
// try_optimize_cfg iteration 2
// ;;
// ;; Full RTL generated for this function:
// ;;
// (note 1 0 3 NOTE_INSN_DELETED)
// (note 3 1 2 2 [bb 2] NOTE_INSN_BASIC_BLOCK)
// (note 2 3 5 2 NOTE_INSN_FUNCTION_BEG)
// (insn 5 2 6 2 (set (reg:HI 72)
//         (const_int -4 [0xfffffffffffffffc])) "x.c":3 -1
//      (nil))
// (insn 6 5 0 2 (asm_operands/v ("foo %0") ("") 0 [
//             (reg:HI 72)
//         ]
//          [
//             (asm_input:HI ("r") x.c:3)
//         ]
//          [] x.c:3) "x.c":3 -1
//      (nil))
// I don't really know how to read these RTL dumps, so I can't say for sure, but my guess is that this is probably another manifestation of the same underlying problem.  Jim (or anyone else with input), can you confirm that's the case?
// Unlike the original case, this one, for what it's worth, also occurs at -O1, -O2, and -O3 in addition to -Og.


