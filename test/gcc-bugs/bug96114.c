/* GCC Bug #96114 - ICE in make_ssa_name_fn, at tree-ssanames.c:279 since r7-536-g381cdae49785fc4b
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=96114
 */
/* { dg-do compile } */


int test_n;

test(int (*fn())[test_n]) { (*fn())[0]; }

main_fn() { test(main_fn); }

//     7 | main_fn() { test(main_fn); }
//       |                  void (*)()
// vlt_to_pointer.c:4:12: note: expected ‘int (* (*)())[(sizetype)(test_n)]’ but argument is of type ‘void (*)()’
//     4 | test(int (*fn())[test_n]) { (*fn())[0]; }
//     7 | main_fn() { test(main_fn); }


