/* __COUNTER__ must exist as a macro so `#ifdef __COUNTER__` /
 * `defined(__COUNTER__)` see it: metalang99/datatype99 gate ML99_GEN_SYM
 * on exactly that guard, and rcc only handled __COUNTER__ as an
 * expansion keyword (kw_counter) without ever defining it — the guard
 * evaluated false, ML99_GEN_SYM was never defined, and datatype99's
 * util.c failed to compile with ML99_GEN_SYM(...) left unexpanded.
 *
 * Also check that expansion still yields the incrementing values (0, 1,
 * 2) per call, not the placeholder 1 the macro-table entry carries.
 */
int x = 0;
#ifdef __COUNTER__
int c0 = __COUNTER__;
int c1 = __COUNTER__;
int c2 = __COUNTER__;
#else
int c0 = 0, c1 = 0, c2 = 0;
#endif

int main(void) {
#if !defined(__COUNTER__)
    return 1; /* guard must be true */
#else
    /* distinct per call, starting at 0 */
    return (c0 == 0 && c1 == 1 && c2 == 2) ? 0 : 1;
#endif
}
