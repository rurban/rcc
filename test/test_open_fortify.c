// Regression: open() with O_CREAT under _FORTIFY_SOURCE routed to __open_2
// because __builtin_va_arg_pack_len() returned 0 (see test_va_arg_pack_len.c),
// dropping the mode arg. glibc then aborted: "invalid open call: O_CREAT ...
// without mode". Must create + write + read back cleanly.
#define _FORTIFY_SOURCE 2
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

int main(void) {
    // no glibc-style fortify wrappers on mingw
#ifndef _WIN32
    const char *path = "/tmp/rcc_open_fortify_test.txt";
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    assert(fd >= 0);
    assert(write(fd, "hi\n", 3) == 3);
    assert(close(fd) == 0);
    char buf[8] = {0};
    fd = open(path, O_RDONLY);
    assert(fd >= 0);
    assert(read(fd, buf, sizeof buf) == 3);
    close(fd);
    assert(strcmp(buf, "hi\n") == 0);
    unlink(path);
#endif
    return 0;
}
