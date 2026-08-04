/* GCC Bug #80398 - missing -Wattributes on a misplaced attribute packed in an enum definition
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80398
 */


__attribute__ ((packed)) enum E0 { e0 };   // attribute ignored, missing warning
// + cat a.c
__attribute__ ((packed)) enum E0 { e0 };
enum __attribute__ ((packed)) E1 { e1 }; 
enum E2 { e2 } __attribute__ ((packed));

#if __cplusplus
#  define A(e) static_assert (e, #e)
#else
#  define A(e) _Static_assert (e, #e)
#endif

A (sizeof (enum E0) == 1);
A (sizeof (enum E1) == 1);
A (sizeof (enum E2) == 1);
// + for lang in c c++
// + gcc -S -Wall -Wextra -Wpedantic -Wattributes -Wignored-attributes -xc a.c
// a.c:8:16: error: static assertion failed: "sizeof (enum E0) == 1"
 #  define A(e) _Static_assert (e, #e)
//                 ^
// a.c:11:1: note: in expansion of macro ‘A’
 A (sizeof (enum E0) == 1);
//  ^
// + for lang in c c++
// + gcc -S -Wall -Wextra -Wpedantic -Wattributes -Wignored-attributes -xc++ a.c
// a.c:1:31: warning: attribute ignored in declaration of ‘enum E0’ [-Wattributes]
 __attribute__ ((packed)) enum E0 { e0 };
//                                ^~
// a.c:1:31: note: attribute for ‘enum E0’ must follow the ‘enum’ keyword
// a.c:6:16: error: static assertion failed: sizeof (enum E0) == 1
 #  define A(e) static_assert (e, #e)
//                 ^
// a.c:11:1: note: in expansion of macro ‘A’
 A (sizeof (enum E0) == 1);
//  ^


