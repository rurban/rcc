/* An integer-typed (not pointer-typed) global — or struct member — whose
 * initializer is a cast address of another symbol, e.g.
 * "unsigned long x = (unsigned long)&some_array;", needs a real
 * relocation just like a pointer-typed global does: the address isn't
 * known until link time, so it can't be constant-folded, but it's not an
 * error either. The global-initializer code already handled this shape
 * for pointer-typed variables/members (extract_reloc() + append_reloc()),
 * both at file scope and nested inside a struct's designated initializer
 * — but the parallel scalar/integer code paths (both the top-level
 * global_initializer() and the nested-member global_init_one()) only
 * ever tried eval_const_expr()/eval_double_const_expr() and then gave up,
 * reporting "unsupported global initializer" or "expected constant
 * expression in initializer".
 *
 * Found via a real Linux kernel build: arch/x86/include/asm/processor.h's
 * INIT_THREAD macro (x86-64) is
 * "{ .sp = (unsigned long)&__top_init_kernel_stack }" — struct
 * thread_struct's sp field is a plain `unsigned long`, not a pointer —
 * used as init/init_task.c's init_task.thread field's initializer.
 *
 * "unsigned long" is deliberately part of the test: it's what the real
 * kernel source uses, and it's pointer-width on every target the Linux
 * kernel actually builds for. It is NOT pointer-width on LLP64 (Windows,
 * where "unsigned long" is 4 bytes) — real GCC itself rejects this exact
 * cast there as "initializer element is not constant" (confirmed against
 * x86_64-w64-mingw32-gcc), since a 4-byte field can't be guaranteed to hold
 * a real 64-bit address. This test is skipped on Windows for that reason,
 * not because rcc can't handle it there.
 */
#ifndef _WIN32
#include <stddef.h>

unsigned long backing_array[4];

/* File-scope integer global, not nested in a struct. */
unsigned long flat_addr = (unsigned long)&backing_array;

struct thread_struct {
    unsigned long sp;
};

struct task_struct {
    int x;
    struct thread_struct thread;
    int y;
};

/* The exact real-world shape: an integer field nested inside a struct's
 * designated initializer, holding a cast address of another global. */
struct task_struct init_task = {
    .x = 1,
    .thread = {
        .sp = (unsigned long)&backing_array,
    },
    .y = 2,
};

int main(void)
{
    if (flat_addr != (unsigned long)&backing_array) return 1;
    if (init_task.thread.sp != (unsigned long)&backing_array) return 2;
    if (init_task.x != 1 || init_task.y != 2) return 3;
    return 0;
}
#else
int main(void)
{
    return 0;
}
#endif
