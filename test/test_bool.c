// from https://en.cppreference.com/c/language/bool_constant
#include <assert.h>

// Regression: comparisons with negative constants, represented as
// ND_NEG(ND_NUM(C)), must fold to cmp $imm rather than emitting a
// self-compare (mov $1; neg; cmp %reg,%reg) that always produces
// a constant boolean.  Seen as segfaults in rcc-compiled perl5
// (mro_core.c:Perl_mro_method_changed_in, comparing struct fields
// against -1 sentinel values).
int cmp_neg_one(int x) {
    if (x == -1) return 1;
    if (x != -1) return 2;
    return 0;
}
int cmp_lt_neg_one(int x) {
    if (x < -1) return 10;
    if (x <= -1) return 20;
    return 0;
}
int cmp_neg_two(int x) {
    if (x == -2) return 100;
    return 0;
}

void run_neg_regressions(void) {
    assert(cmp_neg_one(-1) == 1);
    assert(cmp_neg_one(0) == 2);
    assert(cmp_neg_one(5) == 2);
    assert(cmp_lt_neg_one(-2) == 10);
    assert(cmp_lt_neg_one(-1) == 20);
    assert(cmp_lt_neg_one(0) == 0);
    assert(cmp_neg_two(-2) == 100);
    assert(cmp_neg_two(-1) == 0);
}

int main()
{
    assert(true == 1 && 0 == false);
    run_neg_regressions();
}
