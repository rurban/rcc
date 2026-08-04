/* GCC Bug #104171 - ICE: 'verify_gimple' failed. verify_gimple_in_seq; gimplify_body
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=104171
 */
/* { dg-do compile } */


// /* Fold a call to built-in function FNDECL with 1 argument, ARG0.
//    This function returns NULL_TREE if no simplification was possible.  */
static tree
// fold_builtin_1 (location_t loc, tree expr, tree fndecl, tree arg0)
{     
// ...
//     CASE_FLT_FN (BUILT_IN_CIMAG):
      if (validate_arg (arg0, COMPLEX_TYPE)
//           && TREE_CODE (TREE_TYPE (TREE_TYPE (arg0))) == REAL_TYPE)
        return non_lvalue_loc (loc, fold_build1_loc (loc, IMAGPART_EXPR, type, arg0));

// we ultimatively arrive here from fold_call_expr (and c_fully_fold_internal)
// where we could do more extensive validation.  In particular the CALL_EXPR
// has &cimagl of unprototyped type and we could also employ checks like those
// done in gimple_builtin_call_types_compatible_p.
// Interestingly for the following testcase we end up _not_ setting
// DECL_FUNCTION_CODE on cimagl and thus we do not ICE.  IMHO a frontend problem.
ong double cimagl(_Complex);
test_cimagl(_Complex z) { return cimagl(z); }


