/* GCC Bug #94106 - error on a function redeclaration with attribute transaction_safe
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=94106
 */
/* { dg-do compile } */


void f0 (void);
__attribute__ ((transaction_safe)) void f0 (void);       // (bogus?) error

                                     void f1 (void);
__attribute__ ((transaction_unsafe)) void f1 (void);     // accepted

                                       void f2 (void);
__attribute__ ((transaction_callable)) void f2 (void);   // accepted

//     2 | __attribute__ ((transaction_safe)) void f0 (void);       // (bogus?) error
//     1 |                                    void f0 (void);


