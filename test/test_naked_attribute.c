/* __attribute__((naked)): GCC emits no prologue/epilogue, the body is basic
 * asm only. rcc ignored it and wrapped the asm in a frame (push %rbp / sub
 * $N,%rsp ... ret), so micropython's nlr_push -- which records the caller's
 * registers and tail-jumps to a C helper -- returned through the wrong frame
 * and crashed (mpy-cross died in nlr_pop on the first compiled file).
 *
 * push() below is the same shape: it saves the return address and rbp into
 * the caller's buffer, then `jmp push_tail`, a C function in the same file.
 * Run with a frame in between, the helper's `ret` would not return to main.
 */
#include <stdio.h>

#if defined(__x86_64__) && !defined(_WIN32)

typedef struct {
    void *prev;
    long rip, rbp;
} buf_t;
static buf_t *top;

__attribute__((used)) unsigned int push_tail(buf_t *b) {
    b->prev = top;
    top = b;
    return 0;
}

__attribute__((naked)) unsigned int push(buf_t *b) {
    __asm volatile(
        "movq (%rsp), %rax\n"
        "movq %rax, 8(%rdi)\n"
        "movq %rbp, 16(%rdi)\n"
        "jmp push_tail\n");
}

static __attribute__((naked)) int seven(void) {
    __asm volatile("movl $7, %eax\nret\n");
}

__attribute__((naked)) int add_two_args(int a, int b) {
    __asm volatile("leal (%rdi,%rsi), %eax\nret\n");
}

int main(void) {
    buf_t b = {0, 0, 0};
    unsigned r = push(&b);
    if (r != 0) {
        printf("FAIL: push returned %u\n", r);
        return 1;
    }
    if (top != &b || b.prev != NULL) {
        printf("FAIL: push_tail did not link the buffer (top=%p)\n", (void *)top);
        return 2;
    }
    if (b.rip == 0) {
        printf("FAIL: return address not saved\n");
        return 3;
    }
    if (seven() != 7) {
        printf("FAIL: seven() = %d\n", seven());
        return 4;
    }
    if (add_two_args(40, 2) != 42) {
        printf("FAIL: add_two_args = %d\n", add_two_args(40, 2));
        return 5;
    }
    printf("OK\n");
    return 0;
}

#else
int main(void) {
    printf("OK\n");
    return 0;
}
#endif
