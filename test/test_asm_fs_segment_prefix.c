/* GAS AT&T segment-override-prefixed memory operands ("%fs:0", "%fs:(%rax)",
 * "%gs:...", ...) in inline asm. rcc's built-in assembler only supported
 * the prefix as a separate mnemonic word ("fs movq ..."); the glued
 * operand spelling "%fs:..." -- which is what glibc/mimalloc actually
 * emit -- started with '%' so the operand classifier (X86_ISREG, a plain
 * "starts with %" test) treated it as a REGISTER. parse_x86_reg64() then
 * silently fell back to X86_NOREG == -1, which aliases physical register
 * 7/RDI once masked into a ModRM field, so `movq %fs:0, %rax` came out
 * as `mov %rdi, %rax` -- a garbage value. Real-world impact: mimalloc's
 * _mi_prim_thread_id() read %fs:0 this way and returned 0/garbage, so
 * every segment looked "abandoned", free spans were never queued, and
 * the first allocation recursed infinitely allocating fresh segments.
 *
 * Fixed by classifying %es/%cs/%ss/%ds/%fs/%gs-prefixed operands as
 * memory (x86_op_is_seg, asm.c), stripping the prefix in parse_x86_mem()
 * into a new X86Mem.seg field, and emitting the prefix byte (0x64/0x65/
 * ...) as the very first instruction byte in encode_x86() before any
 * REX/opcode (a segment prefix must precede everything else).
 *
 * The %fs:0 == pthread_self() equivalence is glibc x86-64 specific
 * (TLS TCB self-pointer): on Darwin x86-64 the TCB lives in %gs, on
 * Windows %fs:0 is the TEB self-pointer (unrelated to winpthreads'
 * pthread_t), and on AArch64 there is no %fs at all (rcc's ARM64
 * inline-asm path silently ignores the x86 template, leaving the
 * operand uninitialized). Skip everywhere else.
 */
#if defined(__linux__) && defined(__x86_64__)
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

static inline void *read_fs_slot(size_t slot) {
    void *res;
    const size_t ofs = slot * sizeof(void *);
    __asm__("movq %%fs:%1, %0" : "=r"(res) : "m"(*((void **)ofs)));
    return res;
}

/* Register-indirect form: %fs:(%reg), the other common spelling. */
static inline void *read_fs_indirect(void) {
    void *res;
    __asm__("movq %%fs:(%%rax), %0" : "=r"(res) : "a"(0));
    return res;
}

static void *worker_main(void *arg) {
    return read_fs_slot((size_t)arg);
}

int main(void) {
    /* %fs:0 on glibc x86-64 is the thread pointer (TCB self-pointer),
     * which pthread_self() must equal. */
    void *slot0 = read_fs_slot(0);
    void *ind = read_fs_indirect();
    pthread_t self = pthread_self();

    if (slot0 == NULL || slot0 != (void *)(uintptr_t)self) {
        printf("FAIL: fs:0=%p pthread_self=%p\n", slot0, (void *)(uintptr_t)self);
        return 1;
    }
    if (ind == NULL || ind != (void *)(uintptr_t)self) {
        printf("FAIL: fs:(%%rax)=%p pthread_self=%p\n", ind, (void *)(uintptr_t)self);
        return 2;
    }

    /* A worker thread's %fs:0 must differ from the main thread's. */
    pthread_t t;
    void *worker_val = NULL;
    if (pthread_create(&t, NULL, worker_main, (void *)0) != 0)
        return 3;
    pthread_join(t, &worker_val);
    if (worker_val == NULL || worker_val == (void *)(uintptr_t)self) {
        printf("FAIL: worker fs:0=%p == main %p\n", worker_val, (void *)(uintptr_t)self);
        return 4;
    }

    return 0;
}
#else
int main(void) {
    return 0;
}
#endif
