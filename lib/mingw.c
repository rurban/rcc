#include <stdlib.h>

/* on_exit - register cleanup function with exit code and argument.
   glibc extension, not available in mingw CRT.
   Registered via atexit (exit code not propagated to callback). */
static struct {
    void (*fn)(int, void *);
    void *arg;
} on_exit_ent[32];
static int on_exit_n;
static void on_exit_run(void) {
    int i = on_exit_n;
    while (i > 0) {
        i--;
        on_exit_ent[i].fn(0, on_exit_ent[i].arg);
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
