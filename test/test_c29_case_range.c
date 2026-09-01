/* C29 standardizes GCC's long-standing `case low ... high:` range
 * extension (`case-range-label`). rcc already accepted it unconditionally
 * as a GNU extension; this pins down boundary values, adjacency between
 * consecutive ranges, fallthrough, and default.
 */
#include "test_common.h"

static const char *classify(int c)
{
    switch (c) {
    case 0 ... 8:
        return "low";
    case 9 ... 9: /* single-value range, degenerate but legal */
        return "nine";
    case 10 ... 19:
        return "teens";
    case -5 ... -1:
        return "small-negative";
    default:
        return "other";
    }
}

static int fallthrough_sum(int c)
{
    int hit_range = 0, hit_next = 0;
    switch (c) {
    case 1 ... 3:
        hit_range = 1;
        /* fall through */
    case 4:
        hit_next = 1;
        break;
    default:
        break;
    }
    return hit_range * 10 + hit_next;
}

int main(void)
{
    struct { int c; const char *want; } cases[] = {
        {0, "low"}, {8, "low"}, {9, "nine"}, {10, "teens"}, {19, "teens"},
        {20, "other"}, {-1, "small-negative"}, {-5, "small-negative"},
        {-6, "other"}, {100, "other"},
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        const char *got = classify(cases[i].c);
        if (got != cases[i].want && __builtin_strcmp(got, cases[i].want) != 0) {
            printf("FAIL: classify(%d) = \"%s\", want \"%s\"\n", cases[i].c, got, cases[i].want);
            return (int)(i + 1);
        }
    }

    /* case-range fallthrough into the next label. */
    if (fallthrough_sum(2) != 11) {
        printf("FAIL: fallthrough_sum(2) = %d, want 11\n", fallthrough_sum(2));
        return 100;
    }
    if (fallthrough_sum(4) != 1) {
        printf("FAIL: fallthrough_sum(4) = %d, want 1\n", fallthrough_sum(4));
        return 101;
    }
    if (fallthrough_sum(9) != 0) {
        printf("FAIL: fallthrough_sum(9) = %d, want 0\n", fallthrough_sum(9));
        return 102;
    }

    return 0;
}
