/* GCC Bug #83696 - false positive warning when [[fallthrough]] is inside of if statement
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=83696
 */
/* { dg-do compile } */


#pragma GCC diagnostic error "-Wimplicit-fallthrough"

// func(int i, int j)
{
    switch (i) {
        if ( i || j )
            return 1;
//             [[fallthrough]];
        return 0;
    }
}
// GNU C++14 (Homebrew GCC 7.2.0) version 7.2.0 (x86_64-apple-darwin15.6.0)
#include "..." search starts here:
#include <...> search starts here:
// GNU C++14 (Homebrew GCC 7.2.0) version 7.2.0 (x86_64-apple-darwin15.6.0)
// tmp.cpp: In function 'int func(int, int)':
         if ( i || j )


