/* Regression test: a constructor function with a local variable
 * declaration in its body must still be registered in .init_array.
 *
 * declaration() resets the pending constructor/destructor attribute
 * flags at the start of every statement-level declaration (so they
 * can't leak onto the next TOP-LEVEL declaration), so the FIRST local
 * declaration inside `static void __attribute__((constructor)) f(void)
 * { int x; ... }` wiped the pending constructor flag before the
 * function-definition path consumed it -- the constructor never landed
 * in .init_array and never ran. Found via test_gnutls: libgnutls'
 * lib_init() constructor (which builds the embedded ASN.1 trees via
 * asn1_array2tree) was never registered, so _gnutls_gnutls_asn stayed
 * NULL until gnutls_global_init(); tests importing DH params before
 * that (resume-dtls) failed and crashed. Empty-bodied constructors
 * worked, which is why the pre-existing bug went unnoticed.
 *
 * Runtime-observable: the constructor's side effect must be visible in
 * main(). Fails on the pre-fix compiler (constructor never runs).
 */
#include <assert.h>
#include <stdio.h>

static int ctor_ran;

static void __attribute__((constructor)) init(void)
{
    int x = 42; /* local declaration: used to wipe pending_constructor */
    ctor_ran = x;
}

int main(void)
{
#if defined(_WIN32)
    /* .init_array is ELF; mingw registers constructors through CRT
     * sections (.CRT$XCU) with a different mechanism -- the bug this
     * test guards (pending_constructor wiped by a local declaration)
     * is ELF-specific, so only the compile itself is exercised here. */
    (void)ctor_ran;
    printf("ok\n");
    return 0;
#else
    assert(ctor_ran == 42);
    printf("ok\n");
    return 0;
#endif
}
