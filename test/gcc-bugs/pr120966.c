/* GCC Bug #120966 - ++/-- for short still uses unsigned types even if sizeof(int) == sizeof(short)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=120966
 */


case PREINCREMENT_EXPR:
    case PREDECREMENT_EXPR:
    case POSTINCREMENT_EXPR:
    case POSTDECREMENT_EXPR:
      {
        tree type = TREE_TYPE (TREE_OPERAND (*expr_p, 0));
        if (INTEGRAL_TYPE_P (type) && c_promoting_integer_type_p (type))
          {
            if (!TYPE_OVERFLOW_WRAPS (type))
              type = unsigned_type_for (type);
            return gimplify_self_mod_expr (expr_p, pre_p, post_p, 1, type);
          }
        break;
      }
// ```
// c_promoting_integer_type_p returns true even if sizeof(type) == sizeof(int).
// Most likely this should have `(TYPE_PRECISION (type) != TYPE_PRECISION (integer_type_node))` added to it.
// This is a missed optimization in some cases since it is signed integer overflow is undefined and this just forces `short_var++` to being defined which is ok.


