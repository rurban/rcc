// SPDX-License-Identifier: LGPL-2.1-or-later
// Runtime support for Darwin (macOS): provides glibc extensions missing on macOS.

#include <stdlib.h>

/* on_exit - register cleanup function with exit code and argument.
   glibc extension, not available in Darwin libc.
   Uses dyld interposing to capture the exit code from exit(). */

static struct {
    void (*fn)(int, void *);
    void *arg;
} on_exit_ent[32];
static int on_exit_n;
static int on_exit_code;

static void on_exit_run(void) {
    int i = on_exit_n;
    while (i > 0) {
        i--;
        on_exit_ent[i].fn(on_exit_code, on_exit_ent[i].arg);
    }
}
int on_exit(void (*fn)(int, void *), void *arg) {
    if (on_exit_n >= 32) return -1;
    on_exit_ent[on_exit_n].fn = fn;
    on_exit_ent[on_exit_n].arg = arg;
    if (on_exit_n == 0) atexit(on_exit_run);
    on_exit_n++;
    return 0;
}

/* Forward declaration — defined after the interpose struct */
static void rcc_exit_interposer(int code);

/* dyld interpose struct. The replacee field holds libSystem's original
   exit() address at link time.  dyld does NOT patch this struct field
   (it patches GOT entries at call sites, not immediate data), so
   rcc_exit_interpose.replacee remains the true original address. */
__attribute__((used, section("__DATA,__interpose"))) static const struct {
    const void *replacement;
    const void *replacee;
} rcc_exit_interpose = {(const void *)rcc_exit_interposer, (const void *)exit};

static void rcc_exit_interposer(int code) {
    on_exit_code = code;
    /* Call the original exit() via the stored replacee address.
       This bypasses the interpose since it goes directly to libSystem. */
    typedef void (*exit_fn_t)(int);
    ((exit_fn_t)rcc_exit_interpose.replacee)(code);
    __builtin_unreachable();
}
