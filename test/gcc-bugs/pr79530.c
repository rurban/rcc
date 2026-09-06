/* GCC Bug #79530 - GCC segfault when calling weakref+alias functions within __transaction_atomic block
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79530
 */
/* { dg-do compile } */
/* The original defect (a compiler segfault, 2017, GCC 6.3.1) is
 * already fixed: current GCC gives a clean error instead of crashing
 * ("__transaction_atomic' without transactional memory support
 * enabled"). __transaction_atomic requires -fgnu-tm, a deprecated,
 * rarely-used GNU extension that Clang doesn't implement either
 * (errors "use of undeclared identifier '__transaction_atomic'",
 * identical in shape to rcc's rejection below). Not worth
 * implementing GNU TM support to reproduce an already-fixed crash in
 * an all-but-abandoned extension. */


static a () __attribute__ ((weakref ("")));
b ()
{
  __transaction_atomic { a (); }
}
// 0xa87b10 symtab_node::get_alias_target()
// 0xa87b10 cgraph_node::get_alias_target()


