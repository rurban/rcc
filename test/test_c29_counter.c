/* __COUNTER__: expands to 0, 1, 2, ... incrementing on every use within
 * one translation unit -- widely relied on by metaprogramming libraries
 * (metalang99/datatype99) to synthesize unique identifiers. Not a C29
 * addition itself, but listed in the issue's C29 checklist since it's a
 * prerequisite many C29-era macro libraries assume; rcc already
 * implements it, this pins the behavior down.
 */
#include "test_common.h"

#define CAT_(a, b) a##b
#define CAT(a, b) CAT_(a, b)
#define UNIQUE_NAME(base) CAT(base, __COUNTER__)

int main(void)
{
    /* Sequential, zero-based, one increment per expansion. */
    int a = __COUNTER__;
    int b = __COUNTER__;
    int c = __COUNTER__;
    if (a != 0 || b != 1 || c != 2) {
        printf("FAIL: __COUNTER__ sequence = %d,%d,%d, want 0,1,2\n", a, b, c);
        return 1;
    }

    /* #ifdef / defined() must see it as a real macro. */
#ifndef __COUNTER__
#error "__COUNTER__ must be defined"
#endif
#if !defined(__COUNTER__)
#error "defined(__COUNTER__) must be true"
#endif

    /* Token-paste use: generates distinct identifiers per expansion
     * site, the library-macro idiom this exists for. */
    int UNIQUE_NAME(var_) = 111;
    int UNIQUE_NAME(var_) = 222;
    /* Expect var_3 and var_4 (counter was already at 3 after a/b/c). */
    if (var_3 != 111 || var_4 != 222) {
        printf("FAIL: token-pasted __COUNTER__ names wrong (var_3=%d var_4=%d)\n",
               var_3, var_4);
        return 2;
    }

    /* One more direct use continues from where token-pasting left it. */
    int d = __COUNTER__;
    if (d != 5) {
        printf("FAIL: __COUNTER__ after token-paste uses = %d, want 5\n", d);
        return 3;
    }

    return 0;
}
