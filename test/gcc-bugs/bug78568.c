/* GCC Bug #78568 - Wtype-limits warning regression
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=78568
 */
/* { dg-do compile } */


// For C++ this regressed as expected with <a href="https://gcc.gnu.org/viewcvs/gcc?view=revision&revision=230365">r230365</a> - in both cases delayed folding.
// Without delayed folding, e.g. (val16 & 0xffffffff) is folded into (unsigned int) val16 by the time shorten_compare is called and that is the routine that performs the warning.
// Do we perform similar optimizations that shorten_compare does already in match.pd or gimple-fold.c?
// Would there be a better place to move the warning to (like in c_genericize or so)?
// Or, if we want to warn in shorten_compare, should it be done twice, once on the unfolded values and then on folded values just for comparison purposes?
// Conceptually, shorten_compare (except for the part where it computes the common type for the comparison) is an optimization, kind of folding, so for delayed folding should not be done early.  But if e.g. constexpr evaluation or __builtin_constant_p etc. relies on it, it should conceptually be done at c_fully_fold time.  Though, I think the warning needs to be done before we shorten the compare and c_fully_fold shouldn't be performing warnings (because it can be called multiple times, e.g. when trying to fold something for other warnings).
// Thoughts on this?


