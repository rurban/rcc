/* GCC Bug #52534 - gcc doesn't detect incorrect expression in call to va_start
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=52534
 */
/* { dg-do compile } */

#include <stdarg.h>

int maxof(int, ...) ;
void f(void);

int maxof(int n_args, ...){
    register int i;
    int max, a;
    va_list ap;
    va_start(ap, (unsigned int)n_args); /* { dg-warning "additional arguments other than identifier" } */
    max = va_arg(ap, int);
    for(i = 2; i <= n_args; i++) {
       if((a = va_arg(ap, int)) > max) max = a;
    }
    va_end(ap);
    return max;
}
// The C standard is very clear that the second argument to va_start is a
// parameter identifier, not an expression.  The Intel compiler finds the
// bug:
// vaarg.c(12): error: incorrect use of va_start
//       va_start(ap, (unsigned int)n_args);
//       ^
// GCC only started diagnosing this (as -Wvarargs) for C23 and later,
// because of a late change to C23 that requires the argument to be the
// identifier of the last named parameter; earlier standards permit any
// expression there since it is simply discarded.
