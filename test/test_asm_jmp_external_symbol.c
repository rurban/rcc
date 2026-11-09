/* A jmp/jcc in inline asm to a symbol that is not a C label of the function
 * (here: another function) was assembled with a zero displacement and NO
 * relocation, so it silently jumped to the next instruction. `call foo` in the
 * same position always worked. micropython's nlr_push is `jmp nlr_push_tail`.
 *
 * The child jumps from inline asm into exit_with_42(), which never returns;
 * the parent checks the exit status. With the old behaviour the asm fell
 * through, main returned 0 and the status was wrong.
 */
#include <stdio.h>

#if defined(__x86_64__) && defined(__linux__)
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

__attribute__((used, noreturn)) void exit_with_42(void) {
    _exit(42);
}

__attribute__((used, noreturn)) void exit_with_43(void) {
    _exit(43);
}

static int status_of_child(int use_jcc) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        if (use_jcc) {
            __asm__ volatile("xorl %%eax, %%eax\n\t"
                             "testl %%eax, %%eax\n\t"
                             "je exit_with_43\n\t" ::: "eax", "cc");
        } else {
            __asm__ volatile("jmp exit_with_42");
        }
        _exit(1); /* reached only if the jump went nowhere */
    }
    int st = 0;
    if (waitpid(pid, &st, 0) < 0 || !WIFEXITED(st)) return -1;
    return WEXITSTATUS(st);
}

int main(void) {
    int a = status_of_child(0);
    if (a != 42) {
        printf("FAIL: jmp to external function: child status %d, want 42\n", a);
        return 1;
    }
    int b = status_of_child(1);
    if (b != 43) {
        printf("FAIL: je to external function: child status %d, want 43\n", b);
        return 2;
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
