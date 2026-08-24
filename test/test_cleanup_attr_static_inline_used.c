/* Regression test for a whole-program DCE bug (opt.c's
 * eliminate_unused_static_inline): __attribute__((cleanup(fn))) on a
 * local variable was never recognized as a reference to `fn`, so a
 * `static inline` cleanup helper that nothing else in the TU calls by
 * name got omitted from the object file -- even though codegen.c's
 * emit_cleanup_var() emits a direct call to it at every scope-exit
 * path (return/break/goto/fall-through, one call site per exit, never
 * outlined into a shared subroutine, so there is no ND_FUNCALL node
 * anywhere in the AST body referencing it at all -- the DCE pass's
 * BFS only ever walked ND_FUNCALL/ND_LVAR-address nodes and the
 * separate `defer` statement list, missing this third reference kind
 * entirely).
 *
 * Found via bubblewrap's utils.h/bind-mount.c: cleanup_freep(),
 * cleanup_fdp(), cleanup_mount_tabp() are all plain `static inline`
 * wrappers around free()/close(), referenced only through
 * `#define cleanup_free __attribute__((cleanup(cleanup_freep)))`-
 * style macros -- "undefined reference to cleanup_freep" at link
 * time, though gcc links (and runs) the same source cleanly.
 */
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

static inline void
cleanup_freep(void *p)
{
    void **pp = (void **)p;
    if (*pp)
        free(*pp);
}
#define cleanup_free __attribute__((cleanup(cleanup_freep)))

static inline void
cleanup_fdp(int *fdp)
{
    int fd = *fdp;
    if (fd >= 0)
        close(fd);
}
#define cleanup_fd __attribute__((cleanup(cleanup_fdp)))

static int early_return_path(int take_early) {
    cleanup_free char *buf = malloc(8);
    buf[0] = 'z';
    if (take_early)
        return 1; /* cleanup must still fire on this exit path */
    return 0;
}

int main(void) {
    cleanup_free char *a = malloc(16);
    a[0] = 'a';

    int fd = -1;
    { cleanup_fd int f = fd; (void)f; }

    assert(early_return_path(1) == 1);
    assert(early_return_path(0) == 0);

    return 0;
}
