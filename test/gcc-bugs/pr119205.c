/* GCC Bug #119205 - internal compiler error: in tree_to_uhwi, at tree.cc:6587
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=119205
 */
/* { dg-do compile } */


struct flexible {
  int length;
  int data[];
};

struct flexible x
= {.data = { [0x1ffffffffffffffffwb] = 0 }};
// ```


