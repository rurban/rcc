/* GCC Bug #87911 - OpenACC/OpenMP clauses parsing: comma operator vs. separator
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=87911
 */


void f()
    {
    #pragma omp parallel num_threads (3, 4)
      ;

    #pragma omp target device (5, 6)
      ;
    }
C:
//     ../oo.c: In function 'f':
//     ../oo.c:3:36: warning: left-hand operand of comma expression has no effect [-Wunused-value]
//         3 | #pragma omp parallel num_threads (3, 4)
//           |                                    ^
//     ../oo.c:6:29: error: expected ')' before ',' token
//         6 | #pragma omp target device (5, 6)
//           |                           ~ ^
//           |                             )
// C++:
//     ../oo.c: In function 'void f()':
//     ../oo.c:3:38: warning: left operand of comma operator has no effect [-Wunused-value]
//         3 | #pragma omp parallel num_threads (3, 4)
//           |                                      ^
//     ../oo.c:6:31: warning: left operand of comma operator has no effect [-Wunused-value]
//         6 | #pragma omp target device (5, 6)
//           |                               ^
// I see how this is happening in the front ends.
// Note that at least for OpenACC there are certain clauses that do need the comma as separator, for they accept several expressions: "wait (1, 2, 3)", for example.  Is there any good reason that the single-argument clauses should use the comma operator semantics (consistently)?  I have not verified the OpenACC/OpenMP standards, but I suppose user expectation would be to diagnose all these as syntax errors?  (Jakub?)

// (Also note how the caret location is different for C and C++, and not quite right for both.  But that seems to be a general problem, also seen for "return 1, 2;", for example.)


