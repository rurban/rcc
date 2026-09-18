/* Regression tests for two soundness bugs in -O1 block-local value
 * propagation (opt.c's local_opt_stmt_list/local_opt_eligible, added by
 * the "propagate local values" pass):
 *
 * 1. Loop-condition substitution: the pass replaced variables in an
 *    ND_FOR/ND_DO condition with their last known straight-line value,
 *    but a loop condition is RE-EVALUATED after the body/inc clause
 *    mutates the variable -- "while (i < 10)" with i==0 became
 *    "while (0 < 10)", an infinite loop. TinyCC 08_while, 09_do_while,
 *    30_hanoi, 87_dead_code and 89_nocode_wanted all hung or exited
 *    non-zero because of it. IF/SWITCH conditions are evaluated exactly
 *    once, so substitution there remains sound.
 *
 * 2. Cleanup/defer variables: local_opt_eligible() didn't exclude
 *    __attribute__((cleanup)))/C23-defer variables, whose address is
 *    passed to arbitrary code at scope exit -- an invisible read/write
 *    no AST walk (not even var->addr_taken) sees. The pass dropped such
 *    a variable's store as dead once its only AST read was propagated
 *    away, so the cleanup function printed a stale stack slot (tinycc
 *    101_cleanup's test_cleanup1 printed -1 instead of 42).
 *
 * The loop cases are compiled and run as subprocesses under an
 * alarm(10)+kill-child guard: if the bug ever returns, the test fails
 * fast instead of hanging the whole suite (and no orphaned spinning
 * binary is left behind).
 */
#if !defined(_WIN32)
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include "test_common.h"

static pid_t opt_fix_child;

static void opt_fix_on_alarm(int sig) {
    (void)sig;
    if (opt_fix_child > 0)
        kill(opt_fix_child, SIGKILL);
    _exit(124);
}

/* Compile `src` with rcc -O1, run it, check the exit status is want_rc. */
static int compile_run_check(const char *rcc, const char *td, int pid,
                             const char *tag, const char *src, int want_rc) {
    char srcf[128], exe[128], cmd[512];
    snprintf(srcf, sizeof(srcf), "%s/test_lofix_%s_%d.c", td, tag, pid);
    snprintf(exe, sizeof(exe), "%s/test_lofix_%s_%d", td, tag, pid);
    FILE *f = fopen(srcf, "w");
    if (!f) { printf("FAIL: [%s] cannot write %s\n", tag, srcf); return 0; }
    fputs(src, f);
    fclose(f);

    snprintf(cmd, sizeof(cmd), "%s -O1 -o %s %s " NULL_REDIRECT, rcc, exe, srcf);
    int rc = system(cmd);
    remove(srcf);
    if (rc != 0) {
        printf("FAIL: [%s] compile failed (rc=%d)\n", tag, rc);
        remove(exe);
        return 0;
    }

    opt_fix_child = fork();
    if (opt_fix_child == 0) {
        execl(exe, exe, (char *)NULL);
        _exit(127);
    }
    if (opt_fix_child < 0) {
        printf("FAIL: [%s] fork failed\n", tag);
        remove(exe);
        return 0;
    }
    alarm(10);
    int st = 0;
    waitpid(opt_fix_child, &st, 0);
    alarm(0);
    opt_fix_child = 0;
    remove(exe);
    if (!WIFEXITED(st) || WEXITSTATUS(st) != want_rc) {
        printf("FAIL: [%s] expected exit %d, got status 0x%x\n", tag, want_rc, st);
        return 0;
    }
    return 1;
}

int main(void) {
    const char *rcc = find_rcc();
    const char *td = get_tmpdir();
    int pid = (int)getpid();
    int ok = 1;

    signal(SIGALRM, opt_fix_on_alarm);

    /* Bug 1: condition must keep reading the live variable, not the
     * pre-loop constant, across body/inc mutations. */
    ok &= compile_run_check(rcc, td, pid, "while_mut",
        "int main(void){int i=0;while(i<10)i=i+1;return i;}\n", 10);
    ok &= compile_run_check(rcc, td, pid, "do_while_mut",
        "int main(void){int i=0;do{i=i+1;}while(i<10);return i;}\n", 10);
    ok &= compile_run_check(rcc, td, pid, "for_inc_mut",
        "int main(void){int i=0;for(;i<10;i++);return i;}\n", 10);
    ok &= compile_run_check(rcc, td, pid, "for_sum",
        "int main(void){int s=0;for(int i=0;i<10;i++)s+=i;return s;}\n", 45);

    /* Bug 2: the cleanup attribute passes &n to cl() at scope exit;
     * dropping the n=42 store leaves cl() reading a stale slot. The
     * volatile -1 pad warms the stack region f's frame reuses so a
     * regression reads -1 deterministically instead of leftover 42. */
    ok &= compile_run_check(rcc, td, pid, "cleanup_store",
        "static int seen;"
        "static void cl(int *p){seen=*p;*p=-1;}"
        "static int f(void){int __attribute__((cleanup(cl))) n=42;return n;}"
        "int main(void){volatile int pad[4]={-1,-1,-1,-1};(void)pad;"
        "f();return seen==42?0:1;}\n", 0);

    if (!ok) return 1;
    printf("OK -O1 local values: loop conditions stay live across body/inc "
           "mutations; cleanup/defer variables keep their stores\n");
    return 0;
}
#else
int main(void) {
    return 0;
}
#endif
