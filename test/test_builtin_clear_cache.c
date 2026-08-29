/* __builtin___clear_cache(begin, end): flushes the instruction cache so
 * newly-written (JIT-generated) code is visible to the CPU's instruction
 * fetch unit.
 *
 * Regression: rcc had no special recognition for this builtin's exact
 * name at all, so a call to it fell through to an ordinary (never
 * defined anywhere) direct call and linked as "undefined reference to
 * `__builtin___clear_cache`".
 *
 * Fix: redirect to the real `__clear_cache(char*, char*)` runtime
 * function libgcc provides (rcc's linker driver invokes the system's
 * real `gcc` to link, which always pulls in libgcc). Verified against
 * real GCC on x86-64: a bare call compiles to nothing there (the
 * architecture's instruction cache is coherent) -- this test just
 * checks the call links and returns without touching memory it
 * shouldn't, since the actual cache-flush has no observable effect
 * from portable C.
 *
 * Found via a real PHP build: ext/opcache/jit/ir/ir.c's
 * ir_mem_flush(), part of PHP's opcache JIT.
 */
#include <stdio.h>
#include <string.h>

static void jit_write_and_flush(char *buf, int n)
{
    memset(buf, 0x90, (size_t)n); /* pretend to emit code */
    __builtin___clear_cache(buf, buf + n);
}

int main(void)
{
    char buf[64];
    jit_write_and_flush(buf, sizeof(buf));
    for (int i = 0; i < 64; i++)
        if ((unsigned char)buf[i] != 0x90) return 1;
    printf("OK __builtin___clear_cache\n");
    return 0;
}
