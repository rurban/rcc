/* GCC Bug #87084 - Excessive diagnostic messages for invalid use of __builtin_va_arg_pack{,_len}() in a loop
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=87084
 */


int ignore)
{
// ...
    case BUILT_IN_VA_ARG_PACK:
//       /* All valid uses of __builtin_va_arg_pack () are removed during
//          inlining.  */
//       error ("%Kinvalid use of %<__builtin_va_arg_pack ()%>", exp);
      return const0_rtx;

    case BUILT_IN_VA_ARG_PACK_LEN:
//       /* All valid uses of __builtin_va_arg_pack_len () are removed during
//          inlining.  */
//       error ("%Kinvalid use of %<__builtin_va_arg_pack_len ()%>", exp);
      return const0_rtx;
// and diagnostics of valid/invalid uses of these builtins should really happen
// in the FE (they may be only used in variadic functions and those functions
// need to be marked always-inline either by the user or the frontend).
// So - marking as C-family FE bug.  We should reject this testcase and
// the above code should be changed to ICE.


