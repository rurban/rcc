/* GCC Bug #98090 - ICE in simd_clone_adjust_argument_types, at omp-simd-clone.c:591
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=98090
 */
/* { dg-do compile } */


void f ();
#pragma omp declare simd
void f ()
{
  void f (int a);
}
//     6 | }
// 0x149960e expand_simd_clones(cgraph_node*)


