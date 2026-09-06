/* GCC Bug #92826 - Impossible to silence warning: non-standard suffix on floating constant [-Wpedantic]
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=92826
 */


int main()
{
    if (1.0Q > 0)
        return 0;
    else
        return 1;
}

// GCC warns here with -Wpedantic: "non-standard suffix on floating
// constant [-Wpedantic]", and there is no way to silence it (not even
// __extension__ or #pragma GCC system_header, because the warning's
// location info is wrong - it points at the whole expression instead of
// the numeric token). The same false-positive warning also fires on
// constants like FLT128_MIN from <quadmath.h>:
//
//   #include <quadmath.h>
//   int main()
//   {
//       if (1.0 > FLT128_MIN)
//           return 0;
//       else
//           return 1;
//   }
//
// GCC should definitely not warn when using constants from <quadmath.h>.
// GCC should also provide an option to disable these warnings (e.g.
// -Wno-non-standard-suffix). People are currently using disgusting hacks
// to avoid these warnings so please add an option for that.
