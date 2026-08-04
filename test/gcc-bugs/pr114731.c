/* GCC Bug #114731 - -Wincompatible-pointer-types false positive in combination with _Generic(3)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=114731
 */
/* { dg-do compile } */


#define a2i(TYPE, ...)                                                        \
// (                                                                             \
// 	_Generic((TYPE) 0,                                                    \
// 		long:               a2sl(__VA_ARGS__),                        \
// 		int:                a2si(__VA_ARGS__),                        \
// 	)                                                                     \
// )
#define a2sl(n, s, endp, base, min, max)                                      \
// (                                                                             \
// 	_Generic(endp,                                                        \
  const char **:  a2sl_c,                                       \
  char **:        a2sl_nc,                                      \
  void *:         a2sl_c                                        \
// 	)(n, s, endp, base, min, max)                                         \
// )

#define a2si(n, s, endp, base, min, max)                                      \
// (                                                                             \
// 	_Generic(endp,                                                        \
  const char **:  a2si_c,                                       \
  char **:        a2si_nc,                                      \
  void *:         a2si_c                                        \
// 	)(n, s, endp, base, min, max)                                         \
// )
// As long as you only use one _Generic, it's easy, because you can pass the argument list outside of _Generic(), and it will call the right function.  However, if you want to call a macro (the second level), you must write the argument list inside the _Generic() switch.  This triggers warnings in all the non-evaluated expressions.

// These warnings should not be emitted, and only the evaluated expression in _Generic() should emit warnings about types, I think.
// Below is an entire reproducer program, and it's output:
#include <bsd/inttypes.h>
#include <errno.h>
#include <time.h>
#define a2i(TYPE, ...)                                                        \
// (                                                                             \
// 	_Generic((TYPE) 0,                                                    \
// 		long:               a2sl(__VA_ARGS__),                        \
// 		int:                a2si(__VA_ARGS__),                        \
// 	)                                                                     \
// )
#define a2sl(n, s, endp, base, min, max)                                      \
// (                                                                             \
// 	_Generic(endp,                                                        \
  const char **:  a2sl_c,                                       \
  char **:        a2sl_nc,                                      \
  void *:         a2sl_c                                        \
// 	)(n, s, endp, base, min, max)                                         \
// )

#define a2si(n, s, endp, base, min, max)                                      \
// (                                                                             \
// 	_Generic(endp,                                                        \
  const char **:  a2si_c,                                       \
  char **:        a2si_nc,                                      \
  void *:         a2si_c                                        \
// 	)(n, s, endp, base, min, max)                                         \
// )
static inline int a2sl_c(long *restrict n, const char *s,
    const char **restrict endp, int base, long min, long max);
static inline int a2si_c(int *restrict n, const char *s,
    const char **restrict endp, int base, int min, int max);

static inline int a2sl_nc(long *restrict n, const char *s,
    char **restrict endp, int base, long min, long max);
static inline int a2si_nc(int *restrict n, const char *s,
    char **restrict endp, int base, int min, int max);
static inline int
// a2sl_c(long *restrict n, const char *s,
    const char **restrict endp, int base, long min, long max)
{
 return a2sl_nc(n, s, (char **) endp, base, min, max);
}
static inline int
// a2si_c(int *restrict n, const char *s,
    const char **restrict endp, int base, int min, int max)
{
 return a2si_nc(n, s, (char **) endp, base, min, max);
}
static inline int
// a2sl_nc(long *restrict n, const char *s,
    char **restrict endp, int base, long min, long max)
{
 int  status;

// 	*n = strtoi(s, endp, base, min, max, &status);
	if (status != 0) {
  errno = status;
  return -1;
 }
 return 0;
}
static inline int
// a2si_nc(int *restrict n, const char *s,
    char **restrict endp, int base, int min, int max)
{
 int  status;

// 	*n = strtoi(s, endp, base, min, max, &status);
	if (status != 0) {
  errno = status;
  return -1;
 }
 return 0;
}
// int
// main(void)
{
 time_t  t;

// 	a2i(time_t, &t, "42", NULL, 0, 0, 10);
}
// g.c: In function ‘main’:
// g.c:96:21: warning: passing argument 1 of ‘a2si_c’ from incompatible pointer type [-Wincompatible-pointer-types]
//    96 |         a2i(time_t, &t, "42", NULL, 0, 0, 10);
//       |                     ^~
//       |                     |
      |                     time_t * {aka long int *}
// g.c:30:11: note: in definition of macro ‘a2si’
//    30 |         )(n, s, endp, base, min, max)                                         \
//       |           ^
// g.c:96:9: note: in expansion of macro ‘a2i’
//    96 |         a2i(time_t, &t, "42", NULL, 0, 0, 10);
//       |         ^~~
// g.c:54:22: note: expected ‘int * restrict’ but argument is of type ‘time_t *’ {aka ‘long int *’}
//    54 | a2si_c(int *restrict n, const char *s,
//       |        ~~~~~~~~~~~~~~^
// g.c:11:9: error: expected specifier-qualifier-list before ‘)’ token
//    11 |         )                                                                     \
//       |         ^
// g.c:96:9: note: in expansion of macro ‘a2i’
//    96 |         a2i(time_t, &t, "42", NULL, 0, 0, 10);
//       |         ^~~


