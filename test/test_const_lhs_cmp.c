// Regression: a comparison with a constant on the LHS (C == x, C != x) and a
// non-constant on the RHS fell through to the reg-reg compare path.  Under
// register pressure gen(lhs) materialised the constant, spilled it, and the
// rhs address computation reused the same register/spill slot, degenerating
// the compare into a self-compare (cmp %esi,%esi) that is always equal.
//
// This broke the toke.c identifier scanner in rcc-compiled perl5, whose
// is_WORD_BUT_NONCONT_safe macro is a long chain of `0xNN == s[i]` tests: every
// one collapsed to "always equal", so ordinary identifiers like foo() and
// __FILE__ were rejected as "\\w char that isn't valid in a name".
#include <assert.h>

unsigned char buf[8];

__attribute__((noinline))
static int classify(const unsigned char *s, const unsigned char *e) {
    // constant-lhs equalities in a nested-ternary chain (mirrors the macro),
    // enough operands to force spilling
    return ( (e > s) && ((e - s) >= 1) ) ? (
        ( 0xCD == s[0] ) ? 1
      : ( 0xD2 == s[0] ) ? 2
      : ( 0xE1 == s[0] ) ? ( ( (0xAA == s[1]) && (0xBE == s[2]) ) ? 3 : 0 )
      : ( 0xE2 == s[0] ) ? ( ( 0x83 == s[1] ) ? ( (0x9D == s[2]) ? 3 : 0 )
                           : ( 0x92 == s[1] ) ? ( (0xB6 == s[2]) ? 3 : 0 )
                           : 0 )
      : 0 ) : 0;
}

int main(void) {
    // ASCII bytes: none of the multibyte leads match, must return 0
    buf[0]='('; buf[1]='f'; buf[2]='_';
    assert(classify(buf, buf+3) == 0);
    buf[0]='f';  assert(classify(buf, buf+3) == 0);
    buf[0]='_';  assert(classify(buf, buf+3) == 0);
    buf[0]='A';  assert(classify(buf, buf+3) == 0);
    // a real matching lead still works
    buf[0]=0xCD; assert(classify(buf, buf+3) == 1);
    buf[0]=0xD2; assert(classify(buf, buf+3) == 2);
    return 0;
}
