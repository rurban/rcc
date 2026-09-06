/* GCC Bug #100993 - ICE with -O2: Segmentation fault, cgraph_update_edges_for_call_stmt(gimple*, tree_node*, gimple*)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100993
 */
/* { dg-do compile } */


int __builtin_acc_on_device(int dev) { return __builtin_acc_on_device(dev); }
// ```


