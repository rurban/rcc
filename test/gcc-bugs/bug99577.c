/* GCC Bug #99577 - Non-constant (but actually constant) initializers referencing other constants no longer diagnosed as of GCC 8
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99577
 */
/* { dg-do compile } */


const int i = 0;
const int j = i;
// Up until GCC 7, this resulted in:
//   test.c:2:15: error: initializer element is not constant
//    const int j = i;
//                  ^
// As of GCC 8, no diagnostic is issued (this bug asks for one, e.g. under
// a new warning option, since the standard doesn't require a diagnostic
// but this code is not portable and other compilers reject it).

// As in the similar (and perhaps related?) bug #66618, the standard does
// not require a diagnostic for this code, but this code is not portable,
// it gets rejected by some other compilers, so an option in GCC to
// diagnose this would be useful.  This bug is the opposite of bug #53091,
// which asks for this to be accepted and was never updated after GCC
// started to accept it.  clang accepts this as well without diagnostic.


