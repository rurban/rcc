/* GCC Bug #110947 - Should -Wmissing-variable-declarations not trigger on register variables?
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=110947
 */
/* { dg-do compile } */


extern unsigned long current_stack_pointer;
// before the
register unsigned long current_stack_pointer asm("rsp");
// but that seems excessive. Perhaps we can simply not diagnose in that case?
// Filed this bug report against clang as well: <a href="https://github.com/llvm/llvm-project/issues/64509">https://github.com/llvm/llvm-project/issues/64509</a>


