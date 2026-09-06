/* GCC Bug #100854 - TS 18661-3 and backwards-incompatible setting of __FLT_EVAL_METHOD__
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100854
 */
/* { dg-do compile } */


#define __FLT_EVAL_METHOD__ 0
#define __FLT_EVAL_METHOD_TS_18661_3__ 0
#define __FLT_EVAL_METHOD_C99__ 0
// Adding fp16 to the -march, we see that all three of these macros take the value 16:
#define __FLT_EVAL_METHOD__ 16
#define __FLT_EVAL_METHOD_TS_18661_3__ 16
#define __FLT_EVAL_METHOD_C99__ 16
// This is a little surprising. Based on the name of __FLT_EVAL_METHOD_C99__, you might expect it to only take values defined by C99.
// Forcing -std=c99, we see that __FLT_EVAL_METHOD__ itself takes a C99-conforming value, but the others do not:
#define __FLT_EVAL_METHOD__ 0
#define __FLT_EVAL_METHOD_TS_18661_3__ 16
#define __FLT_EVAL_METHOD_C99__ 16
// It seems that the behaviour of __FLT_EVAL_METHOD_C99__ is the exact opposite of what the name suggests.
// Notably the __FLT_EVAL_METHOD_C99__ macro is AArch64-specific. It isn't implemented on the arm port:
#define __FLT_EVAL_METHOD__ 0
#define __FLT_EVAL_METHOD_TS_18661_3__ 0
#define __FLT_EVAL_METHOD__ 16
#define __FLT_EVAL_METHOD_TS_18661_3__ 16
#define __FLT_EVAL_METHOD__ 0
#define __FLT_EVAL_METHOD_TS_18661_3__ 16

// It would be useful if GCC provided a portable pre-defined __FLT_EVAL_METHOD__ variant that was guaranteed to only take values defined by C99/C11. As it stands, GCC with -march=armv8.2-a+fp16 (or any -mcpu/-march that implies fp16) on arm and aarch64 fails to compile any file that includes newlib's math.h.

// This could be considered a bug in TS 18661-3 which stipulates that __FLT_EVAL_METHOD__ take backwards-incompatible values. Either way, it seems that GCC should provide a way to recover a conforming __FLT_EVAL_METHOD__ without forcing the user to compile everything in a strict standards-conforming mode (-std=c{99,11}).
// At a minimum, the __FLT_EVAL_METHOD_C99__ builtin macro should probably be removed from the AArch64 backend as its current behaviour is entirely unhelpful.
// Ideally, GCC would define a new macro (portable across all architectures implementing fp16) which is guaranteed to only take values defined by C99/C11.


