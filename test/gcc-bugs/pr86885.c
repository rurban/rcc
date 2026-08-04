/* GCC Bug #86885 - gcc erroneously allows constructor/destructor attributes on nested functions
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86885
 */
/* { dg-do compile } */


if (TREE_CODE (decl) == FUNCTION_DECL
//       && TREE_CODE (type) == FUNCTION_TYPE
//       && decl_function_context (decl) == 0)
    {

// Which means this was supposed to be rejected (decl_function_context is supposed to return if the context of a function is a function or not) but maybe we set the DECL_CONTEXT of the function too late now.

// The decl_function_context check has been there since constructor attribute support was added by Jason in 1995 (<a href="https://gcc.gnu.org/cgit/gcc/commit/?id=2c5f4139a91db2">r0-8721-g2c5f4139a91db2</a>) . I am 90% sure this worked at one point but I have no way to test anything earlier than 4.1.2 though.


