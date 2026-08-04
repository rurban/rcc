/* GCC Bug #59159 - Need opaque pass-through as optimization barrier
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=59159
 */


// (category:c because I am asking for a built-in, but this is more about middle/back-end)

// Since gcc doesn't support FENV, I often need to insert optimization barriers in my code that hide things from gcc optimizers, so it doesn't perform constant propagation, or replace x*-y with -x*y, or move operations across fesetround, etc. Note that speed is still very important, doing a store/read on a volatile variable (the most portable optimization barrier) can make the whole application 15% slower.
// One common way to do that is to use an inline asm like:
asm volatile ("", "+g"(x));
// (4.9 is the first version where this doesn't ICE all over the place on x86, use "+m" earlier)
// so no code is emitted but the value of x is hidden from the compiler. However, the "g" constraint doesn't include FP registers. Depending if the code is compiled for power, sparc, itanium, arm or x86_64, I need to add either "d", "e", "f", "w", or "x" (yeah, there aren't 2 using the same letter). It looks like there isn't even a letter for x87 (<a class="bz_bug_link 
//           bz_status_RESOLVED  bz_closed"
//    title="RESOLVED INVALID - "+f" constraint rejected: output constraint 0 must specify a single register"
//    href="show_bug.cgi?id=59157">PR 59157</a>). Using the more general "+X" is tempting but ICEs (<a class="bz_bug_link 
//           bz_status_RESOLVED  bz_closed"
//    title="RESOLVED DUPLICATE - ICE: in reg_overlap_mentioned_p, at rtlanal.c:1473"
//    href="show_bug.cgi?id=59155">PR 59155</a>).
// There is at least one case this inline asm approach can never handle optimally: constants. We need the variable as asm output or it won't have any effect. But the constants are in a read-only location that is not suitable for an output operand, so with "+m" gcc copies the constant to another memory location.
// Ideally, there would be a __builtin_opaque built-in that does the right thing and I wouldn't have to think about it...


