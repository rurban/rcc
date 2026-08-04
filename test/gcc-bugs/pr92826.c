/* GCC Bug #92826 - Impossible to silence warning: non-standard suffix on floating constant [-Wpedantic]
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=92826
 */


int main()
{
    if (1.0Q > 0)
        return 0;
        return 1;
}

  if (1.0Q > 0)
#include <quadmath.h>

int main()
{
    if (1.0 > FLT128_MIN)
        return 0;
        return 1;
}

     if (1.0 > FLT128_MIN)

// GCC should definitely not warn when using constants from <quadmath.h>. GCC should also provide an option to disable these warnings (e.g. -Wno-non-standard-suffix). People are currently using disgusting hacks to avoid these warnings so please add an option for that.


