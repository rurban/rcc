/* GCC Bug #80400 - missing -Wattributes on a invalid attribute packed on a typedef
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80400
 */


typedef struct __attribute__ ((packed)) S S1;   // missing warning in C
// + cat a.c
struct S { int i; char c; };

typedef __attribute__ ((packed)) struct S S0;
typedef struct __attribute__ ((packed)) S S1;   // missing warning in C
typedef struct S __attribute__ ((packed)) S2;
typedef struct S S3 __attribute__ ((packed));

#if __cplusplus
#  define A(e) static_assert(e, #e)
#else
#  define A(e) _Static_assert(e, #e)
#endif

A (sizeof (S0) == 1);
A (sizeof (S1) == 1);
A (sizeof (S2) == 1);
A (sizeof (S3) == 1);
// + for lang in c c++
// + gcc -S -Wall -Wextra -Wpedantic -Wattributes -Wignored-attributes -xc a.c
// a.c:3:41: warning: ‘packed’ attribute ignored [-Wattributes]
 typedef __attribute__ ((packed)) struct S S0;
//                                          ^
// a.c:5:16: warning: ‘packed’ attribute ignored [-Wattributes]
 typedef struct S __attribute__ ((packed)) S2;
//                 ^
// a.c:6:16: warning: ‘packed’ attribute ignored [-Wattributes]
 typedef struct S S3 __attribute__ ((packed));
//                 ^
// a.c:11:16: error: static assertion failed: "sizeof (S0) == 1"
 #  define A(e) _Static_assert(e, #e)
//                 ^
// a.c:14:1: note: in expansion of macro ‘A’
 A (sizeof (S0) == 1);
//  ^
// a.c:11:16: error: static assertion failed: "sizeof (S1) == 1"
 #  define A(e) _Static_assert(e, #e)
//                 ^
// a.c:15:1: note: in expansion of macro ‘A’
 A (sizeof (S1) == 1);
//  ^
// a.c:11:16: error: static assertion failed: "sizeof (S2) == 1"
 #  define A(e) _Static_assert(e, #e)
//                 ^
// a.c:16:1: note: in expansion of macro ‘A’
 A (sizeof (S2) == 1);
//  ^
// a.c:11:16: error: static assertion failed: "sizeof (S3) == 1"
 #  define A(e) _Static_assert(e, #e)
//                 ^
// a.c:17:1: note: in expansion of macro ‘A’
 A (sizeof (S3) == 1);
//  ^
// + for lang in c c++
// + gcc -S -Wall -Wextra -Wpedantic -Wattributes -Wignored-attributes -xc++ a.c
// a.c:3:43: warning: ‘packed’ attribute ignored [-Wattributes]
 typedef __attribute__ ((packed)) struct S S0;
//                                            ^~
// a.c:4:41: warning: attributes ignored on elaborated-type-specifier that is not a forward declaration [-Wattributes]
 typedef struct __attribute__ ((packed)) S S1;
//                                          ^
// a.c:5:43: warning: ‘packed’ attribute ignored [-Wattributes]
 typedef struct S __attribute__ ((packed)) S2;
//                                            ^~
// a.c:6:44: warning: ‘packed’ attribute ignored [-Wattributes]
 typedef struct S S3 __attribute__ ((packed));
//                                             ^
// a.c:9:16: error: static assertion failed: sizeof (S0) == 1
 #  define A(e) static_assert(e, #e)
//                 ^
// a.c:14:1: note: in expansion of macro ‘A’
 A (sizeof (S0) == 1);
//  ^
// a.c:9:16: error: static assertion failed: sizeof (S1) == 1
 #  define A(e) static_assert(e, #e)
//                 ^
// a.c:15:1: note: in expansion of macro ‘A’
 A (sizeof (S1) == 1);
//  ^
// a.c:9:16: error: static assertion failed: sizeof (S2) == 1
 #  define A(e) static_assert(e, #e)
//                 ^
// a.c:16:1: note: in expansion of macro ‘A’
 A (sizeof (S2) == 1);
//  ^
// a.c:9:16: error: static assertion failed: sizeof (S3) == 1
 #  define A(e) static_assert(e, #e)
//                 ^
// a.c:17:1: note: in expansion of macro ‘A’
 A (sizeof (S3) == 1);
//  ^


