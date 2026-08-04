/* GCC Bug #44949 - extend Wparentheses from & to &=
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=44949
 */
/* { dg-do compile } */


case MODIFY_EXPR:
      if (!TREE_NO_WARNING (expr)
// 	  && warn_parentheses)
 {
// 	  warning (OPT_Wparentheses,
// 		   "suggest parentheses around assignment used as truth value");
   TREE_NO_WARNING (expr) = 1;
 }
      break;
// since it may be the point to catch most of:
  if (i&=2 == 0)
// but you may need to revert the hack done here:
  if (code == NOP_EXPR)
//     ret.original_code = MODIFY_EXPR;
//   else
    {
      TREE_NO_WARNING (ret.value) = 1;
//       ret.original_code = ERROR_MARK;
    }

// and devise a way to keep the info that this was originally more than an assignment operator (set ret.original_code to BIT_AND_EXPR, etc ?).


