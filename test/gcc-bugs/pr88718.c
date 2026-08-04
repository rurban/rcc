/* GCC Bug #88718 - Strange inconsistency between old style and new style definitions of inline functions.
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88718
 */


static int x;
struct s { int a[sizeof(x)]; } inline *f (void) { return 0; }

// where the reference to x is part of the return type (still syntactically 
// part of the inline definition, so I think still included in what should be 
// diagnosed).  So it will be necessary to track references to identifiers 
// with internal linkage in such contexts which might or might not turn out 
// to be part of an inline definition - not just (for example) in what might 
// or might not be a function prototype scope for an inline function.


