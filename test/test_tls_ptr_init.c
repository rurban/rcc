/* A __thread/_Thread_local pointer variable statically initialized to the
 * address of an ordinary (non-TLS) global -- `static _Thread_local int *p
 * = &marker;` -- got a NULL/garbage pointer in every thread's TLS block
 * (main thread included) instead of the real address of `marker`.
 * (The reverse -- a TLS variable initialized to point at *another* TLS
 * object -- is not valid C in the first place: real GCC rejects it with
 * "initializer element is not constant", since each thread's copy would
 * need a different, runtime-computed value.)
 *
 * Root cause: src/elf_write.c built ELF relocation sections for every
 * data-bearing built-in section (.rela.text, .rela.data, .rela.rodata,
 * .rela.init_array, .rela.fini_array, ...) except .tdata. codegen.c's
 * global-initializer emission already correctly appended the pointer's
 * relocation to obj->data_tls_relocs (obj->data_tls_reloc_count > 0), but
 * elf_write() never wrote a .rela.tdata section header or its reloc
 * entries at all, so the linker had nothing to patch: the pointer's
 * initial 8 bytes in .tdata's copy stayed all-zero for every thread.
 *
 * A non-TLS global doing the exact same `&other_global` initializer
 * worked fine (routed through .rela.data), which is why this went
 * unnoticed until a real TLS-pointer initializer was tried.
 *
 * ELF-specific (the fix lives in elf_write.c's .rela.tdata handling).
 * Darwin's TLS ABI is a completely different, separate mechanism (TLV
 * descriptors in a __thread_vars Mach-O section, driven by
 * macho_write.c, untouched by this fix) and isn't one of this repo's 3
 * tracked targets (native Linux, mingw, arm64 -- see AGENTS.md), so this
 * test is skipped there rather than asserting Mach-O TLV behavior this
 * session didn't touch.
 */
#ifndef __APPLE__
#include <pthread.h>

static int marker = 42;
static _Thread_local int *tls_ptr = &marker;

static void *thread_fn(void *arg) {
    (void)arg;
    if (tls_ptr != &marker) return (void *)1;
    if (*tls_ptr != 42) return (void *)2;
    return NULL;
}

int main(void) {
    if (tls_ptr != &marker) return 1;
    if (*tls_ptr != 42) return 2;

    pthread_t t;
    if (pthread_create(&t, NULL, thread_fn, NULL) != 0) return 3;
    void *rc = NULL;
    if (pthread_join(t, &rc) != 0) return 4;
    if (rc != NULL) return 5;

    return 0;
}
#else
int main(void) {
    return 0;
}
#endif
