/* GCC Bug #110976 - Cryptic error message when incorrectly dereferencing a pointer in an anonymous union
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=110976
 */
/* { dg-do compile } */


struct S { union { struct S* x; }; };
    struct S* f(struct S s)
    {
      return s.x.x;
    }
// anon_union.c:4:13: error: 's.<U27b8>.x' is a pointer; did you mean to use '->'?
//     4 |   return s.x.x;
// GNU C17 (GCC) version 14.0.0 20230806 (experimental) (x86_64-pc-linux-gnu)
#include "..." search starts here:
#include <...> search starts here:


