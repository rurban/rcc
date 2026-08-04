/* GCC Bug #86983 - documentation inconsistent with always_inline diagnostics for computed goto
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86983
 */
/* { dg-do compile } */


case GIMPLE_GOTO:
      t = gimple_goto_dest (stmt);

//       /* We will not inline a function which uses computed goto.  The
//          addresses of its local labels, which may be tucked into
//          global storage, are of course not constant across
//          instantiations, which causes unexpected behavior.  */
      if (TREE_CODE (t) != LABEL_DECL)
        {
//           inline_forbidden_reason
//             = G_("function %q+F can never be inlined "
//                  "because it contains a computed goto");
//           *handled_ops_p = true;
          return t;
        }
// we have to assume that the goto is to labels of the function or functions
// inlined into it.  We do not seem to compute whether any of those escape
// (your testcase returns them though).
// So the wording is correct and the Labels-as-Values documentation is
// wrong.
// Of course if we get enough analysis GCC might decide to inline, all the
// wording always applies to a concrete implementation and is not to be
// treated as language specification.


