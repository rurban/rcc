/* C11 6.9p3: a function identifier may have at most one external
 * definition (a body) per translation unit. rcc silently accepted a
 * second body for the same name (keeping/overwriting whichever
 * definition it happened to process), where every other C compiler
 * errors "redefinition of 'foo'". Real-world impact: GCC PR c/108964's
 * __attribute__((target(...))) multiversioning reproducer (unsupported
 * by GCC's own C front-end) must also be rejected by rcc, not silently
 * link with one definition winning.
 */
int foo(void) { return 1; }
int foo(void) { return 2; } /* must be a redefinition error */

int main(void) { return foo(); }
