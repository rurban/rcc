// Regression: __builtin_va_arg_pack_len() was hardcoded to 0 in the parser
// instead of expanding to the count of the call site's trailing variadic args.
//
// glibc's _FORTIFY_SOURCE open()/openat() inlines gate the mode argument on
// this count (`if (__va_arg_pack_len() < 1) return __open_2(path, oflag);`).
// With a constant 0, open(path, O_CREAT|..., mode) always saw "0 varargs",
// routed to __open_2, dropped the mode, and glibc aborted at runtime with
// "invalid open call: O_CREAT or O_TMPFILE without mode" -- which broke every
// file-creating open in rcc-compiled perl5.
#include <assert.h>

extern int real_sink(const char *fmt, ...);

extern __inline__ __attribute__((__always_inline__, __gnu_inline__))
int sink(const char *fmt, ...) {
    if (__builtin_va_arg_pack_len() < 1)
        return -1000 - __builtin_va_arg_pack_len();
    return __builtin_va_arg_pack_len() * 100 + real_sink(fmt, __builtin_va_arg_pack());
}

int real_sink(const char *fmt, ...) { (void)fmt; return 7; }

int main(void) {
    assert(sink("x")          == -1000); // 0 trailing args
    assert(sink("x", 1)       == 107);   // 1
    assert(sink("x", 1, 2)    == 207);   // 2
    assert(sink("x", 1, 2, 3) == 307);   // 3
    return 0;
}
