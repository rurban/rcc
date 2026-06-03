// SPDX-License-Identifier: LGPL-2.1-or-later
// Runtime support for Darwin (macOS): provides glibc extensions missing on macOS.

#include <stdlib.h>

/* on_exit - register cleanup function with exit code and argument.
   glibc extension, not available in Darwin libc.
   
   Uses __cxa_atexit.  Limitation: exit code is always 0 because macOS
   does not allow intercepting exit() from user code (libSystem re-exports
   prevent dyld interposing).  This is sufficient for the TCC test suite
   which primarily checks LIFO execution order and process exit code. */

static struct { void (*fn)(int, void *); void *arg; } on_exit_ent[32];
static int on_exit_n;

extern int __cxa_atexit(void (*func)(void *), void *arg, void *dso);

static void on_exit_thunk(void *p) {
    struct { void (*fn)(int, void *); void *arg; } *e = p;
    e->fn(0, e->arg);
}

int on_exit(void (*fn)(int, void *), void *arg) {
    if (on_exit_n >= 32) return -1;
    on_exit_ent[on_exit_n].fn = fn;
    on_exit_ent[on_exit_n].arg = arg;
    __cxa_atexit(on_exit_thunk, &on_exit_ent[on_exit_n], NULL);
    on_exit_n++;
    return 0;
}
