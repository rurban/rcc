/* GCC Bug #122732 - Add __builtin_assume_dereferenceable(a,N)  which tells the compiler *a ... *(a+(N-1)/sizeof(*a)) is not trapping
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=122732
 */


// clang added __builtin_assume_dereferenceable recently (<a href="https://github.com/llvm/llvm-project/pull/121789">https://github.com/llvm/llvm-project/pull/121789</a>).

// Basically it corresponds to the idea that `*a ... *(a+(N-1)/sizeof(*a))` will not trap.  This seems like a good hint for the vectorizer and CSelim and ifcvt (and other passes). How to communicate that information further down the IR is still an open question. But at least accepting the builtin is a good first step ...


