/* GCC Bug #117810 - Feature request: attribute access but for (start, end) type interfaces
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117810
 */


for (i=0; i<n; ++i) { ... a[i] ... }
// n is not the index of the last element in the array but the one after, as counting starts with 0.
// Also, one thing I have wondered for a while now: Since the compiler can do constexpr stuff for C++, can't we leverage that in attributes? Allow little expressions maybe?
// If we had that, we could even do attributes declaring pre- and postconditions. That way we wouldn't need to have special attributes for things like "this pointer can't be NULL", that could be a simple expression.


