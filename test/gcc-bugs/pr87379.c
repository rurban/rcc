/* GCC Bug #87379 - Warn about function pointer casts which differ in variadic-ness [-Wcast-variadic-function-type]
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=87379
 */


int open (const char *, int, ...);
int (*ptr1) (const char *, int) = (int (*) (const char *, int)) open;
int (*ptr2) (const char *, int, short)
//   = (int (*) (const char *, int, short)) open;
// But perhaps we should not warn for this because the variadic nature is preserved in the non-prototyped function pointer:
void (*ptr3) () = (void (*) ()) open;
// Whether we want to warn for this up to debate because these casts are clearly different, so it is more likely that the programmer knows what they are doing:
void (*ptr4) (double) = (void (*) (double) open;
// Since multiple targets are affected, I think it makes sense to warn for this issue independently of the current target.
// Calls with variadic-ness mismatches cause bugs which can be very difficult to track down.  The bug I particularly remember is this one:
// commit c7774174beffe9a8d29dd4fb38bbed43ece1cecd
// Author: Andreas Schneider <<a href="mailto:asn@samba.org">asn@samba.org</a>>
// Date:   Wed Aug 2 13:21:59 2017 +0200
//     swrap: Fix prototype of open[64] to prevent segfault on ppc64le
//     The calling conventions for vaarg are different on ppc64le. The patch
//     fixes segfaults on that platform.
// <<a href="https://git.samba.org/?p=socket_wrapper.git;a=commitdiff;h=c7774174beffe9a8d29dd4fb38bbed43ece1cecd">https://git.samba.org/?p=socket_wrapper.git;a=commitdiff;h=c7774174beffe9a8d29dd4fb38bbed43ece1cecd</a>>

// Here the problematic pointer came through dlopen, so a warning for casts would not have helped.  It was difficult to diagnose because the lack of the parameter save area (which was supposed by the called function to be allocated by the caller) caused stack corruption a couple of frames up the stack, in a fairly different area of the code.


