/* GCC Bug #91554 - if (!__builtin_constant_p (fn_arg)) warning_function() works in inline when fn_arg is int, not when it is void *
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=91554
 */
/* { dg-do compile } */


extern void thefun_called_with_nonnull_arg (void)
    __attribute__((__warning__(
//         "'thefun' called with second argument not NULL")));

extern int real_thefun (void *, void *);

static inline int
// thefun (void *a, void *b)
{
   if (!__builtin_constant_p(b) || b != 0)
//        thefun_called_with_nonnull_arg();
   return real_thefun(a, b);
}

int warning_expected (void *a, void *b)
{
    return thefun(a, b);
}
int warning_not_expected (void *a)
{
    return thefun(a, 0);
}

// generates warnings from _both_ `warning_expected` and `warning_not_expected`, on all versions of GCC I can conveniently test (see <a href="https://godbolt.org/z/V-FHtZ">https://godbolt.org/z/V-FHtZ</a> ).  If I change the type of `b` to be `int` throughout, or if I convert the static inline to a macro

#define thefun(a, b) \
//   (((!__builtin_constant_p(b) || (b) != 0) \
//     ? thefun_called_with_nonnull_arg()     \
//     : (void) 0),                           \
//    real_thefun(a, b))


