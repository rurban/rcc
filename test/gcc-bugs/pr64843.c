/* GCC Bug #64843 - miscompilation of atomic_fetch_add on atomic pointer type
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=64843
 */


#include <stdatomic.h>
int *_Atomic p;
void f() { atomic_fetch_add(&p, 1); }
// gives
// 	pushq	%rbp
// 	movq	%rsp, %rbp
// 	lock addq	$1, p(%rip)
// 	popq	%rbp
// 	ret
// ... which is wrong; gcc should add 4 to p, not 1.

// C11's atomic_fetch_add seems very difficult to implement with GCC's current set of builtins (you could in principle use _Generic to detect whether you have an atomic integer type).

// To this end, Clang adds a __c11_atomic_fetch_add builtin which provides the correct C11 semantics (premultiply by sizeof(*x) for an atomic pointer type) of atomic_fetch_add_explicit. Clang's __c11_... builtins also enforce some of the other C11 rules; for instance, only pointers to _Atomic types are permitted, so they may be valuable to add to GCC independent of this bug.


