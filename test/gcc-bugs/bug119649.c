/* GCC Bug #119649 - internal compiler error: tree check: expected class 'type', have 'exceptional' (error_mark) in create_tmp_from_val, at gimplify.cc:621
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=119649
 */
/* { dg-do compile } */


int f(const char*);
int g(int args, int sum) {
//     sum +=
      f(__builtin_va_arg(args, char const *)); /* { dg-error "first argument to " } */
}
// ```


