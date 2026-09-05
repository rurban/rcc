/* GCC Bug #86885 - gcc erroneously allows constructor/destructor attributes on nested functions
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86885
 */
/* { dg-do compile } */

#include <stdio.h>

__attribute__((constructor(102))) void global_constructor_prio102(void) { printf("%s\n", __func__); }
__attribute__((constructor())) void global_constructor(void) { printf("%s\n", __func__); }
__attribute__((constructor(101))) void global_constructor_prio101(void) { printf("%s\n", __func__); }

int main(int argc, char **argv)
{
    printf("%s - gcc %s\n", __func__, __VERSION__);

    __attribute__((constructor(102))) void nested_constructor_prio102(void) { printf("%s\n", __func__); }
    __attribute__((constructor())) void nested_constructor(void) { printf("%s\n", __func__); }
    __attribute__((constructor(101))) void nested_constructor_prio101(void) { printf("%s\n", __func__); }

    return 0;
}
// Nested functions cannot meaningfully be called outside of the lifetime of a particular
// instance of the block in which they are nested. However, gcc allows them to be given the
// constructor or destructor attribute, causing them to be called at init/fini time without
// a meaningful value for the hidden context pointer, potentially leading to runaway wrong
// behavior. Applying the ctor/dtor attributes to a nested function should be an error.
//
// GCC's own c-decl.c has this check in start_decl / grokdeclarator, which is supposed to
// reject the attributes but apparently does not anymore:
//   if (TREE_CODE (decl) == FUNCTION_DECL
//       && TREE_CODE (type) == FUNCTION_TYPE
//       && decl_function_context (decl) == 0)
//     {
// Which means this was supposed to be rejected (decl_function_context is supposed to
// return if the context of a function is a function or not) but maybe we set the
// DECL_CONTEXT of the function too late now.
