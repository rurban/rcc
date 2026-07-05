// Test: function-like macros in #if expressions
// Repro for preprocessor bug where ONE() expanded to 1 but
// #if ONE() evaluated to false.
#define ONE() 1

#if ONE()
// OK
#else
#error "FAIL: ONE() should be true"
#endif

#if !ONE()
#error "FAIL: !ONE() should be false"
#endif

#define TWO() 2
#if ONE() + ONE() != TWO()
#error "FAIL: ONE() + ONE() should equal TWO()"
#endif

int main(void) { return 0; }
