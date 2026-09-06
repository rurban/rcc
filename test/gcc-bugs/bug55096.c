/* GCC Bug #55096 - Wconversion-nul does not work in C
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=55096
 */
/* { dg-do compile } */


#include <stdbool.h>
bool foo(void);
bool * ProcessRequest(bool *charge_acct) {
  if (foo()) {
    charge_acct = false;
  }
  return charge_acct;
}
// g++
// test2.cc:5:17: warning: converting ‘false’ to pointer type ‘bool*’ [-Wconversion-null]
     charge_acct = false;
//                  ^
// clang/clang++
// test2.cc:5:19: warning: initialization of pointer of type 'bool *' to null from a constant boolean expression [-Wbool-conversion]
    charge_acct = false;
//                   ^~~~~


