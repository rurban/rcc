/* GCC Bug #79530 - GCC segfault when calling weakref+alias functions within __transaction_atomic block
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79530
 */


static a () __attribute__ ((weakref ("")));
// b ()
{
  __transaction_atomic { a (); }
}
 }
// 0xa87b10 symtab_node::get_alias_target()
// 0xa87b10 cgraph_node::get_alias_target()


