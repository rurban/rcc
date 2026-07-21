#ifdef __aarch64__
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

static void on_alarm(int sig) {
    (void)sig;
    write(2, "FAIL: timed out (infinite loop)\n", 33);
    _exit(1);
}

int main(void) {
    int x = 42, y = 0;
    __asm__("str %w1, [%0]" : : "r"(&y), "r"(x));
    if (y != 42) return 1;

    /* GAS numeric local label ("1b") as an actual branch target, not just
     * a data reference — the ARM64 mirror of the x86 "1b"/"1f" test in
     * test_x86_asm.c. emit_arm64_branch() looks up the raw operand text
     * ("1b") against defined label names directly, but define_label()
     * only ever records the bare digits ("1") — the b/f direction suffix
     * is never part of the recorded name. Unlike encode_x86() (fixed by
     * commit 81fed1ae), the ARM64 branch path never strips it, so the
     * lookup always misses and falls through to a forward fixup for a
     * name ("1b") no definition will ever produce: the branch's
     * placeholder displacement is left unpatched, which decodes as a
     * branch to itself — an infinite loop instead of a decrement retry
     * loop. Guarded with alarm() so a regression fails loudly instead of
     * hanging the test run. */
    signal(SIGALRM, on_alarm);
    alarm(5);
    {
        int loop_n = 5;
        __asm__ volatile (
            "1:\n\t"
            "ldr w9, [%0]\n\t"
            "sub w9, w9, #1\n\t"
            "str w9, [%0]\n\t"
            "cbnz w9, 1b\n\t"
            :
            : "r"(&loop_n)
        );
        if (loop_n != 0) return 2;
    }
    alarm(0);

    printf("OK arm64 asm\n");
    return 0;
}
#else
int main(void) {
    printf("OK skipped arm64\n");
    return 0;
}
#endif
