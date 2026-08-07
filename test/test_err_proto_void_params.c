/* Prototype/definition redeclaration check must catch an arity conflict
 * between an explicit `(void)` (zero-parameter) prototype and a later
 * redeclaration that takes real parameters -- e.g. curl's `configure`
 * compiles exactly this to verify the compiler halts on function
 * prototype mismatch (m4/curl-compilers.m4,
 * CURL_CHECK_COMPILER_PROTOTYPE_MISMATCH):
 *
 *   #include <stdlib.h>      // declares `int rand(void);`
 *   int rand(int n);         // conflicting: void vs (int)
 *   int rand(int n) { ... }  // must also be flagged
 *
 * Both `()` (K&R-style, unspecified/any args) and `(void)` (explicit,
 * zero args) parse to an empty parameter-type list, so the checker
 * needs a way to tell them apart (Type::is_void_params) -- otherwise
 * neither this redeclaration nor a following definition is ever
 * compared against the original zero-arg prototype, and gcc/clang's
 * mismatch is silently accepted. See test_proto_void_params.c for the
 * companion case that must NOT be flagged (K&R-compatible `()`). */

int rcc_void_conflict(void);
int rcc_void_conflict(int n); /* error: conflicting types */

int main(void) { return 0; }
