/* GCC Bug #109828 - static compound literal with flexible array in initializer leads to invalid size and ICE
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=109828
 */
/* { dg-do compile } */
/* { dg-options "-std=gnu23 -O2" } */

#include <stddef.h>

struct s { int i; char c[]; };

const struct s s = { .c = "0", };
const struct s *r = &(constexpr struct s) { .c = "1", };
const struct s *t = &(static struct s) { .c = "2", };

/* Adding a call to __builtin_object_size() with optimization used to
 * trigger an ICE in component_ref_size/decl_init_size (gimplify_expr
 * backtrace in the bug).  Modern gcc emits "negative .space" assembler
 * warnings instead - same invalid flexible-array size handling. */
size_t ice(void)
{
    return __builtin_object_size(t, 0);
}