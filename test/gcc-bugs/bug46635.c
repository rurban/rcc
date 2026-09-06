/* GCC Bug #46635 - c-family/c-common.c uses BITS_PER_UNIT in lieu of TYPE_PRECISION (char_type_node)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=46635
 */
/* { dg-do compile } */


#include <stddef.h>

struct S { char a; int b; };

size_t off = offsetof(struct S, b);
