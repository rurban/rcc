/* glibc's <bits/unistd-decl.h> (pulled in transitively under
 * _FORTIFY_SOURCE=2 with -O1+) declares e.g.
 * `extern ssize_t __read_chk(int, void*, size_t, size_t);` (4 params).
 * rcc predefines `__read_chk`/`__pread_chk`/`__readlink_chk`/
 * `__readlinkat_chk`/`__getcwd_chk`/etc. (bare and __builtin_-prefixed,
 * plus their _warn siblings) so real call sites from glibc's
 * __glibc_fortify expansion resolve to the plain, unchecked function.
 *
 * Regression: those were plain object-like macros
 * (`#define __read_chk read`), which ALSO fire on the bare declaration
 * name in unistd-decl.h itself, textually producing
 * `extern ssize_t read(int, void*, size_t, size_t);` -- 4 params,
 * conflicting with <unistd.h>'s real 3-param `read` prototype, once
 * rcc started diagnosing incompatible redeclarations. Converting these
 * to function-like macros that drop the trailing bufsize/buflen
 * parameter fixes both: real calls still forward correctly, and the
 * declaration's parameter list shrinks in step, keeping arity in sync.
 *
 * Must compile and link cleanly with the whole unistd.h/fcntl.h
 * fortify surface pulled in, and behave like the unchecked functions
 * at runtime. */
#define _FORTIFY_SOURCE 2
#define _XOPEN_SOURCE 700
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

int main(void) {
#ifndef _WIN32
    const char *path = "/tmp/rcc_fortify_chk_arity_test.txt";
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    assert(fd >= 0);
    assert(write(fd, "hi\n", 3) == 3);
    assert(close(fd) == 0);

    char buf[8] = {0};
    fd = open(path, O_RDONLY);
    assert(fd >= 0);
    ssize_t n = read(fd, buf, sizeof(buf));
    assert(n == 3);
    assert(memcmp(buf, "hi\n", 3) == 0);
    assert(close(fd) == 0);

    fd = open(path, O_RDONLY);
    assert(fd >= 0);
    ssize_t pn = pread(fd, buf, sizeof(buf), 0);
    assert(pn == 3);
    assert(close(fd) == 0);
#endif
    return 0;
}
