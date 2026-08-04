/* GCC Bug #105499 - inconsistency between -Werror=c++-compat and g++ in __extension__ block
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105499
 */


int *f (void *q)
{
  return __extension__ ({ int *p = q; p; });
}

// With GCC 11.3.0 under Debian (Debian package), I get the following:

// tst.c: In function ‘int* f(void*)’:
// tst.c:3:36: error: invalid conversion from ‘void*’ to ‘int*’ [-fpermissive]
//     3 |   return __extension__ ({ int *p = q; p; });
//       |                                    ^
//       |                                    |
//       |                                    void*
// so no errors with "gcc -Werror=c++-compat", but an error with g++. This is not consistent. Either this is regarded as a valid extension in C++ so that both should succeed, or this is not valid C++ code even in __extension__ so that both should fail.
// Same issue with various GCC versions from 4.9 to 11.3.0.
// AFAIK, the purpose of -Wc++-compat is to test whether code would still pass when replacing C compilation by C++ (there might be false positives or false negatives, but this should not be the case with the above example).

// FYI, I got the above issue while testing GNU MPFR (tested with -Werror=c++-compat first, and with g++ a bit later in a more extensive test).


