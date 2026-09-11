/* csmith 1513742 (reduced_optmismatch_O1_79.c): `crc32_tab = crc;` where
 * crc32_tab is `static uint32_t crc32_tab[256]` -- an assignment to an
 * expression with array type. Arrays are non-modifiable lvalues in C
 * (C11 6.5.16p2/6.3.2.1p1); gcc rejects this with "assignment to
 * expression with array type". rcc's assign() accepted it silently and
 * generated some reinterpretation of the store, whose exact bytes
 * written differed between -O0 and -O1 (the reduced testcase's
 * "optmismatch" symptom) -- a compile-time diagnostic gap, not a
 * codegen bug: real C never reaches codegen with this construct.
 *
 * Fixed in assign() (parser.c): reject "=" when the already-typed lhs
 * is TY_ARRAY or TY_FUNC, matching gcc's diagnostic.
 *
 * test_err-style: this file must FAIL to compile. run_tests compiles
 * test_err_*.c expecting a compile error (see the harness's
 * "compile error" classification).
 */
static unsigned int crc32_tab[256];

static void crc32_byte(unsigned int crc) {
    crc32_tab = crc; /* array is not assignable */
}

int main(void) {
    crc32_byte(1);
    return 0;
}
