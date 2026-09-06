/* GCC Bug #101358 - Warn when saving a pointer to an object with temporary lifetime
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=101358
 */
/* { dg-do compile } */


typedef struct {
    int x[1];
} foo;

foo f(void);

int g(void) {
    int *p = f().x;
    return *p;
}

// The g() function is always UB, since the return value of f() has temporary lifetime, so doing "return *p;" is dereferencing a pointer to an object whose lifetime has ended. (This is the case both before and after C11's change to temporary lifetime.) Since it's obvious at compile time that p can never be used safely, we should have a warning for it, similar to how we have -Wreturn-local-addr to catch mistakes like this function:

int *h(void) {
    int x;
    return &x;
}


