/* K&R-style unspecified-parameter declaration `int f();` promises
 * nothing about parameters (distinct from the explicit, zero-parameter
 * `int f(void);`), so a later definition taking real parameters is
 * legal C -- must NOT be flagged as a conflicting prototype. Both
 * forms leave the parsed parameter-type list empty, so the checker
 * needs Type::is_void_params to tell them apart; see the companion
 * error-expected case (`(void)` vs `(int)`) in
 * test_err_proto_void_params.c. */

int rcc_kr_ok();
int rcc_kr_ok(int a) { return a; }

int main(void) { return rcc_kr_ok(7) == 7 ? 0 : 1; }
