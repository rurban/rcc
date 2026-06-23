/* SPDX-License-Identifier: LGPL-2.1-or-later
 * Unified test runner for rcc.
 *
 * Replaces: run_tcc_suite.sh, test/torture/run.sh,
 *           test/compliance/run.sh, run-c-testsuite.sh
 *
 * Usage: ./run_tests [rcc-binary] [options] [test-name]
 * For cross platforms, run the whole runner under the cross environment.
 *        wine ./run_tests.exe rcc.exe --tcc --unit-tests --compliance --ctest
 *        qemu-aarch64 -L /usr/aarch64-linux-gnu/sys-root ./run_tests ./rcc-arm64 --tcc --unit-tests --compliance --ctest
 *
 * Options (default: --tcc --unit-tests --compliance --ctest):
 *   --tcc         TCC compatibility tests (tinycc/tests/tests2/)
 *   --unit-tests  Our own Unit tests (test/test_*.c)
 *   --compliance  NCC compliance tests (gcc vs rcc output comparison)
 *   --ctest       C-testsuite (native C runner)
 *   --torture     GCC torture tests (test/torture/)
 *   --all         All test suites
 *   -v, --verbose Show compile/run command lines and output for each test
 *   --summary     Torture summary-only mode
 *   --no-color    Disable ANSI color output
 *   --parallel    Run tests in parallel (auto-detect worker count)
 *   --jobs N      Run tests with N worker threads (--jobs 1 = sequential)
 *
 */

#define _GNU_SOURCE
#ifdef _WIN32
#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#ifdef _WIN32
#include <windows.h>
/* MAX_PATH is only 260 here, so gcc warns that concatenating two
 * path-sized fragments into a PATH_MAX buffer could overflow; in
 * practice all paths used by this test runner are short. */
#pragma GCC diagnostic ignored "-Wformat-truncation"
#else
#include <sys/wait.h>
#include <sys/utsname.h>
#include <dlfcn.h>
#endif

#ifndef _WIN32
#include <pthread.h>
#endif

static void *xrealloc(void *p, size_t sz) {
    void *tmp = realloc(p, sz);
    if (!tmp) {
        perror("realloc");
        exit(1);
    }
    return tmp;
}

#ifdef _WIN32
/* sys/utsname.h replacement */
struct utsname {
    char sysname[32];
    char machine[32];
};
static int uname(struct utsname *u) {
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);
    strcpy(u->sysname, "Windows");
    strcpy(u->machine, si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64 ? "arm64" : "x86_64");
    return 0;
}
#define realpath(path, resolved) _fullpath((resolved), (path), PATH_MAX)

#ifndef NAME_MAX
#define NAME_MAX FILENAME_MAX
#endif

static char *strndup(const char *s, size_t n) {
    size_t len = strnlen(s, n);
    char *p = malloc(len + 1);
    if (!p) return NULL;
    memcpy(p, s, len);
    p[len] = '\0';
    return p;
}


static int setenv(const char *name, const char *value, int overwrite) {
    if (!overwrite && getenv(name)) return 0;
    return _putenv_s(name, value);
}

/* dirent.h on mingw lacks scandir()/alphasort()/versionsort() */
static int alphasort(const struct dirent **a, const struct dirent **b) {
    return strcmp((*a)->d_name, (*b)->d_name);
}

static int scandir(const char *dir, struct dirent ***namelist,
                   int (*filter)(const struct dirent *),
                   int (*cmp)(const struct dirent **, const struct dirent **)) {
    DIR *d = opendir(dir);
    if (!d) return -1;
    struct dirent **list = NULL;
    int n = 0, cap = 0;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (filter && !filter(e)) continue;
        if (n == cap) {
            cap = cap ? cap * 2 : 16;
            list = xrealloc(list, (size_t)cap * sizeof(*list));
        }
        struct dirent *copy = malloc(sizeof(*copy));
        memcpy(copy, e, sizeof(*copy));
        list[n++] = copy;
    }
    closedir(d);
    if (cmp)
        qsort(list, (size_t)n, sizeof(*list),
              (int (*)(const void *, const void *))cmp);
    *namelist = list;
    return n;
}
#endif

/* Return a temporary directory that exists.  On Windows the system
 * temp path is used (C:\Users\...\AppData\Local\Temp); /tmp would
 * only exist under MSYS2. */
static const char *get_tmpdir(void) {
    static char buf[PATH_MAX];
    if (buf[0]) return buf;
#ifdef _WIN32
    if (!GetTempPathA(sizeof(buf), buf)) {
        /* fallback: CreateDirectory + C:\tmp */
        strcpy(buf, "C:\\tmp");
    }
    /* strip trailing backslash */
    size_t len = strlen(buf);
    while (len && buf[len - 1] == '\\') buf[--len] = '\0';
#else
    strcpy(buf, "/tmp");
#endif
    return buf;
}

/* versionsort() is a glibc/dirent.h extension; mingw and BSD-derived
 * libcs (macOS) don't provide it. */
#if defined(_WIN32) || !defined(__GLIBC__)
/* natural-order compare: like GNU strverscmp(), splits runs of digits
 * and compares them numerically so "f9" sorts before "f10" */
static int versionsort(const struct dirent **a, const struct dirent **b) {
    const char *pa = (*a)->d_name, *pb = (*b)->d_name;
    while (*pa && *pb) {
        if (isdigit((unsigned char)*pa) && isdigit((unsigned char)*pb)) {
            const char *sa = pa, *sb = pb;
            while (isdigit((unsigned char)*pa)) pa++;
            while (isdigit((unsigned char)*pb)) pb++;
            size_t la = (size_t)(pa - sa), lb = (size_t)(pb - sb);
            if (la != lb) return la < lb ? -1 : 1;
            int c = strncmp(sa, sb, la);
            if (c) return c;
        } else {
            if (*pa != *pb) return (unsigned char)*pa - (unsigned char)*pb;
            pa++;
            pb++;
        }
    }
    return (unsigned char)*pa - (unsigned char)*pb;
}
#endif

/* ARM64_NATIVE: this binary itself runs natively on aarch64 (an
 * aarch64-linux cross build executed under qemu-aarch64/native arm64,
 * or a native arm64-darwin/arm64-elf build). On such hosts rcc-arm64
 * and the binaries it produces execute directly, without a runner. */
#if defined(__aarch64__) && !defined(ARM64_NATIVE)
#define ARM64_NATIVE 1
#endif

/* ── unified result output ───────────────────────────────────────── */

/* set via --no-color (also forced for the mingw cross/wine target,
 * whose console mangles ANSI SGI sequences) */
static bool g_no_color = false;

#define COL_GREEN  (g_no_color ? "" : "\033[0;32m")
#define COL_RED    (g_no_color ? "" : "\033[0;31m")
#define COL_YELLOW (g_no_color ? "" : "\033[0;33m")
#define COL_CYAN   (g_no_color ? "" : "\033[0;36m")
#define COL_RESET  (g_no_color ? "" : "\033[0m")

static FILE *g_log_fp = NULL;
static char g_log_final_path[PATH_MAX]; /* final destination after atomic rename */
static char g_log_tmp_path[PATH_MAX]; /* tmp path currently being written      */

static void print_result(const char *name, const char *col, const char *status) {
    if (col && *col)
        printf("  %-35s %s%s%s\n", name, col, status, COL_RESET);
    else
        printf("  %-35s %s\n", name, status);
    if (g_log_fp)
        fprintf(g_log_fp, "  %-35s %s\n", name, status);
}

static void logprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static void logprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    if (g_log_fp) {
        va_start(ap, fmt);
        vfprintf(g_log_fp, fmt, ap);
        va_end(ap);
    }
}

static int move_file_atomic(const char *src, const char *dst);

/* Open log to a .tmp file; record final path for close_log(). */
static void open_log(const char *final_path) {
    snprintf(g_log_final_path, sizeof(g_log_final_path), "%s", final_path);
    snprintf(g_log_tmp_path, sizeof(g_log_tmp_path), "%s.tmp", final_path);
    g_log_fp = fopen(g_log_tmp_path, "wb");
}

/* Close log and atomically rename tmp→final. */
static void close_log(void) {
    if (!g_log_fp) return;
    fclose(g_log_fp);
    g_log_fp = NULL;
    if (g_log_tmp_path[0])
        move_file_atomic(g_log_tmp_path, g_log_final_path);
    g_log_tmp_path[0] = g_log_final_path[0] = '\0';
}

/* Write a .summary atomically via a .tmp sidestep. */
static void write_summary(const char *path, const char *content) {
    char tmp[PATH_MAX + 4];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    FILE *f = fopen(tmp, "wb");
    if (!f) return;
    fputs(content, f);
    fclose(f);
    move_file_atomic(tmp, path);
}

/* ── process execution ───────────────────────────────────────────── */

static char *strappend(char *buf, size_t *len, size_t *cap, const char *fmt, ...)
    __attribute__((format(printf, 4, 5)));

typedef struct {
    char *out;
    size_t out_len;
    int exit_code;
    bool timed_out, spawn_failed;
} ProcResult;

#ifndef _WIN32
#include <spawn.h>
extern char **environ;

static ProcResult proc_run_once(char *const argv[], int timeout_sec, int capture, const char *cwd) {
    ProcResult r = {0};
    r.exit_code = -1;

    int out_pipe[2], err_pipe[2];
    if (pipe(out_pipe) < 0) {
        r.spawn_failed = true;
        return r;
    }
    if (capture != 0 && pipe(err_pipe) < 0) {
        close(out_pipe[0]);
        close(out_pipe[1]);
        r.spawn_failed = true;
        return r;
    }

    // Build file actions for posix_spawn: redirect stdout/stderr to pipes
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    if (cwd) posix_spawn_file_actions_addchdir_np(&actions, cwd);
    if (capture == 1) {
        // capture stderr only, stdout -> /dev/null
        posix_spawn_file_actions_adddup2(&actions, err_pipe[1], STDERR_FILENO);
        posix_spawn_file_actions_addclose(&actions, err_pipe[0]);
        posix_spawn_file_actions_addclose(&actions, out_pipe[0]);
        posix_spawn_file_actions_addclose(&actions, out_pipe[1]);
    } else if (capture == 2) {
        // capture stdout only, stderr -> /dev/null
        posix_spawn_file_actions_adddup2(&actions, out_pipe[1], STDOUT_FILENO);
        posix_spawn_file_actions_addclose(&actions, out_pipe[0]);
        posix_spawn_file_actions_addclose(&actions, err_pipe[0]);
        posix_spawn_file_actions_addclose(&actions, err_pipe[1]);
    } else {
        // capture both: dup write end to both stdout and stderr
        posix_spawn_file_actions_adddup2(&actions, out_pipe[1], STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&actions, out_pipe[1], STDERR_FILENO);
        posix_spawn_file_actions_addclose(&actions, out_pipe[0]);
    }

    pid_t pid;
    int spawn_ret = posix_spawnp(&pid, argv[0], &actions, NULL, argv, environ);
    posix_spawn_file_actions_destroy(&actions);

    // Close write ends (child has its own copies via dup2)
    close(out_pipe[1]);
    if (capture != 0) close(err_pipe[1]);

    if (spawn_ret != 0) {
        close(out_pipe[0]);
        if (capture != 0) close(err_pipe[0]);
        r.spawn_failed = true;
        return r;
    }

    int read_fd = capture == 1 ? err_pipe[0] : out_pipe[0];

    // Use select() with timeout — thread-safe, no process-global alarm
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(read_fd, &fds);
    struct timeval tv = {.tv_sec = timeout_sec, .tv_usec = 0};
    int sel_ret = select(read_fd + 1, &fds, NULL, NULL, &tv);

    size_t cap = 8192;
    r.out = malloc(cap);
    r.out_len = 0;
    char buf[8192];
    ssize_t n;
    if (sel_ret > 0) {
        while ((n = read(read_fd, buf, sizeof(buf))) > 0) {
            if (r.out_len + (size_t)n + 1 > cap) {
                cap = r.out_len + (size_t)n + 8192;
                r.out = xrealloc(r.out, cap);
            }
            memcpy(r.out + r.out_len, buf, (size_t)n);
            r.out_len += (size_t)n;
        }
    } else if (sel_ret == 0) {
        // Timeout: kill the child
        kill(pid, SIGKILL);
        r.timed_out = true;
        // Drain any remaining output (non-blocking after kill)
        while ((n = read(read_fd, buf, sizeof(buf))) > 0) {
            if (r.out_len + (size_t)n + 1 > cap) {
                cap = r.out_len + (size_t)n + 8192;
                r.out = xrealloc(r.out, cap);
            }
            memcpy(r.out + r.out_len, buf, (size_t)n);
            r.out_len += (size_t)n;
        }
    }
    close(read_fd);
    if (capture == 1)
        close(out_pipe[0]);
    else if (capture == 2)
        close(err_pipe[0]);

    int status;
    pid_t w;
    int wait_attempts = 0;
    do {
        w = waitpid(pid, &status, WNOHANG);
        if (w != pid) {
            usleep(100000); // 100ms
            wait_attempts++;
        }
    } while (w == 0 && wait_attempts < 10);
    if (w != pid && wait_attempts >= 10)
        r.timed_out = true; /* gave up waiting */

    if (w == pid) {
        if (WIFEXITED(status)) r.exit_code = WEXITSTATUS(status);
        else if (WIFSIGNALED(status))
            r.exit_code = 128 + WTERMSIG(status);
    }

    if (!r.out) r.out = strdup("");
    else {
        r.out = xrealloc(r.out, r.out_len + 1);
        r.out[r.out_len] = '\0';
    }
    return r;
}

static void proc_free(ProcResult *r) {
    free(r->out);
}
#else
/* Build a Windows command line from argv, quoting each argument per the
 * MSVCRT/CommandLineToArgvW backslash-escaping rules. */
static char *build_cmdline(char *const argv[]) {
    char *buf = NULL;
    size_t len = 0, cap = 0;
    for (int i = 0; argv[i]; i++) {
        const char *a = argv[i];
        if (i) buf = strappend(buf, &len, &cap, " ");
        bool needs_quotes = a[0] == '\0' || strpbrk(a, " \t\"") != NULL;
        if (!needs_quotes) {
            buf = strappend(buf, &len, &cap, "%s", a);
            continue;
        }
        buf = strappend(buf, &len, &cap, "\"");
        size_t nbs = 0;
        for (const char *p = a;; p++) {
            if (*p == '\\') {
                nbs++;
                continue;
            }
            if (*p == '"') {
                for (size_t k = 0; k < nbs * 2 + 1; k++) buf = strappend(buf, &len, &cap, "\\");
                buf = strappend(buf, &len, &cap, "\"");
            } else if (*p == '\0') {
                for (size_t k = 0; k < nbs * 2; k++) buf = strappend(buf, &len, &cap, "\\");
                break;
            } else {
                for (size_t k = 0; k < nbs; k++) buf = strappend(buf, &len, &cap, "\\");
                char c[2] = {*p, '\0'};
                buf = strappend(buf, &len, &cap, "%s", c);
            }
            nbs = 0;
        }
        buf = strappend(buf, &len, &cap, "\"");
    }
    if (!buf) buf = strdup("");
    return buf;
}

typedef struct {
    HANDLE pipe;
    char *out;
    size_t out_len, cap;
} ReaderCtx;

static DWORD WINAPI reader_thread(LPVOID arg) {
    ReaderCtx *ctx = arg;
    char buf[8192];
    DWORD n;
    while (ReadFile(ctx->pipe, buf, sizeof(buf), &n, NULL) && n > 0) {
        if (ctx->out_len + n + 1 > ctx->cap) {
            ctx->cap = ctx->out_len + n + 8192;
            ctx->out = xrealloc(ctx->out, ctx->cap);
        }
        memcpy(ctx->out + ctx->out_len, buf, n);
        ctx->out_len += n;
    }
    return 0;
}

/* capture: 0 = both stdout+stderr, 1 = stderr only, 2 = stdout only */
static ProcResult proc_run_once(char *const argv[], int timeout_sec, int capture, const char *cwd) {
    ProcResult r = {0};
    r.exit_code = -1;

    SECURITY_ATTRIBUTES sa = {sizeof(sa), NULL, TRUE};
    HANDLE read_h, write_h;
    if (!CreatePipe(&read_h, &write_h, &sa, 0)) {
        r.spawn_failed = true;
        return r;
    }
    SetHandleInformation(read_h, HANDLE_FLAG_INHERIT, 0);

    HANDLE nul_h = CreateFileA("NUL", GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE, &sa,
                               OPEN_EXISTING, 0, NULL);

    STARTUPINFOA si = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = nul_h;
    if (capture == 1) { /* stderr only */
        si.hStdOutput = nul_h;
        si.hStdError = write_h;
    } else if (capture == 2) { /* stdout only */
        si.hStdOutput = write_h;
        si.hStdError = nul_h;
    } else { /* both */
        si.hStdOutput = write_h;
        si.hStdError = write_h;
    }

    char *cmdline = build_cmdline(argv);
    PROCESS_INFORMATION pi = {0};
    BOOL ok = CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, cwd, &si, &pi);
    free(cmdline);
    CloseHandle(write_h);
    if (nul_h != INVALID_HANDLE_VALUE) CloseHandle(nul_h);
    if (!ok) {
        CloseHandle(read_h);
        r.spawn_failed = true;
        return r;
    }
    CloseHandle(pi.hThread);

    ReaderCtx ctx = {0};
    ctx.pipe = read_h;
    ctx.cap = 8192;
    ctx.out = malloc(ctx.cap);
    HANDLE reader = CreateThread(NULL, 0, reader_thread, &ctx, 0, NULL);

    DWORD wait = WaitForSingleObject(pi.hProcess, (DWORD)timeout_sec * 1000);
    if (wait == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, 1);
        WaitForSingleObject(pi.hProcess, 5000);
        r.timed_out = true;
    } else {
        DWORD exit_code = 0;
        GetExitCodeProcess(pi.hProcess, &exit_code);
        r.exit_code = (int)exit_code;
    }

    WaitForSingleObject(reader, INFINITE);
    CloseHandle(reader);
    CloseHandle(read_h);
    CloseHandle(pi.hProcess);

    r.out = ctx.out;
    r.out_len = ctx.out_len;
    if (!r.out) r.out = strdup("");
    else {
        r.out = xrealloc(r.out, r.out_len + 1);
        r.out[r.out_len] = '\0';
    }
    return r;
}

static void proc_free(ProcResult *r) {
    free(r->out);
}
#endif /* _WIN32 */

/* Wine occasionally crashes a child process with an internal page fault
 * (always the same address, inside ntdll/loader) when many wine processes
 * start/exit concurrently under --parallel. The child is killed with a
 * nonzero exit and "Unhandled page fault ... starting debugger" on
 * stderr. Retry such spurious crashes a couple of times before giving up,
 * since the underlying compile/run is otherwise unaffected. */
#define PROC_RUN_MAX_ATTEMPTS 3

static bool is_wine_crash(const ProcResult *r) {
    if (r->exit_code == 0 || r->timed_out || r->spawn_failed) return false;
    if (r->out && strstr(r->out, "Unhandled page fault") != NULL) return true;
    /* Wine's loader sometimes kills the child with exit code 5 and no
     * output at all during a wineserver-connection race when many wine
     * processes start concurrently; rcc itself never exits with 5. */
    return r->exit_code == 5 && (!r->out || r->out[0] == '\0');
}

static ProcResult proc_run_cwd(char *const argv[], int timeout_sec, int capture, const char *cwd) {
    ProcResult r;
    for (int attempt = 1;; attempt++) {
        r = proc_run_once(argv, timeout_sec, capture, cwd);
        if (attempt >= PROC_RUN_MAX_ATTEMPTS || !is_wine_crash(&r)) break;
        proc_free(&r);
    }
    return r;
}

static ProcResult proc_run(char *const argv[], int timeout_sec, int capture) {
    return proc_run_cwd(argv, timeout_sec, capture, NULL);
}

/* ── string / file helpers ───────────────────────────────────────── */

static char *slurp(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz < 0) {
        fclose(f);
        return NULL;
    }
    rewind(f);
    char *buf = malloc((size_t)sz + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    size_t n = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[n] = '\0';
    return buf;
}

static bool file_exists(const char *path) { return access(path, F_OK) == 0; }
static bool streq(const char *a, const char *b) { return strcmp(a, b) == 0; }
static bool contains(const char *h, const char *n) { return h && strstr(h, n) != NULL; }

static void strip_ansi(char *s) {
    char *d = s;
    while (*s) {
        if (*s == '\x1b' && s[1] == '[') {
            s += 2;
            while (*s && (*s < 'A' || *s > 'z')) s++;
            if (*s) s++;
        } else
            *d++ = *s++;
    }
    *d = '\0';
}

static char *normalize_output(const char *s, const char *base) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *buf = malloc(len * 2 + 2);
    if (!buf) return strdup(s);

    /* strip ANSI */
    char *w = buf;
    const char *r = s;
    while (*r) {
        if (*r == '\x1b' && r[1] == '[') {
            r += 2;
            while (*r && (*r < 'A' || *r > 'z')) r++;
            if (*r) r++;
        } else
            *w++ = *r++;
    }
    *w = '\0';

    /* strip trailing whitespace per line */
    char *p = buf, *dest = buf;
    while (*p) {
        char *nl = strchr(p, '\n');
        size_t ll = nl ? (size_t)(nl - p) : strlen(p);
        const char *ce = p + ll;
        while (ce > p && (ce[-1] == ' ' || ce[-1] == '\t' || ce[-1] == '\r')) ce--;
        size_t cl = (size_t)(ce - p);
        if (cl > 0) {
            memmove(dest, p, cl);
            dest += cl;
        }
        if (nl) {
            *dest++ = '\n';
            p = nl + 1;
        } else
            p += ll;
    }
    *dest = '\0';

    if (base && streq(base, "46_grep")) {
        char *r2 = buf, *w2 = buf;
        while (*r2) {
            if (r2[0] == '\\' && r2[1] == '\\' && r2[2] == '\\' && r2[3] == '\\') {
                *w2++ = '\\';
                *w2++ = '\\';
                r2 += 4;
            } else
                *w2++ = *r2++;
        }
        *w2 = '\0';
    }

    /* only add trailing newline for non-empty output (mirrors diff -Nbu behaviour) */
    size_t blen = strlen(buf);
    if (blen > 0 && buf[blen - 1] != '\n') {
        buf[blen] = '\n';
        buf[blen + 1] = '\0';
    }
    return buf;
}

static bool word_in(const char *s, const char *needle) {
    if (!s || !needle) return false;
    size_t nlen = strlen(needle);
    const char *p = s;
    while (*p) {
        while (*p == ' ' || *p == '\n') p++;
        if (strncmp(p, needle, nlen) == 0 &&
            (p[nlen] == ' ' || p[nlen] == '\n' || p[nlen] == '\0')) return true;
        while (*p && *p != ' ' && *p != '\n') p++;
    }
    return false;
}

static char *strappend(char *buf, size_t *len, size_t *cap, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int need = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (need <= 0) return buf;
    size_t n = (size_t)need;
    if (*len + n + 1 > *cap) {
        *cap = *len + n + 4096;
        buf = xrealloc(buf, *cap);
    }
    va_start(ap, fmt);
    vsnprintf(buf + *len, n + 1, fmt, ap);
    va_end(ap);
    *len += n;
    return buf;
}

/* Format an argv array into a single shell-style command line string.
 * The returned buffer is malloc'd; caller frees. */
static char *cmdline_from_argv(char *const argv[]) {
    size_t len = 0, cap = 0;
    char *buf = NULL;
    for (int i = 0; argv[i]; i++) {
        if (i) buf = strappend(buf, &len, &cap, " ");
        const char *a = argv[i];
        bool needs_quotes = a[0] == '\0' || strpbrk(a, " \t\n\r\"") != NULL;
        if (!needs_quotes) {
            buf = strappend(buf, &len, &cap, "%s", a);
            continue;
        }
        buf = strappend(buf, &len, &cap, "\"");
        for (const char *p = a; *p; p++) {
            if (*p == '"' || *p == '\\')
                buf = strappend(buf, &len, &cap, "\\");
            buf = strappend(buf, &len, &cap, "%c", *p);
        }
        buf = strappend(buf, &len, &cap, "\"");
    }
    if (!buf) buf = strdup("");
    return buf;
}

/* ── platform detection ──────────────────────────────────────────── */

static const char *platform = "linux";
/* platform_suffix: like `platform`, but with "_<compiler-basename>"
 * appended when `rcc` refers to an external/unknown compiler (e.g.
 * /usr/bin/gcc) rather than one of rcc's own binaries, so report
 * filenames don't clobber rcc's own reports. compiler_name is set in
 * that case (NULL for rcc's own binaries). */
static const char *platform_suffix = "linux";
static const char *compiler_name;
static bool is_arm64, is_darwin_cross, is_mingw_native, is_wine;
static char runner_cmd[512];
static bool has_runner;
static int g_num_workers = 0;
#ifndef ARM64_NATIVE
static char *arm64_sysroot;
#endif

#ifndef _WIN32
static int under_aarch64_qemu(void) {
    /* QEMU user-mode registers binfmt_misc handlers.
     * Check for aarch64 entry; if present the kernel transparently
     * invokes qemu-aarch64 for ARM64 ELF binaries. */
    return access("/proc/sys/fs/binfmt_misc/qemu-aarch64", F_OK) == 0;
}
#endif

static void detect_platform(const char *rcc_path) {
    struct utsname u;
    uname(&u);

    if (contains(rcc_path, "arm64-cross") || contains(rcc_path, "rcc-arm64")) {
        platform = "arm64_cross";
        is_arm64 = true;
#ifndef ARM64_NATIVE
        // run each test via qemu. better is to run run_tests_arm64 and all tests via qemu
        static const char *sysroots[] = {
            "/usr/aarch64-linux-gnu",
            "/usr/aarch64-redhat-linux/sys-root/fc43",
            "/usr/aarch64-redhat-linux/sys-root/fc44",
            "/usr/aarch64-linux-gnu/sys-root",
        };
        for (size_t i = 0; i < sizeof(sysroots) / sizeof(sysroots[0]); i++) {
            char path[256];
            snprintf(path, sizeof(path), "%s/lib/ld-linux-aarch64.so.1", sysroots[i]);
            if (access(path, F_OK) == 0) {
                snprintf(runner_cmd, sizeof(runner_cmd), "qemu-aarch64 -L %s", sysroots[i]);
                has_runner = true;
                arm64_sysroot = (char *)sysroots[i];
                break;
            }
            snprintf(path, sizeof(path), "%s/usr/include", sysroots[i]);
            if (access(path, F_OK) == 0) {
                snprintf(runner_cmd, sizeof(runner_cmd), "qemu-aarch64 -L %s", sysroots[i]);
                has_runner = true;
                arm64_sysroot = (char *)sysroots[i];
                break;
            }
        }
        if (!has_runner) {
            snprintf(runner_cmd, sizeof(runner_cmd), "qemu-aarch64");
            has_runner = true;
        }
#endif
    } else if (contains(rcc_path, "darwin-cross") || contains(rcc_path, "rcc-darwin")) {
        platform = "darwin_cross";
        is_darwin_cross = true;
        is_arm64 = true;
    } else if (contains(rcc_path, "mingw-cross")) {
        platform = "mingw_cross";
        snprintf(runner_cmd, sizeof(runner_cmd), "wine");
        has_runner = true;
    }
#ifdef _WIN32
    /* We are a Windows PE binary.  mingw_cross if under wine. */
    else if (GetProcAddress(GetModuleHandleA("ntdll.dll"), "wine_get_version")) {
        platform = "mingw_cross";
        is_mingw_native = true;
        is_wine = true;
    } else {
        platform = "mingw";
        is_mingw_native = true;
    }
#else
    else if (contains(rcc_path, ".exe")) {
        if (strcmp(u.sysname, "Linux") == 0 && access("/usr/bin/wine", X_OK) == 0) {
            platform = "mingw_cross";
            snprintf(runner_cmd, sizeof(runner_cmd), "wine");
            has_runner = true;
        } else {
            platform = "mingw";
            is_mingw_native = true;
        }
    } else {
        if (strcmp(u.sysname, "Darwin") == 0)
            platform = "arm64";
        else if (contains(u.sysname, "MINGW") || contains(u.sysname, "MSYS") ||
                 contains(u.sysname, "CYGWIN"))
            platform = "mingw";
        else {
            platform = u.sysname;
            if (strcmp(u.sysname, "Linux") == 0)
                platform = "linux";
        }
        if (strcmp(u.machine, "aarch64") == 0 || strcmp(u.machine, "arm64") == 0) {
            is_arm64 = true;
            if (under_aarch64_qemu())
                platform = "arm64_cross";
            else
                platform = "arm64";
        }
    }
#endif
}

/* Unit tests such as test_peep spawn many compiler subprocesses; under
 * cross runners (wine, qemu) the overhead easily exceeds the normal 5 s
 * budget, so give them a longer leash. */
static int unit_run_timeout(void) {
    return (has_runner || is_wine) ? 60 : 5;
}

/* ── rcc library API (dynamically loaded from rcc_lib.{dll,so,dylib}) ──
 * Shared typedefs for both the Windows (LoadLibrary) and POSIX (dlopen)
 * loaders below. */
typedef struct RCCLib RCCLib;
typedef RCCLib *(*rcc_lib_new_fn)(void);
typedef int (*rcc_lib_compile_file_fn)(RCCLib *, const char *);
typedef int (*rcc_lib_compile_file_ex_fn)(RCCLib *, const char *, const char *, const char *);
typedef int (*rcc_lib_compile_file_ex2_fn)(RCCLib *, const char *, const char *, const char *, const char *);
typedef void *(*rcc_lib_get_symbol_fn)(RCCLib *, const char *);
typedef const char *(*rcc_lib_output_path_fn)(const RCCLib *);
typedef void (*rcc_lib_delete_fn)(RCCLib *);
typedef int (*main_fn_t)(int, char **);

#ifdef _WIN32
/* rcc_lib.dll and try_run_exe_inprocess() both redirect the process-wide
 * stdout/stderr fds and (for rcc_lib) drive the compiler's global state.
 * Serialize them with a critical section so parallel workers don't race. */
static CRITICAL_SECTION g_inproc_cs;
static bool g_inproc_cs_init = false;

static void inproc_lock(void) {
    if (!g_inproc_cs_init) {
        InitializeCriticalSection(&g_inproc_cs);
        g_inproc_cs_init = true;
    }
    EnterCriticalSection(&g_inproc_cs);
}

static void inproc_unlock(void) {
    LeaveCriticalSection(&g_inproc_cs);
}

/* Try to run the binary in-process via LoadLibrary + GetProcAddress("main").
 * Returns true if successful (result in *pr), false if fallback needed. */
static bool try_run_exe_inprocess(const char *exe_path, ProcResult *pr,
                                  int timeout_sec) {
    (void)timeout_sec;
    HMODULE h = LoadLibraryA(exe_path);
    if (!h) return false;
    typedef int (*main_fn)(int, char **);
    _Pragma("GCC diagnostic push")
        _Pragma("GCC diagnostic ignored \"-Wcast-function-type\"")
            main_fn fn = (main_fn)GetProcAddress(h, "main");
    _Pragma("GCC diagnostic pop") if (!fn) {
        FreeLibrary(h);
        return false;
    }

    inproc_lock();

    /* Redirect stdout/stderr to pipes */
    HANDLE out_read = NULL, out_write = NULL;
    HANDLE err_read = NULL, err_write = NULL;
    SECURITY_ATTRIBUTES sa = {sizeof(sa), NULL, TRUE};
    if (!CreatePipe(&out_read, &out_write, &sa, 0)) {
        inproc_unlock();
        FreeLibrary(h);
        return false;
    }
    if (!CreatePipe(&err_read, &err_write, &sa, 0)) {
        inproc_unlock();
        CloseHandle(out_read);
        CloseHandle(out_write);
        FreeLibrary(h);
        return false;
    }
    SetHandleInformation(out_read, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(err_read, HANDLE_FLAG_INHERIT, 0);

    int saved_stdout = _dup(_fileno(stdout));
    int saved_stderr = _dup(_fileno(stderr));
    _dup2(_open_osfhandle((intptr_t)out_write, 0), _fileno(stdout));
    _dup2(_open_osfhandle((intptr_t)err_write, 0), _fileno(stderr));
    CloseHandle(out_write);
    CloseHandle(err_write);

    /* Build argv: [exe_path] */
    char *argv[2];
    argv[0] = (char *)exe_path;
    argv[1] = NULL;
    int exit_code = fn(1, argv);

    fflush(stdout);
    fflush(stderr);
    _dup2(saved_stdout, _fileno(stdout));
    _dup2(saved_stderr, _fileno(stderr));
    close(saved_stdout);
    close(saved_stderr);

    /* Read captured output */
    char buf[8192];
    DWORD n;
    size_t cap = 8192, len = 0;
    char *out = malloc(cap);
    while (ReadFile(out_read, buf, sizeof(buf), &n, NULL) && n > 0) {
        if (len + n + 1 > cap) {
            cap = len + n + 8192;
            out = xrealloc(out, cap);
        }
        memcpy(out + len, buf, n);
        len += n;
    }
    /* Also drain stderr into output */
    while (ReadFile(err_read, buf, sizeof(buf), &n, NULL) && n > 0) {
        if (len + n + 1 > cap) {
            cap = len + n + 8192;
            out = xrealloc(out, cap);
        }
        memcpy(out + len, buf, n);
        len += n;
    }
    out[len] = '\0';
    CloseHandle(out_read);
    CloseHandle(err_read);
    FreeLibrary(h);

    pr->out = out;
    pr->out_len = len;
    pr->exit_code = exit_code;
    pr->timed_out = false;
    pr->spawn_failed = false;
    inproc_unlock();
    return true;
}

/* Cast GetProcAddress through uintptr_t to avoid -Wcast-function-type */
#define GETPROC(h, name, type) ((type)(uintptr_t)GetProcAddress((h), (name)))

static HMODULE rcc_lib_dll = NULL;
static rcc_lib_new_fn p_rcc_lib_new = NULL;
static rcc_lib_compile_file_fn p_rcc_lib_compile_file = NULL;
static rcc_lib_compile_file_ex_fn p_rcc_lib_compile_file_ex = NULL;
static rcc_lib_compile_file_ex2_fn p_rcc_lib_compile_file_ex2 = NULL;
static rcc_lib_get_symbol_fn p_rcc_lib_get_symbol = NULL;
static rcc_lib_output_path_fn p_rcc_lib_output_path = NULL;
static rcc_lib_delete_fn p_rcc_lib_delete = NULL;

static void init_rcc_lib(void) {
    if (rcc_lib_dll) return;
    /* Try loading from the run_tests.exe directory first —
     * the working directory may have been changed by chdir() */
    char exe_dir[MAX_PATH];
    if (GetModuleFileNameA(NULL, exe_dir, sizeof(exe_dir))) {
        char *slash = strrchr(exe_dir, '\\');
        if (slash) {
            *(slash + 1) = '\0';
            size_t dlen = strlen(exe_dir);
            snprintf(exe_dir + dlen, sizeof(exe_dir) - dlen, "rcc_lib.dll");
            rcc_lib_dll = LoadLibraryA(exe_dir);
        }
    }
    if (!rcc_lib_dll)
        rcc_lib_dll = LoadLibraryA("rcc_lib.dll");
    if (!rcc_lib_dll) return;
    p_rcc_lib_new = GETPROC(rcc_lib_dll, "rcc_lib_new", rcc_lib_new_fn);
    p_rcc_lib_compile_file = GETPROC(rcc_lib_dll, "rcc_lib_compile_file", rcc_lib_compile_file_fn);
    p_rcc_lib_compile_file_ex = GETPROC(rcc_lib_dll, "rcc_lib_compile_file_ex", rcc_lib_compile_file_ex_fn);
    p_rcc_lib_compile_file_ex2 = GETPROC(rcc_lib_dll, "rcc_lib_compile_file_ex2", rcc_lib_compile_file_ex2_fn);
    p_rcc_lib_get_symbol = GETPROC(rcc_lib_dll, "rcc_lib_get_symbol", rcc_lib_get_symbol_fn);
    p_rcc_lib_output_path = GETPROC(rcc_lib_dll, "rcc_lib_output_path", rcc_lib_output_path_fn);
    p_rcc_lib_delete = GETPROC(rcc_lib_dll, "rcc_lib_delete", rcc_lib_delete_fn);
    if (!p_rcc_lib_new || !p_rcc_lib_compile_file ||
        !p_rcc_lib_get_symbol || !p_rcc_lib_delete) {
        FreeLibrary(rcc_lib_dll);
        rcc_lib_dll = NULL;
    }
}

/* Compile src_path to a DLL via rcc_lib, load it, call main(), capture
 * output.  include_dir/cflags/extra_link_flags may be NULL.  extra_argv
 * (may be NULL) is a NULL-terminated array of extra argv entries appended
 * after argv[0].  cwd (may be NULL) is chdir'd to for the duration of the
 * compile+run.  Returns 0 on success, -1 if fallback to external rcc
 * is needed. */
static int run_test_inprocess(const char *src_path, const char *name,
                              const char *include_dir, const char *cflags,
                              const char *extra_link_flags,
                              char *const *extra_argv, const char *cwd,
                              ProcResult *pr, int timeout_sec) {
    (void)timeout_sec;
    init_rcc_lib();
    if (!rcc_lib_dll) return -1;

    inproc_lock();

    char saved_cwd[PATH_MAX];
    bool chdir_done = false;
    if (cwd && cwd[0]) {
        if (getcwd(saved_cwd, sizeof(saved_cwd)) && chdir(cwd) == 0)
            chdir_done = true;
    }

    RCCLib *lib = p_rcc_lib_new();
    if (!lib) {
        if (chdir_done) chdir(saved_cwd);
        inproc_unlock();
        return -1;
    }

    int saved_stdout = dup(STDOUT_FILENO);
    int cres;
    if (p_rcc_lib_compile_file_ex2)
        cres = p_rcc_lib_compile_file_ex2(lib, src_path, include_dir, cflags, extra_link_flags);
    else if (p_rcc_lib_compile_file_ex)
        cres = p_rcc_lib_compile_file_ex(lib, src_path, include_dir, extra_link_flags);
    else
        cres = p_rcc_lib_compile_file(lib, src_path);
    /* Restore stdout fd: rcc_lib internally fclose(stdout) while we
     * held a dup of fd 1.  dup2 the saved fd back to fd 1, then
     * freopen reinitializes the FILE* struct.  We must keep saved_stdout
     * alive through freopen because freopen closes stdout's old fd (fd 1);
     * afterwards we dup2 saved_stdout back to fd 1 so stdout writes go
     * to the original destination (pipe, file, or console), never lost
     * to the Windows console.  On CI where stdout is a pipe, the old
     * code lost output because freopen("CONOUT$") consumed fd 1 and
     * the subsequent _dup2 was a no-op (CONOUT$ already got fd 1). */
    dup2(saved_stdout, STDOUT_FILENO);
    /* keep saved_stdout alive — needed after freopen to restore fd 1 */
    if (!freopen("CONOUT$", "w", stdout))
        freopen("NUL", "w", stdout);
    /* redirect fd 1 back to the original stdout we saved */
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
    /* If CONOUT$ opened on a fd other than 1, point stdout's fd at fd 1 */
    if (_fileno(stdout) >= 0 && _fileno(stdout) != STDOUT_FILENO)
        _dup2(STDOUT_FILENO, _fileno(stdout));
    if (cres != 0) {
        p_rcc_lib_delete(lib);
        if (chdir_done) chdir(saved_cwd);
        inproc_unlock();
        return -1;
    }

    main_fn_t fn = (main_fn_t)p_rcc_lib_get_symbol(lib, "main");
    if (!fn) {
        p_rcc_lib_delete(lib);
        if (chdir_done) chdir(saved_cwd);
        inproc_unlock();
        return -1;
    }

    /* Build argv: [name, extra_argv..., NULL] */
    char *argv[16];
    int argc = 0;
    argv[argc++] = (char *)name;
    for (int i = 0; extra_argv && extra_argv[i] && argc < 15; i++)
        argv[argc++] = extra_argv[i];
    argv[argc] = NULL;

    /* Capture stdout/stderr via pipes */
    HANDLE out_r = NULL, out_w = NULL;
    SECURITY_ATTRIBUTES sa = {sizeof(sa), NULL, TRUE};
    if (!CreatePipe(&out_r, &out_w, &sa, 0)) {
        p_rcc_lib_delete(lib);
        if (chdir_done) chdir(saved_cwd);
        inproc_unlock();
        return -1;
    }
    SetHandleInformation(out_r, HANDLE_FLAG_INHERIT, 0);

    int saved_out = _dup(_fileno(stdout));
    int saved_err = _dup(_fileno(stderr));
    _dup2(_open_osfhandle((intptr_t)out_w, 0), _fileno(stdout));
    _dup2(_open_osfhandle((intptr_t)out_w, 0), _fileno(stderr));
    CloseHandle(out_w);

    int exit_code = fn(argc, argv);

    fflush(stdout);
    fflush(stderr);
    _dup2(saved_out, _fileno(stdout));
    _dup2(saved_err, _fileno(stderr));
    close(saved_out);
    close(saved_err);

    /* Read output */
    char buf[8192];
    DWORD n;
    size_t cap = 8192, len = 0;
    char *out = malloc(cap);
    while (ReadFile(out_r, buf, sizeof(buf), &n, NULL) && n > 0) {
        if (len + n + 1 > cap) {
            cap = len + n + 8192;
            out = xrealloc(out, cap);
        }
        memcpy(out + len, buf, n);
        len += n;
    }
    out[len] = '\0';
    CloseHandle(out_r);

    pr->out = out;
    pr->out_len = len;
    pr->exit_code = exit_code;
    pr->timed_out = false;
    pr->spawn_failed = false;

    p_rcc_lib_delete(lib);
    if (chdir_done) chdir(saved_cwd);
    inproc_unlock();
    return 0;
}
#else /* !_WIN32 */

/* rcc_lib.{so,dylib} and run_test_inprocess() both drive the compiler's
 * process-wide global state.  Serialize them with a mutex so parallel
 * workers don't race. */
static pthread_mutex_t g_inproc_mutex = PTHREAD_MUTEX_INITIALIZER;

static void inproc_lock(void) { pthread_mutex_lock(&g_inproc_mutex); }
static void inproc_unlock(void) { pthread_mutex_unlock(&g_inproc_mutex); }

/* defined below, in the TCC suite globals section */
static const char *SCRIPT_DIR;

static void *rcc_lib_handle = NULL;
static rcc_lib_new_fn p_rcc_lib_new = NULL;
static rcc_lib_compile_file_fn p_rcc_lib_compile_file = NULL;
static rcc_lib_compile_file_ex_fn p_rcc_lib_compile_file_ex = NULL;
static rcc_lib_compile_file_ex2_fn p_rcc_lib_compile_file_ex2 = NULL;
static rcc_lib_get_symbol_fn p_rcc_lib_get_symbol = NULL;
static rcc_lib_output_path_fn p_rcc_lib_output_path = NULL;
static rcc_lib_delete_fn p_rcc_lib_delete = NULL;

static void init_rcc_lib(void) {
    if (rcc_lib_handle) return;
#ifdef __APPLE__
    static const char *names[] = {"./rcc_lib.dylib", "rcc_lib.dylib"};
#else
    static const char *names[] = {"./rcc_lib.so", "rcc_lib.so"};
#endif
    char path[PATH_MAX];
    if (SCRIPT_DIR) {
#ifdef __APPLE__
        snprintf(path, sizeof(path), "%s/rcc_lib.dylib", SCRIPT_DIR);
#else
        snprintf(path, sizeof(path), "%s/rcc_lib.so", SCRIPT_DIR);
#endif
        rcc_lib_handle = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
    }
    for (size_t i = 0; !rcc_lib_handle && i < sizeof(names) / sizeof(names[0]); i++)
        rcc_lib_handle = dlopen(names[i], RTLD_LAZY | RTLD_LOCAL);
    if (!rcc_lib_handle) return;
    p_rcc_lib_new = (rcc_lib_new_fn)dlsym(rcc_lib_handle, "rcc_lib_new");
    p_rcc_lib_compile_file = (rcc_lib_compile_file_fn)dlsym(rcc_lib_handle, "rcc_lib_compile_file");
    p_rcc_lib_compile_file_ex = (rcc_lib_compile_file_ex_fn)dlsym(rcc_lib_handle, "rcc_lib_compile_file_ex");
    p_rcc_lib_compile_file_ex2 = (rcc_lib_compile_file_ex2_fn)dlsym(rcc_lib_handle, "rcc_lib_compile_file_ex2");
    p_rcc_lib_get_symbol = (rcc_lib_get_symbol_fn)dlsym(rcc_lib_handle, "rcc_lib_get_symbol");
    p_rcc_lib_output_path = (rcc_lib_output_path_fn)dlsym(rcc_lib_handle, "rcc_lib_output_path");
    p_rcc_lib_delete = (rcc_lib_delete_fn)dlsym(rcc_lib_handle, "rcc_lib_delete");
    if (!p_rcc_lib_new || !p_rcc_lib_compile_file ||
        !p_rcc_lib_get_symbol || !p_rcc_lib_delete) {
        dlclose(rcc_lib_handle);
        rcc_lib_handle = NULL;
    }
}

/* Compile src_path to a shared object via rcc_lib, dlopen it, call main(),
 * capture output.  See the _WIN32 implementation above for parameter docs. */
static int run_test_inprocess(const char *src_path, const char *name,
                              const char *include_dir, const char *cflags,
                              const char *extra_link_flags,
                              char *const *extra_argv, const char *cwd,
                              ProcResult *pr, int timeout_sec) {
    (void)timeout_sec;
    init_rcc_lib();
    if (!rcc_lib_handle) return -1;

    inproc_lock();

    char saved_cwd[PATH_MAX];
    bool chdir_done = false;
    if (cwd && cwd[0]) {
        if (getcwd(saved_cwd, sizeof(saved_cwd)) && chdir(cwd) == 0)
            chdir_done = true;
    }

    RCCLib *lib = p_rcc_lib_new();
    if (!lib) {
        if (chdir_done && chdir(saved_cwd) != 0) perror("chdir");
        inproc_unlock();
        return -1;
    }

    /* Redirect stdout/stderr to a pipe while compiling (rcc_lib writes
     * generated assembly via stdout, then reopens it itself; capture any
     * stray diagnostics too) and while running the compiled main(). */
    int pfd[2];
    if (pipe(pfd) != 0) {
        p_rcc_lib_delete(lib);
        if (chdir_done && chdir(saved_cwd) != 0) perror("chdir");
        inproc_unlock();
        return -1;
    }
    /* flush any output buffered by run_tests itself before redirecting fd
     * 1/2, otherwise it gets flushed into our capture pipe later (e.g. when
     * rcc_lib does fclose(stdout)) */
    fflush(NULL);

    int saved_out = dup(STDOUT_FILENO);
    int saved_err = dup(STDERR_FILENO);
    /* extra reference to the pipe's write end that survives rcc_lib's
     * fclose(stdout) (which closes fd 1) */
    int pipe_w2 = dup(pfd[1]);
    dup2(pfd[1], STDOUT_FILENO);
    dup2(pfd[1], STDERR_FILENO);
    close(pfd[1]);

    int cres;
    if (p_rcc_lib_compile_file_ex2)
        cres = p_rcc_lib_compile_file_ex2(lib, src_path, include_dir, cflags, extra_link_flags);
    else if (p_rcc_lib_compile_file_ex)
        cres = p_rcc_lib_compile_file_ex(lib, src_path, include_dir, extra_link_flags);
    else
        cres = p_rcc_lib_compile_file(lib, src_path);

    /* rcc_lib redirects stdout to the assembly file and fclose()s it,
     * closing fd 1; restore fd 1 -> pipe and give it a fresh FILE* so the
     * test program's printf() (via stdout) is captured correctly. */
    dup2(pipe_w2, STDOUT_FILENO);
    close(pipe_w2);
    stdout = fdopen(STDOUT_FILENO, "w");

    main_fn_t fn = cres == 0 ? (main_fn_t)p_rcc_lib_get_symbol(lib, "main") : NULL;
    if (!fn) {
        fflush(stdout);
        fflush(stderr);
        dup2(saved_out, STDOUT_FILENO);
        dup2(saved_err, STDERR_FILENO);
        close(saved_out);
        close(saved_err);
        close(pfd[0]);
        p_rcc_lib_delete(lib);
        if (chdir_done && chdir(saved_cwd) != 0) perror("chdir");
        inproc_unlock();
        return -1;
    }

    /* Build argv: [name, extra_argv..., NULL] */
    char *argv[16];
    int argc = 0;
    argv[argc++] = (char *)name;
    for (int i = 0; extra_argv && extra_argv[i] && argc < 15; i++)
        argv[argc++] = extra_argv[i];
    argv[argc] = NULL;

    fflush(stdout);
    fflush(stderr);

    /* Run the compiled main() in a forked child process: generated code
     * with a stack/register-clobbering bug, or a call to exit()/abort(),
     * would otherwise corrupt or terminate run_tests itself.  fd 1/2
     * (pointing at the pipe) are inherited by the child; it never returns
     * into run_tests' own code. */
    int exit_code;
    pid_t cpid = fork();
    if (cpid == 0) {
        /* exit() (not _exit()): run atexit handlers and ELF/.so destructors
         * (__attribute__((destructor))) registered by the compiled test, and
         * flush stdio, matching how a real standalone binary would exit. */
        exit(fn(argc, argv) & 0xff);
    }

    /* Parent: restore fd 1/2 right away so our own output doesn't go to
     * the pipe. */
    dup2(saved_out, STDOUT_FILENO);
    dup2(saved_err, STDERR_FILENO);
    close(saved_out);
    close(saved_err);

    if (cpid < 0) {
        close(pfd[0]);
        p_rcc_lib_delete(lib);
        if (chdir_done && chdir(saved_cwd) != 0) perror("chdir");
        inproc_unlock();
        return -1;
    }

    int status;
    bool timed_out = false;
    if (timeout_sec > 0) {
        time_t deadline = time(NULL) + timeout_sec;
        for (;;) {
            pid_t w = waitpid(cpid, &status, WNOHANG);
            if (w == cpid) break;
            if (time(NULL) >= deadline) {
                kill(cpid, SIGKILL);
                waitpid(cpid, &status, 0);
                timed_out = true;
                break;
            }
            struct timespec ts = {0, 10 * 1000 * 1000}; /* 10ms */
            nanosleep(&ts, NULL);
        }
    } else {
        waitpid(cpid, &status, 0);
    }

    if (WIFEXITED(status))
        exit_code = WEXITSTATUS(status);
    else if (WIFSIGNALED(status))
        exit_code = 128 + WTERMSIG(status);
    else
        exit_code = -1;

    /* Read captured output (compile diagnostics + program output) */
    char buf[8192];
    ssize_t n;
    size_t cap = 8192, len = 0;
    char *out = malloc(cap);
    while ((n = read(pfd[0], buf, sizeof(buf))) > 0) {
        if (len + (size_t)n + 1 > cap) {
            cap = len + (size_t)n + 8192;
            out = xrealloc(out, cap);
        }
        memcpy(out + len, buf, (size_t)n);
        len += (size_t)n;
    }
    out[len] = '\0';
    close(pfd[0]);

    pr->out = out;
    pr->out_len = len;
    pr->exit_code = exit_code;
    pr->timed_out = timed_out;
    pr->spawn_failed = false;

    p_rcc_lib_delete(lib);
    if (chdir_done && chdir(saved_cwd) != 0) perror("chdir");
    inproc_unlock();
    return 0;
}
#endif /* _WIN32 */

static ProcResult run_exe(const char *exe_path, const char *args, int timeout_sec) {
#ifdef _WIN32
    /* Try in-process execution: if the binary was compiled as a DLL
     * (LoadLibrary succeeds and has a main export), run it without
     * spawning a new wine process.  Falls back to proc_run on failure.
     * Restricted to the main thread (g_num_workers <= 1): under wine,
     * the msvcrt stdio fd-redirection in try_run_exe_inprocess() deadlocks
     * (RtlpWaitForCriticalSection wait-timed-out) when invoked from a
     * worker thread, even with inproc_lock held — this is a CRT-internal
     * deadlock, not an application-level data race. */
    if (g_num_workers <= 1) {
        ProcResult r;
        if (try_run_exe_inprocess(exe_path, &r, timeout_sec))
            return r;
    }
#endif
    char *argv[32];
    int ai = 0;
    if (has_runner) {
        char *rc = strdup(runner_cmd);
        char *save = NULL;
        char *tok = strtok_r(rc, " ", &save);
        while (tok && ai < 30) {
            argv[ai++] = tok;
            tok = strtok_r(NULL, " ", &save);
        }
    }
    argv[ai++] = (char *)exe_path;
    if (args && *args) {
        char *ac = strdup(args);
        char *save = NULL;
        char *tok = strtok_r(ac, " ", &save);
        while (tok && ai < 31) {
            argv[ai++] = tok;
            tok = strtok_r(NULL, " ", &save);
        }
    }
    argv[ai] = NULL;
    return proc_run(argv, timeout_sec, 2);
}

/* Like run_exe, but also returns the command line used (malloc'd). */
static ProcResult run_exe_with_cmdline(const char *exe_path, const char *args,
                                       int timeout_sec, char **cmdline_out) {
    if (cmdline_out) *cmdline_out = NULL;
#ifdef _WIN32
    if (g_num_workers <= 1) {
        ProcResult r;
        if (try_run_exe_inprocess(exe_path, &r, timeout_sec)) {
            if (cmdline_out) {
                char *argv[2];
                argv[0] = (char *)exe_path;
                argv[1] = NULL;
                *cmdline_out = cmdline_from_argv(argv);
            }
            return r;
        }
    }
#endif
    char *argv[32];
    int ai = 0;
    if (has_runner) {
        char *rc = strdup(runner_cmd);
        char *save = NULL;
        char *tok = strtok_r(rc, " ", &save);
        while (tok && ai < 30) {
            argv[ai++] = tok;
            tok = strtok_r(NULL, " ", &save);
        }
    }
    argv[ai++] = (char *)exe_path;
    if (args && *args) {
        char *ac = strdup(args);
        char *save = NULL;
        char *tok = strtok_r(ac, " ", &save);
        while (tok && ai < 31) {
            argv[ai++] = tok;
            tok = strtok_r(NULL, " ", &save);
        }
    }
    argv[ai] = NULL;
    if (cmdline_out) *cmdline_out = cmdline_from_argv(argv);
    return proc_run(argv, timeout_sec, 2);
}

static char **list_c_files_sorted(const char *dir) {
    struct dirent **nl;
    int n = scandir(dir, &nl, NULL, versionsort);
    if (n < 0) return NULL;
    char **files = calloc((size_t)n + 1, sizeof(char *));
    int j = 0;
    for (int i = 0; i < n; i++) {
        const char *name = nl[i]->d_name;
        size_t len = strlen(name);
        if (len > 2 && strcmp(name + len - 2, ".c") == 0) {
            size_t plen = strlen(dir) + 1 + len + 1;
            files[j] = malloc(plen);
            snprintf(files[j], plen, "%s/%s", dir, name);
            j++;
        }
        free(nl[i]);
    }
    free(nl);
    return files;
}

/* ═══════════════════════════════════════════════════════════════════
 * TCC COMPATIBILITY TEST SUITE
 * ═══════════════════════════════════════════════════════════════════ */

#define SKIP_TESTS \
    " 60_errors_and_warnings " \
    " 96_nodata_wanted " \
    " 98_al_ax_extend " \
    " 99_fastcall " \
    " 112_backtrace " \
    " 113_btdll " \
    " 114_bound_signal " \
    " 115_bound_setjmp " \
    " 116_bound_setjmp2 " \
    " 120_alias " \
    " 126_bound_global " \
    " 141_riscv_asm " \
    " 145_winarm64_interlocked "

#define NON_WINDOWS_SKIP " 145_winarm64_interlocked "
#define INTEL_SKIP \
    " 73_arm64 " " 138_arm64_encoding " " 139_arm64_errors " " 140_arm64_extasm "
#define ARM64_SKIP " 95_bitfields_ms " " 127_asm_goto "
#define CD_TESTS " 125_atomic_misc " " 129_scopes " " 139_arm64_errors "
#define DT_TESTS " 125_atomic_misc " " 139_arm64_errors "

static bool is_skipped(const char *base, bool is_mingw) {
    if (word_in(SKIP_TESTS, base)) return true;
    if (!is_mingw && word_in(NON_WINDOWS_SKIP, base)) return true;
    return is_arm64 ? word_in(ARM64_SKIP, base) : word_in(INTEL_SKIP, base);
}
static bool is_cd_test(const char *base) { return word_in(CD_TESTS, base); }
static bool is_dt_test(const char *base) { return word_in(DT_TESTS, base); }

static const char *test_args(const char *base) {
    if (streq(base, "31_args")) return "arg1 arg2 arg3 arg4 arg5";
    if (streq(base, "46_grep"))
        return "'[^* ]*[:a:d: ]+\\:\\*-/: $' tinycc/tests/tests2/46_grep.c";
    if (streq(base, "128_run_atexit")) return "1";
    return "";
}
static int test_expected_exit(const char *base) {
    if (streq(base, "101_cleanup")) return 105;
    return 0;
}
static int test_unit_expected_exit(const char *base) {
    if (streq(base, "test_include")) return 42;
    if (streq(base, "test_include2")) return 10;
    if (streq(base, "test_self_include2")) return 1;
    if (streq(base, "test_simple")) return 1;
    if (streq(base, "test_simple2")) return 1;
    return 0;
}
static const char *extra_ldflags(const char *base, const char *srcfile) {
    bool lm = streq(base, "22_floating_point") || streq(base, "24_math_library");
    bool pt = false;
    if (srcfile) {
        char *c = slurp(srcfile);
        if (c) {
            pt = contains(c, "pthread_");
            free(c);
        }
    }
    if (lm && pt) return " -lm -pthread";
    if (lm) return " -lm";
    if (pt) return " -pthread";
    return "";
}

static bool diff_strings(const char *expect, const char *actual, const char *label) {
    /* treat NULL and empty string as equivalent — mirrors diff -Nbu on empty files */
    const char *e = (expect && *expect) ? expect : NULL;
    const char *a = (actual && *actual) ? actual : NULL;
    if (!e && !a) return true;
    if (!e || !a) return false;
    if (strcmp(e, a) == 0) return true;
    size_t elen = strlen(e), alen = strlen(a);
    if (elen > 0 && e[elen - 1] == '\n' && alen == elen - 1 && memcmp(e, a, alen) == 0)
        return true;
    if (alen > 0 && a[alen - 1] == '\n' && elen == alen - 1 && memcmp(e, a, elen) == 0)
        return true;
    printf("--- expected (%s)\n+++ actual\n@@ output differs @@\n", label);
    /* Print the first differing line from each */
    {
        const char *es = e, *as = a;
        while (*es && *as && *es == *as) es++, as++;
        /* back up to start of line */
        while (es > e && es[-1] != '\n') es--;
        while (as > a && as[-1] != '\n') as--;
        /* print the line from expect */
        const char *ee = strchr(es, '\n');
        if (!ee) ee = es + strlen(es);
        printf("-%.*s\n", (int)(ee - es), es);
        /* print the line from actual */
        const char *ae = strchr(as, '\n');
        if (!ae) ae = as + strlen(as);
        printf("+%.*s\n", (int)(ae - as), as);
    }
    return false;
}

static char **extract_dt_tests(const char *src_path) {
    char *content = slurp(src_path);
    if (!content) return NULL;
    char **names = NULL;
    int ncount = 0, ncap = 0;
    char *p = content;
    while (*p) {
        char *def = strstr(p, "defined");
        if (!def) break;
        const char *s = def + 7;
        while (*s == ' ' || *s == '\t') s++;
        if (*s == '(') s++;
        if (strncmp(s, "test_", 5) == 0) {
            const char *start = s;
            while ((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z') || (*s >= '0' && *s <= '9') || *s == '_') s++;
            size_t len = (size_t)(s - start);
            if (len > 0) {
                bool dup = false;
                for (int i = 0; i < ncount; i++)
                    if (strncmp(names[i], start, len) == 0 && names[i][len] == '\0') {
                        dup = true;
                        break;
                    }
                if (!dup) {
                    if (ncount >= ncap) {
                        ncap = ncap ? ncap * 2 : 16;
                        names = xrealloc(names, (size_t)ncap * sizeof(char *));
                    }
                    names[ncount++] = strndup(start, len);
                }
            }
        }
        p = def + 7;
    }
    free(content);
    if (ncount == 0) {
        free(names);
        return NULL;
    }
    names = xrealloc(names, (size_t)(ncount + 1) * sizeof(char *));
    names[ncount] = NULL;
    return names;
}

/* On macOS, lldb needs task_for_pid access which is only granted when
 * launched from a proper Terminal session.  In CI / SSH / launchd
 * contexts it fails with "cannot get permission to debug processes".
 * Probe once and cache the result so we never try lldb when it can't work. */
#ifdef __APPLE__
static bool lldb_probed(void) {
    static int ok = -1; /* -1 = unchecked, 0 = no, 1 = yes */
    if (ok >= 0) return ok;
    const char *bin = "/usr/bin/true";
    if (access(bin, X_OK) != 0)
        bin = "/bin/true";
    if (access(bin, X_OK) != 0)
        bin = "/bin/ls";
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "lldb --batch -o quit -- %s 2>&1", bin);
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        ok = 0;
        return false;
    }
    char ln[512];
    bool can = true;
    while (fgets(ln, sizeof(ln), fp)) {
        if (strstr(ln, "cannot get permission") ||
            strstr(ln, "error: process launch failed")) {
            can = false;
            break;
        }
    }
    int rc = pclose(fp);
    ok = (can && rc == 0) ? 1 : 0;
    return ok;
}
#endif

/* extra: NULL-terminated extra compile flags (e.g. {"-I",".","-lm",NULL}); may be NULL.
 * ob/ol/oc: output buffer for TCC suite (append mode); pass all NULL to print directly. */
static void emit_backtrace(const char *exe_path, const char *args,
                           const char *src_file, const char *rcc, const char *cflags,
                           const char *const extra[],
                           char **ob, size_t *ol, size_t *oc) {
    static bool lldb_failed, gdb_failed;
    char dbg[512];
    snprintf(dbg, sizeof(dbg), "%s.dbg", exe_path);
    bool have_dbg = false;
    if (src_file && file_exists(src_file)) {
        char *a[32];
        int ai = 0;
        a[ai++] = (char *)rcc;
        if (cflags && *cflags)
            a[ai++] = (char *)cflags;
        if (extra) {
            for (int i = 0; extra[i] && ai < 28; i++)
                a[ai++] = (char *)extra[i];
        }
        a[ai++] = "-g";
        a[ai++] = "-o";
        a[ai++] = dbg;
        a[ai++] = (char *)src_file;
        a[ai] = NULL;
        ProcResult cr = proc_run(a, 30, 1);
        if (cr.exit_code == 0)
            have_dbg = true;
        proc_free(&cr);
    }
    const char *dp = have_dbg ? dbg : exe_path;
#define BT_OUT(fmt, ...) do { \
    if (ob) *ob = strappend(*ob, ol, oc, fmt, ##__VA_ARGS__); \
    else logprintf(fmt, ##__VA_ARGS__); \
} while (0)
    if (access("/usr/bin/lldb", X_OK) == 0 && !lldb_failed
#ifdef __APPLE__
        && lldb_probed()
#endif
    ) {
        BT_OUT("\n=== EXEC_FAIL backtrace ===\n");
        BT_OUT("command: %s %s\n", dp, args ? args : "");
        char cmd[1024];
        snprintf(cmd, sizeof(cmd),
                 "lldb --batch --one-line run --one-line-on-crash \"thread backtrace all\" "
                 "--one-line-on-crash \"register read pc lr x0 x1 x2 x16 x17\" "
                 "--one-line-on-crash \"quit 1\" -- %s %s </dev/null 2>&1",
                 dp, args ? args : "");
        FILE *fp = popen(cmd, "r");
        if (fp) {
            char ln[1024];
            while (fgets(ln, sizeof(ln), fp)) {
                if (strstr(ln, "cannot get permission to debug processes")) {
                    lldb_failed = true;
                    BT_OUT("lldb: permission denied (skipping)\n");
                    break;
                }
                BT_OUT("%s", ln);
            }
            pclose(fp);
        }
    } else if ((access("/usr/bin/gdb", X_OK) == 0
#ifdef _WIN32
                || access("/mingw64/bin/gdb.exe", X_OK) == 0 || access("/mingw32/bin/gdb.exe", X_OK) == 0
#endif
                ) &&
               !gdb_failed) {
        BT_OUT("\n=== EXEC_FAIL backtrace ===\n");
        BT_OUT("command: %s %s\n", dp, args ? args : "");
        char cmd[1024];
#ifdef _WIN32
        const char *gdb_path = access("/mingw64/bin/gdb.exe", X_OK) == 0
            ? "/mingw64/bin/gdb.exe"
            : access("/mingw32/bin/gdb.exe", X_OK) == 0
            ? "/mingw32/bin/gdb.exe"
            : "gdb";
#else
        const char *gdb_path = "gdb";
#endif
        snprintf(cmd, sizeof(cmd),
                 "%s -q -batch -ex run -ex \"thread apply all bt\" --args %s %s 2>&1",
                 gdb_path, dp, args ? args : "");
        FILE *fp = popen(cmd, "r");
        if (fp) {
            char ln[1024];
            while (fgets(ln, sizeof(ln), fp)) {
                BT_OUT("%s", ln);
                if (strstr(ln, "not permitted") || strstr(ln, "ptrace"))
                    gdb_failed = true;
            }
            pclose(fp);
        }
    } else if (!lldb_failed && !gdb_failed) {
        BT_OUT("\nNo debugger available for backtrace.\n");
    }
#undef BT_OUT
    if (have_dbg) unlink(dbg);
}


/* ── parallel test execution ─────────────────────────────────────── */

static bool g_verbose = false;
#ifndef _WIN32
static pthread_mutex_t g_verbose_mutex = PTHREAD_MUTEX_INITIALIZER;
#else
static CRITICAL_SECTION g_verbose_cs;
static bool g_verbose_cs_init = false;
#endif

static void vlog(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static void vlog(const char *fmt, ...) {
#ifdef _WIN32
    if (!g_verbose_cs_init) {
        InitializeCriticalSection(&g_verbose_cs);
        g_verbose_cs_init = true;
    }
    EnterCriticalSection(&g_verbose_cs);
#else
    pthread_mutex_lock(&g_verbose_mutex);
#endif
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    fflush(stdout);
#ifdef _WIN32
    LeaveCriticalSection(&g_verbose_cs);
#else
    pthread_mutex_unlock(&g_verbose_mutex);
#endif
}

/* Print the compile/run command lines and captured output for one test.
 * Safe to call from parallel workers via vlog(). */
static void vlog_test_details(const char *name, const char *compile_cmd,
                              const char *compile_out, const char *run_cmd,
                              const char *run_out) {
    if (!g_verbose) return;
    vlog("--- %s ---\n", name);
    if (compile_cmd && compile_cmd[0])
        vlog("compile: %s\n", compile_cmd);
    if (compile_out && compile_out[0])
        vlog("compile output:\n%s", compile_out);
    if (run_cmd && run_cmd[0])
        vlog("run: %s\n", run_cmd);
    if (run_out && run_out[0])
        vlog("run output:\n%s", run_out);
    vlog("\n");
}

/* Emit the exit code for a failed execution; timeouts are reported
 * explicitly so they are not mistaken for a plain nonzero exit. */
static void log_fail_exit(int exit_code, bool timed_out) {
    if (timed_out)
        fprintf(stderr, "    TIMEOUT\n");
    else
        fprintf(stderr, "    exit=%d\n", exit_code);
}

// Per-test result captured during parallel execution
typedef struct {
    int exit_code;
    char *compile_out;
    char *compile_cmdline;
    int exec_exit;
    char *exec_out;
    char *run_cmdline;
    char tmp_exe[256];
    bool did_compile;
    bool did_exec;
    bool exec_timed_out;
    // Compliance-specific: gcc execution
    int gcc_exec_exit;
    char *gcc_exec_out;
    char gcc_exe_path[PATH_MAX];
} ParallelResult;

typedef enum { SUITE_TCC,
               SUITE_TORTURE,
               SUITE_UNIT,
               SUITE_COMPLIANCE,
               SUITE_CTEST } SuiteType;

// Forward declarations for per-suite compile+exec functions
static void compile_and_exec(const char *src_path, const char *base,
                             const char *p_src, const char *ldflags, bool is_mingw,
                             ParallelResult *r, int index);
static void tort_compile_exec(const char *src_path, const char *base, bool summary_only,
                              ParallelResult *r, int index);
static void unit_compile_exec(const char *src_path, const char *base,
                              ParallelResult *r, int index);
static void comp_compile_exec(const char *src_path, const char *base,
                              const char *gcc_path, ParallelResult *r, int index);
static void ctest_compile_exec(const char *src_path, const char *base,
                               ParallelResult *r, int index);

#ifdef _WIN32
typedef HANDLE thread_t;
#define THREAD_CREATE(t, fn, arg) ((void)(*(t) = CreateThread(NULL,0,fn,arg,0,NULL)))
#define THREAD_JOIN(t) WaitForSingleObject(t, INFINITE)
#else
typedef pthread_t thread_t;
#define THREAD_CREATE(t, fn, arg) ((void)pthread_create(t, NULL, fn, arg))
#define THREAD_JOIN(t) pthread_join(t, NULL)
#endif
typedef struct {
    SuiteType suite;
    const char *src_path;
    const char *base;
    // TCC-specific
    const char *p_src;
    const char *ldflags;
    bool is_mingw;
    // Torture-specific
    bool summary_only;
    // Compliance-specific
    const char *gcc_path;
    // Generic
    ParallelResult result;
    int index;
} ParallelJob;

// Worker: compile and execute one test, capture results — dispatches by suite
#ifdef _WIN32
static DWORD WINAPI worker_compile_exec(LPVOID arg) {
#else
static void *worker_compile_exec(void *arg) {
#endif
    ParallelJob *job = (ParallelJob *)arg;
    switch (job->suite) {
    case SUITE_TCC:
        compile_and_exec(job->src_path, job->base, job->p_src, job->ldflags, job->is_mingw,
                         &job->result, job->index);
        break;
    case SUITE_TORTURE:
        tort_compile_exec(job->src_path, job->base, job->summary_only, &job->result, job->index);
        break;
    case SUITE_UNIT:
        unit_compile_exec(job->src_path, job->base, &job->result, job->index);
        break;
    case SUITE_COMPLIANCE:
        comp_compile_exec(job->src_path, job->base, job->gcc_path, &job->result, job->index);
        break;
    case SUITE_CTEST:
        ctest_compile_exec(job->src_path, job->base, &job->result, job->index);
        break;
    }
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

// Work-stealing state for bounded thread pool
static int g_pool_next;
static int g_pool_count;
static ParallelJob *g_pool_jobs;
#ifndef _WIN32
static pthread_mutex_t g_pool_mutex = PTHREAD_MUTEX_INITIALIZER;
#else
static CRITICAL_SECTION g_pool_cs;
static bool g_pool_cs_init = false;
#endif

static int pool_next_job(void) {
#ifdef _WIN32
    if (!g_pool_cs_init) {
        InitializeCriticalSection(&g_pool_cs);
        g_pool_cs_init = true;
    }
    EnterCriticalSection(&g_pool_cs);
    int idx = g_pool_next++;
    LeaveCriticalSection(&g_pool_cs);
#else
    pthread_mutex_lock(&g_pool_mutex);
    int idx = g_pool_next++;
    pthread_mutex_unlock(&g_pool_mutex);
#endif
    return idx;
}

#ifdef _WIN32
static DWORD WINAPI pool_worker(LPVOID arg) {
#else
static void *pool_worker(void *arg) {
#endif
    int tid = *(int *)arg;
    for (;;) {
        int idx = pool_next_job();
        if (idx >= g_pool_count) break;
        ParallelJob *job = &g_pool_jobs[idx];
        if (g_verbose) {
            const char *sname = "???";
            switch (job->suite) {
            case SUITE_TCC: sname = "tcc"; break;
            case SUITE_TORTURE: sname = "torture"; break;
            case SUITE_UNIT: sname = "unit"; break;
            case SUITE_COMPLIANCE: sname = "compliance"; break;
            case SUITE_CTEST: sname = "ctest"; break;
            }
            vlog("[T%d] job %d/%d: %s %s\n", tid, idx + 1, g_pool_count, sname, job->base);
        }
        worker_compile_exec(job);
        if (g_verbose)
            vlog("[T%d] job %d done\n", tid, idx + 1);
    }
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

// Dispatch jobs to thread pool (bounded by g_num_workers), wait for completion
static void parallel_dispatch(ParallelJob *jobs, int count) {
    if (g_num_workers < 1) g_num_workers = 1;
    g_pool_jobs = jobs;
    g_pool_count = count;
    g_pool_next = 0;

    int nw = g_num_workers;
    if (nw > count) nw = count;

    if (g_verbose)
        vlog("[pool] %d workers, %d jobs\n", nw, count);

    thread_t *threads = calloc((size_t)nw, sizeof(thread_t));
    int *tids = calloc((size_t)nw, sizeof(int));
    for (int i = 0; i < nw; i++) {
        tids[i] = i;
        THREAD_CREATE(&threads[i], pool_worker, &tids[i]);
    }
    for (int i = 0; i < nw; i++)
        THREAD_JOIN(threads[i]);
    free(tids);
    free(threads);

    if (g_verbose)
        vlog("[pool] done\n");
}
/* ── TCC suite globals ───────────────────────────────────────────── */

static const char *rcc, *rccflags, *TEST_DIR, *REPORT_FILE, *SCRIPT_DIR;
/* In-process mode: compile + run tests via rcc_lib instead of spawning
 * `rcc` per test.  Enabled by default when no rcc-binary positional arg
 * is given (see main()); disabled by an explicit binary/runner arg, by
 * "compiler flags" args (e.g. "ccc -O2"), and for cross-compile targets. */
static bool use_rcc_lib;
static const char *only_test;
static bool only_test_found; /* single-test mode: stop after the suite containing it */
static int total, passed, failed, todo, regressions, fixes, changed_cnt;

typedef struct {
    char *name, *status, *message;
} ReportRow;
static ReportRow *report_rows;
static int nrows, rows_cap;

static void add_row(const char *name, const char *status, const char *message) {
    if (nrows >= rows_cap) {
        rows_cap = rows_cap ? rows_cap * 2 : 256;
        report_rows = xrealloc(report_rows, (size_t)rows_cap * sizeof(ReportRow));
    }
    report_rows[nrows++] = (ReportRow){strdup(name), strdup(status), strdup(message)};
}

typedef struct {
    char *name, *status;
} OldState;
static OldState *old_states;
static int nold;

static void load_old_states(const char *report_file) {
    char *content = slurp(report_file);
    if (!content) return;
    char *line = content;
    int line_no = 0;
    while (*line) {
        char *end = strchr(line, '\n');
        if (end) *end = '\0';
        line_no++;
        if (line_no > 3 && line[0] == '|') {
            char *p = line + 1;
            while (*p == ' ') p++;
            char *ns = p;
            while (*p && *p != '|') p++;
            char *ne = p;
            while (ne > ns && ne[-1] == ' ') ne--;
            *ne = '\0';
            if (*p == '|') p++;
            while (*p == ' ') p++;
            char *ss = p;
            while (*p && *p != '|') p++;
            char *se = p;
            while (se > ss && se[-1] == ' ') se--;
            *se = '\0';
            if (*ns && *ss) {
                old_states = xrealloc(old_states, (size_t)(nold + 1) * sizeof(OldState));
                old_states[nold].name = strdup(ns);
                old_states[nold].status = strdup(ss);
                nold++;
            }
        }
        if (end) line = end + 1;
        else
            break;
    }
    free(content);
}

static const char *old_status_for(const char *name) {
    for (int i = 0; i < nold; i++)
        if (streq(old_states[i].name, name)) return old_states[i].status;
    return NULL;
}

static void print_change(const char *base, const char *new_status) {
    if (only_test) return;
    const char *old = old_status_for(base);
    if (!old) return;
    const char *oc = streq(old, "COMPILE_OK") ? "PASS" : old;
    const char *nc = streq(new_status, "COMPILE_OK") ? "PASS" : new_status;
    if (streq(oc, nc)) return;
    if (streq(nc, "PASS")) {
        printf("    %s-> FIXED%s (was %s)\n", COL_GREEN, COL_RESET, old);
        fixes++;
    } else if (streq(oc, "PASS")) {
        printf("    %s-> REGRESSION%s (was PASS)\n", COL_RED, COL_RESET);
        regressions++;
    } else {
        printf("    %s-> CHANGED%s (%s -> %s)\n", COL_YELLOW, COL_RESET, old, new_status);
        changed_cnt++;
    }
}

/* ── run one TCC test ────────────────────────────────────────────── */

static void run_one_test(const char *src_path, const char *base,
                         const char *p_src, const char *ldflags, bool is_mingw) {
    total++;
    char expect_file[512], local_expect[512];
    snprintf(local_expect, sizeof(local_expect), "%s/test/tinycc-%s.expect", SCRIPT_DIR, base);
    if (file_exists(local_expect)) snprintf(expect_file, sizeof(expect_file), "%s", local_expect);
    else
        snprintf(expect_file, sizeof(expect_file), "%s/%s.expect", TEST_DIR, base);

    bool in_cd_dir = false;
    const char *orig_src = src_path, *orig_rcc = rcc;
    const char *fname_only = strrchr(orig_src, '/');
    if (!fname_only) fname_only = orig_src;
    else
        fname_only++;

    /* In-process fast path (see compile_and_exec() for rationale): always
     * compile+run with cwd=TEST_DIR and a basename source path, skipping
     * the external rcc/exe process spawns entirely.  On any failure, fall
     * back to the external-rcc path below. */
    if (use_rcc_lib && !is_dt_test(base) && !streq(base, "128_run_atexit") &&
        !streq(base, "46_grep") && !is_darwin_cross) {
        const char *args = test_args(base);
        int expected_exit = test_expected_exit(base);

        char *extra_argv[8];
        int eai = 0;
        char *args_copy = NULL;
        if (args && *args) {
            args_copy = strdup(args);
            char *sv = NULL;
            for (char *tok = strtok_r(args_copy, " ", &sv); tok && eai < 7;
                 tok = strtok_r(NULL, " ", &sv))
                extra_argv[eai++] = tok;
        }
        extra_argv[eai] = NULL;

        char tmp_name[64];
        snprintf(tmp_name, sizeof(tmp_name), "rcc_test_%d", getpid());

        ProcResult pr;
        int rc = run_test_inprocess(fname_only, tmp_name, NULL, p_src, ldflags,
                                    extra_argv, TEST_DIR, &pr, 30);
        free(args_copy);
        if (rc == 0) {
            char *out_buf = pr.out;
            int actual_exit = pr.exit_code;
            if (actual_exit != expected_exit) {
                print_result(base, COL_RED, "EXEC FAIL");
                log_fail_exit(actual_exit, pr.timed_out);
                failed++;
                add_row(base, "EXEC_FAIL", "non-zero exit");
                print_change(base, "EXEC_FAIL");
                vlog_test_details(base, "(in-process compile)", NULL, "(in-process run)", out_buf);
                free(out_buf);
                return;
            }
            char *er = file_exists(expect_file) ? slurp(expect_file) : NULL;
            if (er) {
                char *en = normalize_output(er, base), *on = normalize_output(out_buf, base);
                if (diff_strings(en, on, base)) {
                    print_result(base, COL_GREEN, "PASS");
                    passed++;
                    add_row(base, "PASS", "Output matches");
                    print_change(base, "PASS");
                } else {
                    print_result(base, COL_RED, "MISMATCH");
                    failed++;
                    add_row(base, "MISMATCH", "Output does not match .expect");
                    print_change(base, "MISMATCH");
                    char sp[512], sp_tmp[516];
                    snprintf(sp, sizeof(sp), "%s/test/%s.out", SCRIPT_DIR, base);
                    snprintf(sp_tmp, sizeof(sp_tmp), "%s.tmp", sp);
                    FILE *sf = fopen(sp_tmp, "wb");
                    if (sf) {
                        if (out_buf) fputs(out_buf, sf);
                        fclose(sf);
                        move_file_atomic(sp_tmp, sp);
                    }
                }
                free(on);
                free(en);
                free(er);
            } else {
                print_result(base, COL_GREEN, "PASS (no expect)");
                passed++;
                add_row(base, "PASS", "Executed successfully (no .expect)");
                print_change(base, "PASS");
            }
            vlog_test_details(base, "(in-process compile)", NULL, "(in-process run)", out_buf);
            free(out_buf);
            return;
        }
    }

    if (is_cd_test(base)) {
        char rcc_abs[PATH_MAX];
        if (realpath(rcc, rcc_abs)) {
            static char buf[PATH_MAX];
            strncpy(buf, rcc_abs, sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';
            rcc = buf;
        }
        if (chdir(TEST_DIR) == 0) {
            src_path = fname_only;
            in_cd_dir = true;
        } else
            perror("chdir");
    }

    char tmp_exe[256];
    snprintf(tmp_exe, sizeof(tmp_exe), "%s/rcc_test_%d", get_tmpdir(), getpid());
    if (is_mingw || (has_runner && contains(runner_cmd, "wine")))
        strcat(tmp_exe, ".exe");
    char *out_buf = NULL;
    size_t out_len = 0, out_cap = 0;
    char *vlog_compile_cmd = NULL;
    char *vlog_run_cmd = NULL;

    /* DT tests */
    if (is_dt_test(base)) {
        char **dt = extract_dt_tests(src_path);
        if (dt) {
            for (char **tn = dt; *tn; tn++) {
                char *ca[16];
                int ai = 0;
                ca[ai++] = (char *)rcc;
                ca[ai++] = (char *)rccflags;
                char df[128];
                snprintf(df, sizeof(df), "-D%s", *tn);
                ca[ai++] = df;
                ca[ai++] = "-o";
                ca[ai++] = tmp_exe;
                if (p_src && *p_src) {
                    char *ps = strdup(p_src);
                    char *sv = NULL;
                    char *tok = strtok_r(ps, " ", &sv);
                    while (tok && ai < 14) {
                        ca[ai++] = tok;
                        tok = strtok_r(NULL, " ", &sv);
                    }
                }
                ca[ai++] = (char *)src_path;
                if (ldflags && *ldflags) {
                    char *lf = strdup(ldflags);
                    char *sv = NULL;
                    char *tok = strtok_r(lf, " ", &sv);
                    while (tok && ai < 15) {
                        ca[ai++] = tok;
                        tok = strtok_r(NULL, " ", &sv);
                    }
                }
                ca[ai] = NULL;
                if (!vlog_compile_cmd) vlog_compile_cmd = cmdline_from_argv(ca);
                ProcResult cr = proc_run(ca, 30, 0);
                out_buf = strappend(out_buf, &out_len, &out_cap, "[%s]\n", *tn);
                if (cr.exit_code == 0) {
                    if (cr.out_len > 0) {
                        strip_ansi(cr.out);
                        out_buf = strappend(out_buf, &out_len, &out_cap, "%s", cr.out);
                    }
                    if (!is_darwin_cross) {
                        ProcResult rr = run_exe_with_cmdline(tmp_exe, "", 20,
                                                             vlog_run_cmd ? NULL : &vlog_run_cmd);
                        out_buf = strappend(out_buf, &out_len, &out_cap, "%s", rr.out);
                        proc_free(&rr);
                    }
                } else {
                    strip_ansi(cr.out);
                    out_buf = strappend(out_buf, &out_len, &out_cap, "%s", cr.out);
                }
                out_buf = strappend(out_buf, &out_len, &out_cap, "\n");
                proc_free(&cr);
            }
        }
        if (in_cd_dir) {
            if (chdir(SCRIPT_DIR) != 0) perror("chdir");
            src_path = orig_src;
            rcc = orig_rcc;
            in_cd_dir = false;
        }
        if (is_darwin_cross) {
            print_result(base, COL_GREEN, "PASS (compile only)");
            passed++;
            add_row(base, "COMPILE_OK", "linked, (execution skipped)");
            print_change(base, "COMPILE_OK");
            vlog_test_details(base, vlog_compile_cmd, NULL, vlog_run_cmd, out_buf);
            free(vlog_compile_cmd);
            free(vlog_run_cmd);
            free(out_buf);
            return;
        }
        {
            char *er = file_exists(expect_file) ? slurp(expect_file) : NULL;
            if (er) {
                char *en = normalize_output(er, base), *on = normalize_output(out_buf, base);
                if (diff_strings(en, on, base)) {
                    print_result(base, COL_GREEN, "PASS");
                    passed++;
                    add_row(base, "PASS", "Output matches");
                    print_change(base, "PASS");
                } else {
                    print_result(base, COL_RED, "MISMATCH");
                    failed++;
                    add_row(base, "MISMATCH", "Output differs");
                    print_change(base, "MISMATCH");
                    char sp[512], sp_tmp[516];
                    snprintf(sp, sizeof(sp), "%s/test/%s.out", SCRIPT_DIR, base);
                    snprintf(sp_tmp, sizeof(sp_tmp), "%s.tmp", sp);
                    FILE *sf = fopen(sp_tmp, "wb");
                    if (sf) {
                        if (out_buf) fputs(out_buf, sf);
                        fclose(sf);
                        move_file_atomic(sp_tmp, sp);
                    }
                }
                free(on);
                free(en);
                free(er);
            } else {
                print_result(base, COL_GREEN, "PASS (no expect)");
                passed++;
                add_row(base, "PASS", "Output generated (no expect)");
                print_change(base, "PASS");
            }
        }
        vlog_test_details(base, vlog_compile_cmd, NULL, vlog_run_cmd, out_buf);
        free(vlog_compile_cmd);
        free(vlog_run_cmd);
        free(out_buf);
        return;
    }

    /* 128_run_atexit special handling */
    if (streq(base, "128_run_atexit")) {
        const char *tests[] = {"test_128_return", "test_128_exit", NULL};
        int exp_rc[] = {1, 2};
        for (int t = 0; tests[t]; t++) {
            char *ca[8];
            int ai = 0;
            ca[ai++] = (char *)rcc;
            ca[ai++] = (char *)rccflags;
            char df[64];
            snprintf(df, sizeof(df), "-D%s", tests[t]);
            ca[ai++] = df;
            ca[ai++] = "-o";
            ca[ai++] = tmp_exe;
            ca[ai++] = (char *)src_path;
            ca[ai] = NULL;
            if (!vlog_compile_cmd) vlog_compile_cmd = cmdline_from_argv(ca);
            ProcResult cr = proc_run(ca, 30, 0);
            out_buf = strappend(out_buf, &out_len, &out_cap, "[%s]\n", tests[t]);
            if (is_darwin_cross) {
                out_buf = strappend(out_buf, &out_len, &out_cap, "[linked]\n");
            } else {
                ProcResult rr = run_exe_with_cmdline(tmp_exe, "", 10,
                                                     vlog_run_cmd ? NULL : &vlog_run_cmd);
                int rc = rr.exit_code;
                out_buf = strappend(out_buf, &out_len, &out_cap, "%s", rr.out);
                out_buf = strappend(out_buf, &out_len, &out_cap, "[returns %d]\n", rc);
                if (rc != exp_rc[t])
                    emit_backtrace(tmp_exe, "", src_path, rcc, rccflags, NULL, &out_buf,
                                   &out_len, &out_cap);
                proc_free(&rr);
            }
            if (t == 0) out_buf = strappend(out_buf, &out_len, &out_cap, "\n");
            proc_free(&cr);
        }
        if (in_cd_dir) {
            if (chdir(SCRIPT_DIR) != 0) perror("chdir");
            src_path = orig_src;
            rcc = orig_rcc;
            in_cd_dir = false;
        }
        {
            char *er = file_exists(expect_file) ? slurp(expect_file) : NULL;
            if (er) {
                char *en = normalize_output(er, base), *on = normalize_output(out_buf, base);
                if (diff_strings(en, on, base)) {
                    print_result(base, COL_GREEN, "PASS");
                    passed++;
                    add_row(base, "PASS", "Output matches");
                    print_change(base, "PASS");
                } else {
                    print_result(base, COL_RED, "MISMATCH");
                    failed++;
                    add_row(base, "MISMATCH", "Output differs");
                    print_change(base, "MISMATCH");
                }
                free(on);
                free(en);
                free(er);
            }
        }
        vlog_test_details(base, vlog_compile_cmd, NULL, vlog_run_cmd, out_buf);
        free(vlog_compile_cmd);
        free(vlog_run_cmd);
        free(out_buf);
        return;
    }

    /* normal test: compile */
    {
        char *ca[16];
        int ai = 0;
        ca[ai++] = (char *)rcc;
        ca[ai++] = (char *)rccflags;
        ca[ai++] = "-o";
        ca[ai++] = tmp_exe;
        if (p_src && *p_src) {
            char *ps = strdup(p_src);
            char *sv = NULL;
            char *tok = strtok_r(ps, " ", &sv);
            while (tok && ai < 14) {
                ca[ai++] = tok;
                tok = strtok_r(NULL, " ", &sv);
            }
        }
        ca[ai++] = (char *)src_path;
        if (ldflags && *ldflags) {
            char *lf = strdup(ldflags);
            char *sv = NULL;
            char *tok = strtok_r(lf, " ", &sv);
            while (tok && ai < 15) {
                ca[ai++] = tok;
                tok = strtok_r(NULL, " ", &sv);
            }
        }
        ca[ai] = NULL;
        vlog_compile_cmd = cmdline_from_argv(ca);
        ProcResult cr = proc_run(ca, 30, 0);
        if (cr.exit_code != 0) {
            print_result(base, COL_RED, "COMPILE FAIL");
            failed++;
            add_row(base, "COMPILE_FAIL", "rcc returned non-zero");
            print_change(base, "COMPILE_FAIL");
            if (cr.out && cr.out[0]) fprintf(stderr, "%s", cr.out);
            vlog_test_details(base, vlog_compile_cmd, cr.out, NULL, NULL);
            free(vlog_compile_cmd);
            if (in_cd_dir) {
                if (chdir(SCRIPT_DIR) != 0) perror("chdir");
                src_path = orig_src;
                rcc = orig_rcc;
            }
            proc_free(&cr);
            return;
        }
        if (cr.out && cr.out[0]) out_buf = strappend(out_buf, &out_len, &out_cap, "%s", cr.out);
        proc_free(&cr);
    }

    if (in_cd_dir) {
        if (chdir(SCRIPT_DIR) != 0) perror("chdir");
        src_path = orig_src;
        rcc = orig_rcc;
        in_cd_dir = false;
    }

    if (access(tmp_exe, X_OK) != 0) {
        print_result(base, COL_RED, "COMPILE FAIL");
        failed++;
        add_row(base, "COMPILE_FAIL", "executable missing");
        print_change(base, "COMPILE_FAIL");
        vlog_test_details(base, vlog_compile_cmd, NULL, NULL, NULL);
        free(vlog_compile_cmd);
        return;
    }

    if (is_darwin_cross) {
        print_result(base, COL_GREEN, "PASS (compile only)");
        passed++;
        add_row(base, "COMPILE_OK", "linked, (execution skipped)");
        print_change(base, "COMPILE_OK");
        vlog_test_details(base, vlog_compile_cmd, NULL, NULL, NULL);
        free(vlog_compile_cmd);
        return;
    }

    const char *args = test_args(base);
    int expected_exit = test_expected_exit(base);
    int actual_exit;
    bool exec_timed_out = false;

    if (streq(base, "46_grep")) {
        char save_cwd[PATH_MAX];
        if (!getcwd(save_cwd, sizeof(save_cwd)))
            save_cwd[0] = '\0';
        if (chdir(TEST_DIR) != 0) perror("chdir");
        char *ga[8];
        int gai = 0;
        if (has_runner) {
            char *rc = strdup(runner_cmd);
            char *sv = NULL;
            char *tok = strtok_r(rc, " ", &sv);
            while (tok && gai < 6) {
                ga[gai++] = tok;
                tok = strtok_r(NULL, " ", &sv);
            }
        }
        ga[gai++] = (char *)tmp_exe;
        ga[gai++] = "[^* ]*[:a:d: ]+\\:\\*-/: $";
        ga[gai++] = "46_grep.c";
        ga[gai] = NULL;
        vlog_run_cmd = cmdline_from_argv(ga);
        ProcResult rr = proc_run(ga, 20, 2);
        out_buf = strappend(out_buf, &out_len, &out_cap, "%s", rr.out);
        actual_exit = rr.exit_code;
        exec_timed_out = rr.timed_out;
        proc_free(&rr);
        if (save_cwd[0] && chdir(save_cwd) != 0) perror("chdir");
    } else {
        ProcResult rr = run_exe_with_cmdline(tmp_exe, args, args && *args ? 20 : 5,
                                             &vlog_run_cmd);
        out_buf = strappend(out_buf, &out_len, &out_cap, "%s", rr.out);
        actual_exit = rr.exit_code;
        exec_timed_out = rr.timed_out;
        proc_free(&rr);
    }

    if (actual_exit != expected_exit) {
        print_result(base, COL_RED, "EXEC FAIL");
        log_fail_exit(actual_exit, exec_timed_out);
        failed++;
        add_row(base, "EXEC_FAIL", "non-zero exit");
        print_change(base, "EXEC_FAIL");
        emit_backtrace(tmp_exe, args, src_path, rcc, rccflags, NULL, &out_buf, &out_len,
                       &out_cap);
        vlog_test_details(base, vlog_compile_cmd, NULL, vlog_run_cmd, out_buf);
        free(vlog_compile_cmd);
        free(vlog_run_cmd);
        unlink(tmp_exe);
        free(out_buf);
        return;
    }
    unlink(tmp_exe);

    {
        char *er = file_exists(expect_file) ? slurp(expect_file) : NULL;
        if (er) {
            char *en = normalize_output(er, base), *on = normalize_output(out_buf, base);
            if (diff_strings(en, on, base)) {
                print_result(base, COL_GREEN, "PASS");
                passed++;
                add_row(base, "PASS", "Output matches");
                print_change(base, "PASS");
            } else {
                print_result(base, COL_RED, "MISMATCH");
                failed++;
                add_row(base, "MISMATCH", "Output does not match .expect");
                print_change(base, "MISMATCH");
                char sp[512], sp_tmp[516];
                snprintf(sp, sizeof(sp), "%s/test/%s.out", SCRIPT_DIR, base);
                snprintf(sp_tmp, sizeof(sp_tmp), "%s.tmp", sp);
                FILE *sf = fopen(sp_tmp, "wb");
                if (sf) {
                    if (out_buf) fputs(out_buf, sf);
                    fclose(sf);
                    move_file_atomic(sp_tmp, sp);
                }
            }
            free(on);
            free(en);
            free(er);
        } else {
            print_result(base, COL_GREEN, "PASS (no expect)");
            passed++;
            add_row(base, "PASS", "Executed successfully (no .expect)");
            print_change(base, "PASS");
        }
    }
    vlog_test_details(base, vlog_compile_cmd, NULL, vlog_run_cmd, out_buf);
    free(vlog_compile_cmd);
    free(vlog_run_cmd);
    free(out_buf);
}


/* ── parallel: compile + execute, no reporting ───────────────────── */

static void compile_and_exec(const char *src_path, const char *base,
                             const char *p_src, const char *ldflags, bool is_mingw,
                             ParallelResult *r, int index) {
    memset(r, 0, sizeof(*r));
    snprintf(r->tmp_exe, sizeof(r->tmp_exe), "%s/rcc_par_%d", get_tmpdir(), index);
    if (is_mingw || (has_runner && contains(runner_cmd, "wine")))
        strcat(r->tmp_exe, ".exe");

    char expect_file[512], local_expect[512];
    snprintf(local_expect, sizeof(local_expect), "%s/test/tinycc-%s.expect", SCRIPT_DIR, base);
    if (file_exists(local_expect)) snprintf(expect_file, sizeof(expect_file), "%s", local_expect);
    else
        snprintf(expect_file, sizeof(expect_file), "%s/%s.expect", TEST_DIR, base);

    char *out_buf = NULL;
    size_t out_len = 0, out_cap = 0;
    r->did_compile = true;

    /* In-process fast path: compile + run via rcc_lib, skipping the
     * external rcc/exe process spawns entirely.  Always run with
     * cwd=TEST_DIR and a basename source path so __FILE__/relative
     * includes resolve the same way for every test (cd_tests no longer
     * need special-casing here).  DT-tests, 128_run_atexit and 46_grep
     * keep using the external-rcc path below (multiple compiles / custom
     * argv per sub-test). On any failure, fall back to external rcc.
     *
     * Restricted to g_num_workers <= 1 (sequential): run_test_inprocess
     * forks to isolate the compiled test's main(), but fork() from a
     * multi-threaded process only clones the calling thread - any mutex
     * (e.g. glibc's malloc arena lock) held by another worker thread at
     * that instant stays locked forever in the child, deadlocking it if
     * fn() ever needs it. Under --parallel, use the external-rcc path
     * (separate processes, no such hazard) instead. */
    if (use_rcc_lib && g_num_workers <= 1 && !is_dt_test(base) &&
        !streq(base, "128_run_atexit") &&
        !streq(base, "46_grep") && !is_darwin_cross) {
        const char *fn = strrchr(src_path, '/');
        const char *base_src = fn ? fn + 1 : src_path;
        const char *args = test_args(base);
        int expected_exit = test_expected_exit(base);

        char *extra_argv[8];
        int eai = 0;
        char *args_copy = NULL;
        if (args && *args) {
            args_copy = strdup(args);
            char *sv = NULL;
            for (char *tok = strtok_r(args_copy, " ", &sv); tok && eai < 7;
                 tok = strtok_r(NULL, " ", &sv))
                extra_argv[eai++] = tok;
        }
        extra_argv[eai] = NULL;

        ProcResult pr;
        int rc = run_test_inprocess(base_src, r->tmp_exe, NULL, p_src, ldflags,
                                    extra_argv, TEST_DIR, &pr, 30);
        free(args_copy);
        if (rc == 0) {
            r->compile_cmdline = strdup("(in-process compile)");
            r->run_cmdline = strdup("(in-process run)");
            r->did_exec = true;
            r->exec_exit = pr.exit_code;
            r->exec_out = pr.out;
            if (r->exec_exit != expected_exit) {
                size_t len = pr.out_len, cap = pr.out_len + 1;
                r->exec_out = strappend(r->exec_out, &len, &cap,
                                        "\n(in-process execution, no backtrace available)\n");
            }
            return;
        }
    }

    /* cd_tests (e.g. 125_atomic_misc, 129_scopes) need __FILE__ to expand
     * to just the basename. Instead of a process-global chdir() (thread-
     * unsafe), compile with cwd=TEST_DIR via
     * posix_spawn_file_actions_addchdir_np / CreateProcess's
     * lpCurrentDirectory and pass only the basename. */
    const char *compile_src = src_path;
    const char *compile_cwd = NULL;
    if (is_cd_test(base)) {
        const char *fn = strrchr(src_path, '/');
        compile_src = fn ? fn + 1 : src_path;
        compile_cwd = TEST_DIR;
    }

    /* DT tests */
    if (is_dt_test(base)) {
        char **dt = extract_dt_tests(src_path);
        if (dt) {
            for (char **tn = dt; *tn; tn++) {
                char *ca[16];
                int ai = 0;
                ca[ai++] = (char *)rcc;
                ca[ai++] = (char *)rccflags;
                char df[128];
                snprintf(df, sizeof(df), "-D%s", *tn);
                ca[ai++] = df;
                ca[ai++] = "-o";
                ca[ai++] = r->tmp_exe;
                if (p_src && *p_src) {
                    char *ps = strdup(p_src);
                    char *sv = NULL;
                    char *tok = strtok_r(ps, " ", &sv);
                    while (tok && ai < 14) {
                        ca[ai++] = tok;
                        tok = strtok_r(NULL, " ", &sv);
                    }
                }
                ca[ai++] = (char *)compile_src;
                if (ldflags && *ldflags) {
                    char *lf = strdup(ldflags);
                    char *sv = NULL;
                    char *tok = strtok_r(lf, " ", &sv);
                    while (tok && ai < 15) {
                        ca[ai++] = tok;
                        tok = strtok_r(NULL, " ", &sv);
                    }
                }
                ca[ai] = NULL;
                if (!r->compile_cmdline) r->compile_cmdline = cmdline_from_argv(ca);
                ProcResult cr = proc_run_cwd(ca, 30, 0, compile_cwd);
                out_buf = strappend(out_buf, &out_len, &out_cap, "[%s]\n", *tn);
                if (cr.exit_code == 0) {
                    if (cr.out_len > 0) {
                        strip_ansi(cr.out);
                        out_buf = strappend(out_buf, &out_len, &out_cap, "%s", cr.out);
                    }
                    if (!is_darwin_cross) {
                        ProcResult rr = run_exe_with_cmdline(r->tmp_exe, "", 20,
                                                             r->run_cmdline ? NULL : &r->run_cmdline);
                        out_buf = strappend(out_buf, &out_len, &out_cap, "%s", rr.out);
                        proc_free(&rr);
                    }
                } else {
                    r->exit_code = cr.exit_code;
                    strip_ansi(cr.out);
                    out_buf = strappend(out_buf, &out_len, &out_cap, "%s", cr.out);
                }
                out_buf = strappend(out_buf, &out_len, &out_cap, "\n");
                proc_free(&cr);
            }
        }
        if (is_darwin_cross) {
            r->did_exec = false;
        } else {
            r->did_exec = true;
            r->exec_exit = 0;
        }
        r->exec_out = out_buf;
        return;
    }

    /* 128_run_atexit special handling */
    if (streq(base, "128_run_atexit")) {
        const char *tests[] = {"test_128_return", "test_128_exit", NULL};
        int exp_rc[] = {1, 2};
        for (int t = 0; tests[t]; t++) {
            char *ca[8];
            int ai = 0;
            ca[ai++] = (char *)rcc;
            ca[ai++] = (char *)rccflags;
            char df[64];
            snprintf(df, sizeof(df), "-D%s", tests[t]);
            ca[ai++] = df;
            ca[ai++] = "-o";
            ca[ai++] = r->tmp_exe;
            ca[ai++] = (char *)src_path;
            ca[ai] = NULL;
            if (!r->compile_cmdline) r->compile_cmdline = cmdline_from_argv(ca);
            ProcResult cr = proc_run(ca, 30, 0);
            out_buf = strappend(out_buf, &out_len, &out_cap, "[%s]\n", tests[t]);
            if (is_darwin_cross) {
                out_buf = strappend(out_buf, &out_len, &out_cap, "[linked]\n");
            } else {
                ProcResult rr = run_exe_with_cmdline(r->tmp_exe, "", 10,
                                                     r->run_cmdline ? NULL : &r->run_cmdline);
                int rc = rr.exit_code;
                out_buf = strappend(out_buf, &out_len, &out_cap, "%s", rr.out);
                out_buf = strappend(out_buf, &out_len, &out_cap, "[returns %d]\n", rc);
                if (rc != exp_rc[t])
                    emit_backtrace(r->tmp_exe, "", src_path, rcc, rccflags, NULL, &out_buf,
                                   &out_len, &out_cap);
                proc_free(&rr);
            }
            if (t == 0) out_buf = strappend(out_buf, &out_len, &out_cap, "\n");
            proc_free(&cr);
        }
        r->did_exec = !is_darwin_cross;
        r->exec_out = out_buf;
        return;
    }

    /* normal test: compile */
    {
        /* cd_tests (e.g. 129_scopes) need __FILE__ to expand to just the
         * basename. Instead of a process-global chdir() (thread-unsafe),
         * compile with cwd=TEST_DIR via posix_spawn_file_actions_addchdir_np
         * / CreateProcess's lpCurrentDirectory and pass only the basename. */
        const char *compile_src = src_path;
        const char *compile_cwd = NULL;
        if (is_cd_test(base)) {
            const char *fn = strrchr(src_path, '/');
            compile_src = fn ? fn + 1 : src_path;
            compile_cwd = TEST_DIR;
        }

        char *ca[16];
        int ai = 0;
        ca[ai++] = (char *)rcc;
        ca[ai++] = (char *)rccflags;
        ca[ai++] = "-o";
        ca[ai++] = r->tmp_exe;
        if (p_src && *p_src) {
            char *ps = strdup(p_src);
            char *sv = NULL;
            char *tok = strtok_r(ps, " ", &sv);
            while (tok && ai < 14) {
                ca[ai++] = tok;
                tok = strtok_r(NULL, " ", &sv);
            }
        }
        ca[ai++] = (char *)compile_src;
        if (ldflags && *ldflags) {
            char *lf = strdup(ldflags);
            char *sv = NULL;
            char *tok = strtok_r(lf, " ", &sv);
            while (tok && ai < 15) {
                ca[ai++] = tok;
                tok = strtok_r(NULL, " ", &sv);
            }
        }
        ca[ai] = NULL;
        r->compile_cmdline = cmdline_from_argv(ca);
        ProcResult cr = proc_run_cwd(ca, 30, 0, compile_cwd);
        if (cr.exit_code != 0) {
            r->exit_code = cr.exit_code;
            r->compile_out = cr.out;
            cr.out = NULL;
            proc_free(&cr);
            return;
        }
        if (cr.out && cr.out[0]) out_buf = strappend(out_buf, &out_len, &out_cap, "%s", cr.out);
        proc_free(&cr);
    }

    if (access(r->tmp_exe, X_OK) != 0) {
        r->exit_code = -1;
        free(out_buf);
        return;
    }

    if (is_darwin_cross) {
        r->did_exec = false;
        r->exec_out = out_buf;
        return;
    }

    r->did_exec = true;
    const char *args = test_args(base);
    int expected_exit = test_expected_exit(base);

    if (streq(base, "46_grep")) {
        char *ga[8];
        int gai = 0;
        if (has_runner) {
            char *rc = strdup(runner_cmd);
            char *sv = NULL;
            char *tok = strtok_r(rc, " ", &sv);
            while (tok && gai < 6) {
                ga[gai++] = tok;
                tok = strtok_r(NULL, " ", &sv);
            }
        }
        ga[gai++] = (char *)r->tmp_exe;
        ga[gai++] = "[^* ]*[:a:d: ]+\\:\\*-/: $";
        ga[gai++] = "46_grep.c";
        ga[gai] = NULL;
        r->run_cmdline = cmdline_from_argv(ga);
        // Run with cwd=TEST_DIR (per-process, thread-safe) so "46_grep.c"
        // resolves and the output matches the .expect basename.
        ProcResult rr = proc_run_cwd(ga, 20, 2, TEST_DIR);
        out_buf = strappend(out_buf, &out_len, &out_cap, "%s", rr.out);
        r->exec_exit = rr.exit_code;
        r->exec_timed_out = rr.timed_out;
        proc_free(&rr);
    } else {
        ProcResult rr = run_exe_with_cmdline(r->tmp_exe, args, args && *args ? 20 : 5,
                                             &r->run_cmdline);
        out_buf = strappend(out_buf, &out_len, &out_cap, "%s", rr.out);
        r->exec_exit = rr.exit_code;
        r->exec_timed_out = rr.timed_out;
        proc_free(&rr);
    }
    r->exec_out = out_buf;
    if (r->exec_exit != expected_exit) {
        emit_backtrace(r->tmp_exe, args, src_path, rcc, rccflags, NULL, &out_buf, &out_len,
                       &out_cap);
        r->exec_out = out_buf;
    }
    unlink(r->tmp_exe);
}

/* ── parallel: evaluate stored results + report ──────────────────── */

static void evaluate_and_report(const char *base, ParallelResult *r) {
    total++;
    vlog_test_details(base, r->compile_cmdline, r->compile_out,
                      r->run_cmdline, r->exec_out);
    free(r->compile_cmdline);
    r->compile_cmdline = NULL;
    free(r->run_cmdline);
    r->run_cmdline = NULL;
    char expect_file[512], local_expect[512];
    snprintf(local_expect, sizeof(local_expect), "%s/test/tinycc-%s.expect", SCRIPT_DIR, base);
    if (file_exists(local_expect))
        snprintf(expect_file, sizeof(expect_file), "%s", local_expect);
    else
        snprintf(expect_file, sizeof(expect_file), "%s/%s.expect", TEST_DIR, base);

    char *out_buf = r->exec_out;

    /* DT tests evaluation */
    if (is_dt_test(base)) {
        if (is_darwin_cross) {
            print_result(base, COL_GREEN, "PASS (compile only)");
            passed++;
            add_row(base, "COMPILE_OK", "linked, (execution skipped)");
            print_change(base, "COMPILE_OK");
            free(out_buf);
            return;
        }
        {
            char *er = file_exists(expect_file) ? slurp(expect_file) : NULL;
            if (er) {
                char *en = normalize_output(er, base), *on = normalize_output(out_buf, base);
                if (diff_strings(en, on, base)) {
                    print_result(base, COL_GREEN, "PASS");
                    passed++;
                    add_row(base, "PASS", "Output matches");
                    print_change(base, "PASS");
                } else {
                    print_result(base, COL_RED, "MISMATCH");
                    failed++;
                    add_row(base, "MISMATCH", "Output differs");
                    print_change(base, "MISMATCH");
                    char sp[512], sp_tmp[516];
                    snprintf(sp, sizeof(sp), "%s/test/%s.out", SCRIPT_DIR, base);
                    snprintf(sp_tmp, sizeof(sp_tmp), "%s.tmp", sp);
                    FILE *sf = fopen(sp_tmp, "wb");
                    if (sf) {
                        if (out_buf) fputs(out_buf, sf);
                        fclose(sf);
                        move_file_atomic(sp_tmp, sp);
                    }
                }
                free(on);
                free(en);
                free(er);
            } else {
                print_result(base, COL_GREEN, "PASS (no expect)");
                passed++;
                add_row(base, "PASS", "Output generated (no expect)");
                print_change(base, "PASS");
            }
        }
        free(out_buf);
        return;
    }

    /* 128_run_atexit evaluation */
    if (streq(base, "128_run_atexit")) {
        {
            char *er = file_exists(expect_file) ? slurp(expect_file) : NULL;
            if (er) {
                char *en = normalize_output(er, base), *on = normalize_output(out_buf, base);
                if (diff_strings(en, on, base)) {
                    print_result(base, COL_GREEN, "PASS");
                    passed++;
                    add_row(base, "PASS", "Output matches");
                    print_change(base, "PASS");
                } else {
                    print_result(base, COL_RED, "MISMATCH");
                    failed++;
                    add_row(base, "MISMATCH", "Output differs");
                    print_change(base, "MISMATCH");
                }
                free(on);
                free(en);
                free(er);
            }
        }
        free(out_buf);
        return;
    }

    /* normal test evaluation */
    if (r->exit_code != 0) {
        print_result(base, COL_RED, "COMPILE FAIL");
        failed++;
        if (r->exit_code == -1)
            add_row(base, "COMPILE_FAIL", "executable missing");
        else
            add_row(base, "COMPILE_FAIL", "rcc returned non-zero");
        print_change(base, "COMPILE_FAIL");
        if (r->compile_out && r->compile_out[0]) fprintf(stderr, "%s", r->compile_out);
        free(r->compile_out);
        r->compile_out = NULL;
        free(out_buf);
        return;
    }
    free(r->compile_out);
    r->compile_out = NULL;

    if (!r->did_exec) {
        print_result(base, COL_GREEN, "PASS (compile only)");
        passed++;
        add_row(base, "COMPILE_OK", "linked, (execution skipped)");
        print_change(base, "COMPILE_OK");
        free(out_buf);
        return;
    }

    int expected_exit = test_expected_exit(base);
    if (r->exec_exit != expected_exit) {
        print_result(base, COL_RED, "EXEC FAIL");
        log_fail_exit(r->exec_exit, r->exec_timed_out);
        failed++;
        add_row(base, "EXEC_FAIL", "non-zero exit");
        print_change(base, "EXEC_FAIL");
        free(out_buf);
        return;
    }

    {
        char *er = file_exists(expect_file) ? slurp(expect_file) : NULL;
        if (er) {
            char *en = normalize_output(er, base), *on = normalize_output(out_buf, base);
            if (diff_strings(en, on, base)) {
                print_result(base, COL_GREEN, "PASS");
                passed++;
                add_row(base, "PASS", "Output matches");
                print_change(base, "PASS");
            } else {
                print_result(base, COL_RED, "MISMATCH");
                failed++;
                add_row(base, "MISMATCH", "Output does not match .expect");
                print_change(base, "MISMATCH");
                char sp[512], sp_tmp[516];
                snprintf(sp, sizeof(sp), "%s/test/%s.out", SCRIPT_DIR, base);
                snprintf(sp_tmp, sizeof(sp_tmp), "%s.tmp", sp);
                FILE *sf = fopen(sp_tmp, "wb");
                if (sf) {
                    if (out_buf) fputs(out_buf, sf);
                    fclose(sf);
                    move_file_atomic(sp_tmp, sp);
                }
            }
            free(on);
            free(en);
            free(er);
        } else {
            print_result(base, COL_GREEN, "PASS (no expect)");
            passed++;
            add_row(base, "PASS", "Executed successfully (no .expect)");
            print_change(base, "PASS");
        }
    }
    free(out_buf);
}
/* ── unit tests ──────────────────────────────────────────────────── */

/* New C23 feature tests.
 * Report their failures as TODO rather than FAIL. */
static bool is_todo_test(const char *base) {
    static const char *todo_tests[] = {
        "test_bit",
        "test_bool",
        "test_c23_attributes",
        "test_ckdint",
        "test_decimal",
        "test_float",
        "test_nullptr",
        "test_static_assert",
        NULL};
    for (const char **p = todo_tests; *p; p++)
        if (streq(base, *p)) return true;
    return false;
}

/* ── unit: parallel compile+exec ─────────────────────────────────── */

static void unit_compile_exec(const char *src_path, const char *base,
                              ParallelResult *r, int index) {
    memset(r, 0, sizeof(*r));
    snprintf(r->tmp_exe, sizeof(r->tmp_exe), "%s/rcc_par_%d", get_tmpdir(), index);
    if (is_mingw_native || (has_runner && contains(runner_cmd, "wine")))
        strcat(r->tmp_exe, ".exe");

    /* test_err: expect compile failure */
    if (streq(base, "test_err")) {
        char *ca[] = {(char *)rcc, (char *)rccflags, "-o", r->tmp_exe, (char *)src_path, NULL};
        r->compile_cmdline = cmdline_from_argv(ca);
        ProcResult cr = proc_run(ca, 30, 1);
        r->exit_code = cr.exit_code;
        r->compile_out = cr.out;
        cr.out = NULL;
        proc_free(&cr);
        return;
    }

    /* normal compile */
    {
        char *ca[] = {(char *)rcc, (char *)rccflags, "-o", r->tmp_exe, (char *)src_path, NULL};
        r->compile_cmdline = cmdline_from_argv(ca);
        ProcResult cr = proc_run(ca, 30, 1);
        if (cr.exit_code != 0 || access(r->tmp_exe, X_OK) != 0) {
            r->exit_code = cr.exit_code != 0 ? cr.exit_code : -1;
            r->compile_out = cr.out;
            cr.out = NULL;
            proc_free(&cr);
            return;
        }
        proc_free(&cr);
    }
    r->did_compile = true;

    if (is_darwin_cross) {
        r->did_exec = false;
        return;
    }

    ProcResult rr = run_exe_with_cmdline(r->tmp_exe, "", unit_run_timeout(), &r->run_cmdline);
    r->exec_exit = rr.exit_code;
    r->exec_timed_out = rr.timed_out;
    r->exec_out = rr.out;
    rr.out = NULL;
    proc_free(&rr);
    r->did_exec = true;

    /* Capture backtrace on non-zero exit (unit tests expect 0, except a few).
     * emit_backtrace spawns gdb/lldb child processes; safe to call from
     * worker threads as it only touches thread-local/process-local state. */
    if (r->exec_exit != 0 && !is_darwin_cross && !has_runner) {
        size_t bt_len = r->exec_out ? strlen(r->exec_out) : 0;
        size_t bt_cap = bt_len + 4096;
        char *bt_buf = r->exec_out ? xrealloc(r->exec_out, bt_cap) : malloc(bt_cap);
        if (!r->exec_out) { bt_buf[0] = '\0'; }
        r->exec_out = bt_buf;
        emit_backtrace(r->tmp_exe, "", src_path, rcc, rccflags, NULL,
                       &r->exec_out, &bt_len, &bt_cap);
    }
}

/* ── unit: parallel evaluate+report ──────────────────────────────── */

static void unit_evaluate_report(const char *base, ParallelResult *r) {
    total++;
    vlog_test_details(base, r->compile_cmdline, r->compile_out,
                      r->run_cmdline, r->exec_out);
    free(r->compile_cmdline);
    r->compile_cmdline = NULL;
    free(r->run_cmdline);
    r->run_cmdline = NULL;
    /* test_err: expect compile failure */
    if (streq(base, "test_err")) {
        if (r->exit_code == 0) {
            if (is_todo_test(base)) {
                print_result(base, COL_YELLOW, "TODO (should fail)");
                todo++;
                add_row(base, "TODO", "expected compile error but succeeded");
                print_change(base, "TODO");
            } else {
                print_result(base, COL_RED, "SHOULD FAIL");
                failed++;
                add_row(base, "FAIL", "expected compile error but succeeded");
                print_change(base, "FAIL");
            }
        } else {
            print_result(base, COL_GREEN, "PASS (compile error)");
            passed++;
            add_row(base, "PASS", "compile error as expected");
            print_change(base, "PASS");
        }
        free(r->compile_out);
        r->compile_out = NULL;
        unlink(r->tmp_exe);
        return;
    }
    /* compile fail */
    if (!r->did_compile) {
        if (is_todo_test(base)) {
            print_result(base, COL_YELLOW, "TODO (compile)");
            todo++;
            add_row(base, "TODO", r->exit_code != 0 ? "rcc returned non-zero" : "executable missing");
            print_change(base, "TODO");
        } else {
            print_result(base, COL_RED, "COMPILE FAIL");
            failed++;
            add_row(base, "COMPILE_FAIL", r->exit_code != 0 ? "rcc returned non-zero" : "executable missing");
            print_change(base, "COMPILE_FAIL");
        }
        if (r->compile_out && r->compile_out[0])
            fprintf(stderr, "%s", r->compile_out);
        /* Re-run compile for diagnostics */
        if (r->compile_cmdline) {
            ProcResult vr = run_exe_with_cmdline(NULL, r->compile_cmdline, 30, NULL);
            fprintf(stderr, "--- %s COMPILE re-run (exit=%d%s%s) ---\n",
                    base, vr.exit_code,
                    vr.timed_out ? ", timed out" : "",
                    vr.spawn_failed ? ", spawn failed" : "");
            if (vr.out && vr.out[0])
                fprintf(stderr, "%s", vr.out);
            else
                fprintf(stderr, "(no output)\n");
            proc_free(&vr);
        }
        free(r->compile_out);
        r->compile_out = NULL;
        return;
    }
    free(r->compile_out);
    r->compile_out = NULL;

    if (!r->did_exec) {
        print_result(base, COL_GREEN, "PASS (compile only)");
        passed++;
        add_row(base, "COMPILE_OK", "linked, (execution skipped)");
        print_change(base, "COMPILE_OK");
        unlink(r->tmp_exe);
        return;
    }

    int expected_exit = test_unit_expected_exit(base);
    if (r->exec_exit != expected_exit) {
        if (is_todo_test(base)) {
            print_result(base, COL_YELLOW, "TODO (exec)");
            todo++;
            add_row(base, "TODO", "");
        } else {
            print_result(base, COL_RED, "EXEC FAIL");
            log_fail_exit(r->exec_exit, r->exec_timed_out);
            failed++;
            add_row(base, "EXEC_FAIL", "");
        }
        free(report_rows[nrows - 1].message);
        char msg[64];
        snprintf(msg, sizeof(msg), "exit=%d", r->exec_exit);
        report_rows[nrows - 1].message = strdup(msg);
        print_change(base, is_todo_test(base) ? "TODO" : "EXEC_FAIL");
        /* print captured stdout/stderr from the failing test */
        if (r->exec_out && r->exec_out[0])
            fprintf(stderr, "--- %s ---\n%s", base, r->exec_out);
        /* Re-run with VERBOSE env to get internal diagnostics */
        setenv("VERBOSE", "1", 1);
        {
            ProcResult vr = run_exe_with_cmdline(r->tmp_exe, "", unit_run_timeout(), NULL);
            fprintf(stderr, "--- %s VERBOSE re-run (exit=%d%s%s) ---\n",
                    base, vr.exit_code,
                    vr.timed_out ? ", timed out" : "",
                    vr.spawn_failed ? ", spawn failed" : "");
            if (vr.out && vr.out[0])
                fprintf(stderr, "%s", vr.out);
            else
                fprintf(stderr, "(no output)\n");
            proc_free(&vr);
        }
#ifdef _WIN32
        SetEnvironmentVariableA("VERBOSE", NULL);
#else
        unsetenv("VERBOSE");
#endif
    } else {
        print_result(base, COL_GREEN, "PASS");
        passed++;
        add_row(base, "PASS", "");
        print_change(base, "PASS");
    }
    free(r->exec_out);
    r->exec_out = NULL;
    unlink(r->tmp_exe);
}

static int run_unit_tests(void) {
    static char unit_path[512];
    snprintf(unit_path, sizeof(unit_path), "%s/test", SCRIPT_DIR);
    if (!file_exists(unit_path)) return 0;
    printf("\n%sUnit tests (test/)%s\n", COL_CYAN, COL_RESET);

    total = passed = failed = todo = 0;

    struct dirent **nl;
    int n = scandir(unit_path, &nl, NULL, alphasort);
    if (n < 0) return 0;

    if (g_num_workers > 1 && !only_test) {
        /* Parallel path: collect, dispatch, evaluate */
        int n_tests = 0;
        struct {
            const char *fname;
            char base[256];
        } *entries = NULL;
        int n_alloc = 0;
        for (int i = 0; i < n; i++) {
            const char *fname = nl[i]->d_name;
            size_t flen = strlen(fname);
            if (flen < 7 || strncmp(fname, "test_", 5) != 0 || strcmp(fname + flen - 2, ".c") != 0)
                continue;
            char base[256];
            strncpy(base, fname, sizeof(base) - 1);
            base[sizeof(base) - 1] = '\0';
            char *dot = strrchr(base, '.');
            if (dot) *dot = '\0';
            /* skip arm64-only tests on non-arm64 */
            if (streq(base, "test_arm64_asm") && !is_arm64) {
                print_result(base, COL_YELLOW, "SKIP");
                add_row(base, "SKIP", "Skipped");
                continue;
            }
            if (n_tests >= n_alloc) {
                n_alloc = n_alloc ? n_alloc * 2 : 16;
                entries = xrealloc(entries, (size_t)n_alloc * sizeof(*entries));
            }
            entries[n_tests].fname = nl[i]->d_name;
            strncpy(entries[n_tests].base, base, sizeof(entries[n_tests].base) - 1);
            entries[n_tests].base[sizeof(entries[n_tests].base) - 1] = '\0';
            n_tests++;
        }
        if (n_tests > 0) {
            ParallelJob *jobs = calloc((size_t)n_tests, sizeof(ParallelJob));
            for (int i = 0; i < n_tests; i++) {
                char *sp = malloc(PATH_MAX);
                snprintf(sp, PATH_MAX, "%s/%s", unit_path, entries[i].fname);
                jobs[i].suite = SUITE_UNIT;
                jobs[i].src_path = sp;
                jobs[i].base = strdup(entries[i].base);
                jobs[i].index = i;
            }
            parallel_dispatch(jobs, n_tests);
            for (int i = 0; i < n_tests; i++) {
                unit_evaluate_report(jobs[i].base, &jobs[i].result);
                free((void *)jobs[i].src_path);
                free((void *)jobs[i].base);
            }
            free(jobs);
        }
        free(entries);
        for (int i = 0; i < n; i++) free(nl[i]);
        free(nl);
    } else {
        /* Sequential path (original) */
        for (int i = 0; i < n; i++) {
            const char *fname = nl[i]->d_name;
            size_t flen = strlen(fname);
            if (flen < 7 || strncmp(fname, "test_", 5) != 0 || strcmp(fname + flen - 2, ".c") != 0) {
                free(nl[i]);
                continue;
            }

            char base[256];
            strncpy(base, fname, sizeof(base) - 1);
            base[sizeof(base) - 1] = '\0';
            char *dot = strrchr(base, '.');
            if (dot) *dot = '\0';

            if (only_test) {
                if (!streq(base, only_test)) {
                    free(nl[i]);
                    continue;
                }
                only_test_found = true;
            }

            /* skip arm64-only tests on non-arm64 */
            if (streq(base, "test_arm64_asm") && !is_arm64) {
                print_result(base, COL_YELLOW, "SKIP");
                add_row(base, "SKIP", "Skipped");
                free(nl[i]);
                continue;
            }

            total++;
            char src_path[PATH_MAX];
            snprintf(src_path, sizeof(src_path), "%s/%s", unit_path, fname);

            char *compile_cmdline = NULL;
            char *run_cmdline = NULL;
            char *run_out = NULL;

            /* test_err: expect compile failure */
            if (streq(base, "test_err")) {
                char tmp[2 * PATH_MAX];
                snprintf(tmp, sizeof(tmp), "%s/rcc_test_%d", get_tmpdir(), getpid());
                if (is_mingw_native || (has_runner && contains(runner_cmd, "wine")))
                    strcat(tmp, ".exe");
                char *ca[] = {(char *)rcc, (char *)rccflags, "-o", tmp, src_path, NULL};
                compile_cmdline = cmdline_from_argv(ca);
                ProcResult cr = proc_run(ca, 30, 1);
                if (cr.exit_code == 0) {
                    if (is_todo_test(base)) {
                        print_result(base, COL_YELLOW, "TODO (should fail)");
                        todo++;
                        add_row(base, "TODO", "expected compile error but succeeded");
                        print_change(base, "TODO");
                    } else {
                        print_result(base, COL_RED, "SHOULD FAIL");
                        failed++;
                        add_row(base, "FAIL", "expected compile error but succeeded");
                        print_change(base, "FAIL");
                    }
                    unlink(tmp);
                } else {
                    print_result(base, COL_GREEN, "PASS (compile error)");
                    passed++;
                    add_row(base, "PASS", "compile error as expected");
                    print_change(base, "PASS");
                }
                vlog_test_details(base, compile_cmdline, cr.out, NULL, NULL);
                free(compile_cmdline);
                proc_free(&cr);
                free(nl[i]);
                continue;
            }

            int expected_exit = test_unit_expected_exit(base);
            char tmp[2 * PATH_MAX];
            snprintf(tmp, sizeof(tmp), "%s/rcc_test_%d", get_tmpdir(), getpid());
            if (is_mingw_native || (has_runner && contains(runner_cmd, "wine")))
                strcat(tmp, ".exe");
            {
                char *ca[] = {(char *)rcc, (char *)rccflags, "-o", tmp, src_path, NULL};
                compile_cmdline = cmdline_from_argv(ca);
                ProcResult cr = proc_run(ca, 30, 1);
                if (cr.exit_code != 0 || access(tmp, X_OK) != 0) {
                    if (is_todo_test(base)) {
                        print_result(base, COL_YELLOW, "TODO (compile)");
                        todo++;
                        add_row(base, "TODO", cr.exit_code != 0 ? "rcc returned non-zero" : "executable missing");
                        print_change(base, "TODO");
                    } else {
                        print_result(base, COL_RED, "COMPILE FAIL");
                        failed++;
                        add_row(base, "COMPILE_FAIL", cr.exit_code != 0 ? "rcc returned non-zero" : "executable missing");
                        print_change(base, "COMPILE_FAIL");
                    }
                    if (cr.out && cr.out[0])
                        fprintf(stderr, "%s", cr.out);
                    vlog_test_details(base, compile_cmdline, cr.out, NULL, NULL);
                    /* Re-run compile for diagnostics */
                    {
                        ProcResult vr = run_exe_with_cmdline(NULL, compile_cmdline, 30, NULL);
                        fprintf(stderr, "--- %s COMPILE re-run (exit=%d%s%s) ---\n",
                                base, vr.exit_code,
                                vr.timed_out ? ", timed out" : "",
                                vr.spawn_failed ? ", spawn failed" : "");
                        if (vr.out && vr.out[0])
                            fprintf(stderr, "%s", vr.out);
                        else
                            fprintf(stderr, "(no output)\n");
                        proc_free(&vr);
                    }
                    free(compile_cmdline);
                    proc_free(&cr);
                    free(nl[i]);
                    continue;
                }
                proc_free(&cr);
            }

            if (is_darwin_cross) {
                print_result(base, COL_GREEN, "PASS (compile only)");
                passed++;
                add_row(base, "COMPILE_OK", "linked, (execution skipped)");
                print_change(base, "COMPILE_OK");
                vlog_test_details(base, compile_cmdline, NULL, NULL, NULL);
                free(compile_cmdline);
                unlink(tmp);
                free(nl[i]);
                continue;
            }

            ProcResult rr = run_exe_with_cmdline(tmp, "", unit_run_timeout(), &run_cmdline);
            int ae = rr.exit_code;
            run_out = rr.out;
            if (ae != expected_exit) {
                if (is_todo_test(base)) {
                    print_result(base, COL_YELLOW, "TODO (exec)");
                    todo++;
                    add_row(base, "TODO", "");
                } else {
                    print_result(base, COL_RED, "EXEC FAIL");
                    log_fail_exit(ae, rr.timed_out);
                    failed++;
                    add_row(base, "EXEC_FAIL", "");
                }
                free(report_rows[nrows - 1].message);
                char msg[64];
                snprintf(msg, sizeof(msg), "exit=%d", ae);
                report_rows[nrows - 1].message = strdup(msg);
                print_change(base, is_todo_test(base) ? "TODO" : "EXEC_FAIL");
                /* Re-run with VERBOSE env to get internal diagnostics */
                setenv("VERBOSE", "1", 1);
                {
                    ProcResult vr = run_exe_with_cmdline(tmp, "", unit_run_timeout(), NULL);
                    fprintf(stderr, "--- %s VERBOSE re-run (exit=%d%s%s) ---\n",
                            base, vr.exit_code,
                            vr.timed_out ? ", timed out" : "",
                            vr.spawn_failed ? ", spawn failed" : "");
                    if (vr.out && vr.out[0])
                        fprintf(stderr, "%s", vr.out);
                    else
                        fprintf(stderr, "(no output)\n");
                    proc_free(&vr);
                }
#ifdef _WIN32
                SetEnvironmentVariableA("VERBOSE", NULL);
#else
                unsetenv("VERBOSE");
#endif
                /* Emit debugger backtrace */
                if (!has_runner)
                    emit_backtrace(tmp, "", src_path, rcc, rccflags, NULL, NULL, NULL, NULL);
            } else {
                print_result(base, COL_GREEN, "PASS");
                passed++;
                add_row(base, "PASS", "");
                print_change(base, "PASS");
            }
            unlink(tmp);
            vlog_test_details(base, compile_cmdline, NULL, run_cmdline, run_out);
            free(compile_cmdline);
            free(run_cmdline);
            proc_free(&rr);
            free(nl[i]);
        }
        free(nl);
    }

    if (!only_test) {
        int pct = total > 0 ? passed * 100 / total : 0;
        const char *red = failed > 0 ? COL_RED : "";
        const char *rst = failed > 0 ? COL_RESET : "";
        if (todo > 0)
            printf("\nUnit tests: %d/%d passed (%d%%), %s%d failed%s, %d todo.\n",
                   passed, total, pct, red, failed, rst, todo);
        else
            printf("\nUnit tests: %d/%d passed (%d%%), %s%d failed%s.\n",
                   passed, total, pct, red, failed, rst);

        char sp[256], sc[256];
        snprintf(sp, sizeof(sp), "test-units-%s.summary", platform_suffix);
        if (todo > 0)
            snprintf(sc, sizeof(sc), "SUITE=units\nTOTAL=%d\nPASS=%d\nFAIL=%d\nTODO=%d\n",
                     total, passed, failed, todo);
        else
            snprintf(sc, sizeof(sc), "SUITE=units\nTOTAL=%d\nPASS=%d\nFAIL=%d\n",
                     total, passed, failed);
        write_summary(sp, sc);
    }
    return failed > 0 ? 1 : 0;
}

/* ── markdown report ─────────────────────────────────────────────── */

/* atomically replace dst with src; rename is atomic on POSIX within
 * the same filesystem, and MOVEFILE_REPLACE_EXISTING is the closest
 * Windows equivalent. */
static int move_file_atomic(const char *src, const char *dst) {
#ifdef _WIN32
    return MoveFileExA(src, dst, MOVEFILE_REPLACE_EXISTING) ? 0 : -1;
#else
    return rename(src, dst);
#endif
}

static void generate_tcc_report(void) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", SCRIPT_DIR, REPORT_FILE);
    char tmp_path[520];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);
    FILE *rf = fopen(tmp_path, "wb");
    if (!rf) return;

    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char date_buf[64];
    strftime(date_buf, sizeof(date_buf), "%B %Y", tm);
    int pct = total > 0 ? passed * 100 / total : 0;

    fprintf(rf, "# TCC Test Suite Report for RCC\n\nGenerated: %s\n\n", date_buf);
    if (compiler_name)
        fprintf(rf, "**Compiler**: %s\n\n", rcc);
    fprintf(rf, "## Summary\n\n- **Total**: %d\n- **Passed**: %d\n"
                "- **Failed**: %d\n- **Pass Rate**: %d%%\n\n",
            total, passed, failed, pct);
    fprintf(rf, "## Detailed Results\n\n");

    int nm = 4, sm = 6, mm = 7;
    for (int i = 0; i < nrows; i++) {
        int nl = (int)strlen(report_rows[i].name);
        int sl = (int)strlen(report_rows[i].status);
        int ml = (int)strlen(report_rows[i].message);
        if (nl > nm) nm = nl;
        if (sl > sm) sm = sl;
        if (ml > mm) mm = ml;
    }

    fprintf(rf, "| %-*s | %-*s | %-*s |\n", nm, "Test", sm, "Status", mm, "Message");
    fprintf(rf, "| ");
    for (int j = 0; j < nm; j++) fputc('-', rf);
    fprintf(rf, " | ");
    for (int j = 0; j < sm; j++) fputc('-', rf);
    fprintf(rf, " | ");
    for (int j = 0; j < mm; j++) fputc('-', rf);
    fprintf(rf, " |\n");
    for (int i = 0; i < nrows; i++)
        fprintf(rf, "| %-*s | %-*s | %-*s |\n",
                nm, report_rows[i].name, sm, report_rows[i].status, mm, report_rows[i].message);
    fclose(rf);
    move_file_atomic(tmp_path, path);
    printf("Report saved to %s\n", REPORT_FILE);
}

/* ── TCC suite entry point ───────────────────────────────────────── */

static int run_tcc_suite(void) {
    /* reset state */
    total = passed = failed = regressions = fixes = changed_cnt = 0;
    nrows = rows_cap = nold = 0;
    free(report_rows);
    report_rows = NULL;
    free(old_states);
    old_states = NULL;

    if (!file_exists("tinycc")) {
        if (system("git submodule update --init --recursive tinycc 2>/dev/null") != 0)
            fprintf(stderr, "warning: failed to fetch tinycc submodule\n");
    }
#ifndef _WIN32
    if (!file_exists("tcc_tests") && file_exists("tinycc/tests/tests2")) {
        if (symlink("tinycc/tests/tests2", "tcc_tests") != 0)
            perror("symlink");
    }
#endif
    if (!file_exists("tinycc/tests/tests2")) {
        fprintf(stderr, "TCC test directory not found: tinycc/tests/tests2\n");
        return 1;
    }
    /* Absolute path: under --parallel, the in-process compile path
     * temporarily chdir()s the whole process to TEST_DIR (process-global)
     * while holding inproc_lock; other worker threads may concurrently
     * fork() external rcc with a TEST_DIR-relative src_path/expect_file,
     * inheriting the wrong cwd and failing with "No such file or
     * directory".  Making TEST_DIR absolute makes all such paths
     * independent of the process's current working directory. */
    {
        static char test_dir_buf[PATH_MAX];
        snprintf(test_dir_buf, sizeof(test_dir_buf), "%s/tinycc/tests/tests2", SCRIPT_DIR);
        TEST_DIR = test_dir_buf;
    }

    static char rf_buf[256];
    snprintf(rf_buf, sizeof(rf_buf), "test/tcc_test_%s.md", platform_suffix);
    REPORT_FILE = rf_buf;
    bool is_mingw = is_mingw_native || streq(platform, "mingw_cross");
    load_old_states(REPORT_FILE);

    printf("%sTCC compatibility tests%s\n", COL_CYAN, COL_RESET);

    char **files = list_c_files_sorted(TEST_DIR);
    const char *p_src = NULL;
    if (files) {
        if (g_num_workers > 1 && !only_test) {
            /* Parallel path: collect, dispatch, evaluate */
            /* First pass: count tests */
            int n_tests = 0, n_cd = 0;
            const char *scan_src = NULL;
            for (char **f = files; *f; f++) {
                const char *fn = strrchr(*f, '/');
                if (!fn) fn = *f;
                else
                    fn++;
                char sbase[256];
                strncpy(sbase, fn, sizeof(sbase) - 1);
                sbase[sizeof(sbase) - 1] = '\0';
                char *dot = strrchr(sbase, '.');
                if (dot) *dot = '\0';

                if (contains(fn, "+")) {
                    if (scan_src) free((void *)scan_src);
                    scan_src = strdup(*f);
                    continue;
                }
                if (streq(fn, "95_bitfields_ms.c")) {
                    if (is_mingw) scan_src = "-mms-bitfields";
                } else if (streq(fn, "95_bitfields.c")) {
                    if (is_mingw) scan_src = "-mno-ms-bitfields";
                }
                if (is_skipped(sbase, is_mingw)) {
                    print_result(sbase, COL_YELLOW, "SKIP");
                    add_row(sbase, "SKIP", "Skipped");
                    scan_src = NULL;
                    continue;
                }
                // cd_tests/dt_tests and 46_grep run sequentially afterwards
                if (is_cd_test(sbase) || streq(sbase, "46_grep")) {
                    n_cd++;
                    continue;
                }
                n_tests++;
            }
            if (scan_src) free((void *)scan_src);

            if (n_tests > 0 || n_cd > 0) {
                /* Collect jobs: the main pool, and a separate list of
                 * cd_tests/dt_tests + 46_grep (which need a per-process
                 * cwd for __FILE__ or do multiple compile+exec cycles per
                 * test) run sequentially afterwards. */
                ParallelJob *jobs = calloc((size_t)(n_tests ? n_tests : 1), sizeof(ParallelJob));
                ParallelJob *cd_jobs = calloc((size_t)(n_cd ? n_cd : 1), sizeof(ParallelJob));
                int idx = 0, cd_idx = 0;
                scan_src = NULL;
                for (char **f = files; *f; f++) {
                    const char *fn = strrchr(*f, '/');
                    if (!fn) fn = *f;
                    else
                        fn++;
                    char sbase[256];
                    strncpy(sbase, fn, sizeof(sbase) - 1);
                    sbase[sizeof(sbase) - 1] = '\0';
                    char *dot = strrchr(sbase, '.');
                    if (dot) *dot = '\0';

                    if (contains(fn, "+")) {
                        if (scan_src) free((void *)scan_src);
                        scan_src = strdup(*f);
                        continue;
                    }
                    if (streq(fn, "95_bitfields_ms.c")) {
                        if (is_mingw) scan_src = "-mms-bitfields";
                    } else if (streq(fn, "95_bitfields.c")) {
                        if (is_mingw) scan_src = "-mno-ms-bitfields";
                    }
                    if (is_skipped(sbase, is_mingw)) {
                        scan_src = NULL;
                        continue;
                    }

                    ParallelJob *jp;
                    int *pidx;
                    if (is_cd_test(sbase) || streq(sbase, "46_grep")) {
                        jp = cd_jobs;
                        pidx = &cd_idx;
                    } else {
                        jp = jobs;
                        pidx = &idx;
                    }

                    jp[*pidx].suite = SUITE_TCC;
                    jp[*pidx].src_path = *f;
                    jp[*pidx].base = strdup(sbase);
                    jp[*pidx].p_src = scan_src ? scan_src : "";
                    jp[*pidx].ldflags = extra_ldflags(sbase, *f);
                    jp[*pidx].is_mingw = is_mingw;
                    jp[*pidx].index = *pidx;
                    (*pidx)++;
                    scan_src = NULL;
                }
                if (scan_src) free((void *)scan_src);

                /* Main pool: parallel compile + execute, then evaluate + report */
                if (n_tests > 0) {
                    parallel_dispatch(jobs, n_tests);
                    for (int i = 0; i < n_tests; i++) {
                        evaluate_and_report(jobs[i].base, &jobs[i].result);
                        free((void *)jobs[i].base);
                    }
                }
                free(jobs);

                /* cd_tests/dt_tests + 46_grep: run sequentially (one at a
                 * time, no thread pool) */
                for (int i = 0; i < n_cd; i++) {
                    worker_compile_exec(&cd_jobs[i]);
                    evaluate_and_report(cd_jobs[i].base, &cd_jobs[i].result);
                    free((void *)cd_jobs[i].base);
                }
                free(cd_jobs);
            } else if (n_tests == 1) {
                /* Only one test, run sequentially */
                scan_src = NULL;
                for (char **f = files; *f; f++) {
                    const char *fn = strrchr(*f, '/');
                    if (!fn) fn = *f;
                    else
                        fn++;
                    char sbase[256];
                    strncpy(sbase, fn, sizeof(sbase) - 1);
                    sbase[sizeof(sbase) - 1] = '\0';
                    char *dot = strrchr(sbase, '.');
                    if (dot) *dot = '\0';

                    if (contains(fn, "+")) {
                        if (scan_src) free((void *)scan_src);
                        scan_src = strdup(*f);
                        continue;
                    }
                    if (streq(fn, "95_bitfields_ms.c")) {
                        if (is_mingw) scan_src = "-mms-bitfields";
                    } else if (streq(fn, "95_bitfields.c")) {
                        if (is_mingw) scan_src = "-mno-ms-bitfields";
                    }
                    if (is_skipped(sbase, is_mingw)) continue;

                    run_one_test(*f, sbase, scan_src ? scan_src : "", extra_ldflags(sbase, *f), is_mingw);
                    break;
                }
                if (scan_src) free((void *)scan_src);
            }
        } else {
            /* Sequential path (original) */
            p_src = NULL;
            for (char **f = files; *f; f++) {
                const char *fname = strrchr(*f, '/');
                if (!fname) fname = *f;
                else
                    fname++;
                char base[256];
                strncpy(base, fname, sizeof(base) - 1);
                base[sizeof(base) - 1] = '\0';
                char *dot = strrchr(base, '.');
                if (dot) *dot = '\0';

                if (contains(fname, "+")) {
                    if (p_src) free((void *)p_src);
                    p_src = strdup(*f);
                    continue;
                }
                if (streq(fname, "95_bitfields_ms.c")) {
                    if (is_mingw) p_src = "-mms-bitfields";
                } else if (streq(fname, "95_bitfields.c")) {
                    if (is_mingw) p_src = "-mno-ms-bitfields";
                }

                if (only_test) {
                    if (!streq(base, only_test)) {
                        p_src = NULL;
                        continue;
                    }
                    only_test_found = true;
                }

                if (is_skipped(base, is_mingw)) {
                    print_result(base, COL_YELLOW, "SKIP");
                    add_row(base, "SKIP", "Skipped");
                    p_src = NULL;
                    continue;
                }
                run_one_test(*f, base, p_src ? p_src : "", extra_ldflags(base, *f), is_mingw);
                p_src = NULL;
            }
        }
        for (char **f = files; *f; f++) free(*f);
        free(files);
    }
    if (p_src) free((void *)p_src);

    if (!only_test) {
        char sp[256], sc[256];
        snprintf(sp, sizeof(sp), "test-tcc-%s.summary", platform_suffix);
        snprintf(sc, sizeof(sc), "SUITE=tcc\nTOTAL=%d\nPASS=%d\nFAIL=%d\n",
                 total, passed, failed);
        write_summary(sp, sc);
    }

    if (!only_test) {
        int pct = total > 0 ? passed * 100 / total : 0;
        const char *red = failed > 0 ? COL_RED : "";
        const char *rst = failed > 0 ? COL_RESET : "";
        printf("\nTCC: %d/%d passed (%d%%), %s%d failed%s.\n",
               passed, total, pct, red, failed, rst);
        if (regressions + fixes + changed_cnt > 0) {
            printf("Changes vs previous:");
            if (regressions) printf("  %s%d regression(s)%s", COL_RED, regressions, COL_RESET);
            if (fixes) printf("  %s%d fixed%s", COL_GREEN, fixes, COL_RESET);
            if (changed_cnt) printf("  %s%d changed%s", COL_YELLOW, changed_cnt, COL_RESET);
            printf("\n");
        }
        generate_tcc_report();
    }
    return failed > 0 ? 1 : 0;
}

/* ═══════════════════════════════════════════════════════════════════
 * TORTURE TEST SUITE
 * ═══════════════════════════════════════════════════════════════════ */

typedef enum {
    SKIP_NONE,
    SKIP_X86_ONLY,
    SKIP_TMPNAM,
    //SKIP_COMPLEX,
    SKIP_TRAMPOLINES,
    SKIP_SCALAR_STORAGE,
    SKIP_FINSTRUMENT,
    SKIP_NESTED,
    SKIP_NOT_IMPL,
    SKIP_VECTOR_SIZE,
    SKIP_MODE,
    SKIP_MISSING_INCLUDE,
} SkipReason;

static const char *skip_reason_str(SkipReason r) {
    switch (r) {
    case SKIP_X86_ONLY: return "x86-only";
    case SKIP_TMPNAM: return "tmpnam-macOS";
    //case SKIP_COMPLEX: return "complex";
    case SKIP_TRAMPOLINES: return "trampolines";
    case SKIP_SCALAR_STORAGE: return "scalar_storage_order";
    case SKIP_FINSTRUMENT: return "finstrument";
    case SKIP_NESTED: return "nested-func";
    case SKIP_NOT_IMPL: return "not-implemented";
    case SKIP_VECTOR_SIZE: return "vector_size";
    case SKIP_MODE: return "attribute-mode";
    case SKIP_MISSING_INCLUDE: return "missing-include";
    default: return "unknown";
    }
}

static SkipReason torture_should_skip(const char *name, const char *content) {
    if (!content) return SKIP_NONE;
    if (contains(content, "dg-skip-if") &&
        (contains(content, "i?86") || contains(content, "x86_64") || contains(content, "i386")))
        return SKIP_X86_ONLY;
    if (streq(platform, "arm64") && contains(content, "#include \"gcc_tmpnam.h\""))
        return SKIP_TMPNAM;
    //if (contains(content, "__complex__") || contains(content, "Complex"))
    //    return SKIP_COMPLEX;
    if (contains(content, "dg-require-effective-target trampolines"))
        return SKIP_TRAMPOLINES;
    if (contains(content, "scalar_storage_order")) return SKIP_SCALAR_STORAGE;
    if (contains(content, "dg-options") && contains(content, "-finstrument-functions"))
        return SKIP_FINSTRUMENT;
    if (contains(content, "dg-require-effective-target nested") ||
        streq(name, "20061220-1") || streq(name, "nest-align-1") || streq(name, "920415-1") ||
        streq(name, "pr22061-3") || streq(name, "pr22061-4") || streq(name, "pr51447") || streq(name, "pr71494"))
        return SKIP_NESTED;
    if (streq(name, "pr70460") || streq(name, "pr41935"))
        return SKIP_NOT_IMPL;
    if (contains(content, "vector_size") || streq(name, "pr71626-2"))
        return SKIP_VECTOR_SIZE;
    if (contains(content, "__attribute__((mode"))
        return SKIP_MODE;
    return SKIP_NONE;
}

static int g_tort_pass, g_tort_fail_compile, g_tort_fail_runtime, g_tort_skip, g_tort_total;
static char *g_tort_compile_errors, *g_tort_runtime_errors;

static void tort_add_error(char **list, const char *name) {
    if (!*list) {
        *list = strdup(name);
        return;
    }
    size_t need = strlen(*list) + 1 + strlen(name) + 1;
    *list = xrealloc(*list, need);
    strcat(*list, " ");
    strcat(*list, name);
}

/* ── torture: parallel compile+exec ──────────────────────────────── */

static char g_tort_dir[PATH_MAX];

static void tort_compile_exec(const char *src_path, const char *name, bool summary_only,
                              ParallelResult *r, int index) {
    (void)summary_only;
    memset(r, 0, sizeof(*r));
    snprintf(r->tmp_exe, sizeof(r->tmp_exe), "%s/rcc_par_%d", get_tmpdir(), index);
    if (is_mingw_native || (has_runner && contains(runner_cmd, "wine")))
        strcat(r->tmp_exe, ".exe");

    char *content = slurp(src_path);
    SkipReason sk = torture_should_skip(name, content);
    if (sk != SKIP_NONE) {
        r->exit_code = -(int)sk - 1;
        free(content);
        return;
    }

#ifdef _WIN32
    /* In-process compilation is fine on Windows, but in-process execution
     * is NOT: many torture tests call exit(0) which kills run_tests.exe
     * (no fork() on Windows).  Fall through to the external process path
     * which compiles to a temp exe and runs it as a separate process. */
    (void)use_rcc_lib; /* in-process compile still available for non-torture */
#endif

    /* compile */
    {
        char *ca[16];
        int ai = 0;
        ca[ai++] = (char *)rcc;
        ca[ai++] = (char *)rccflags;
        ca[ai++] = "-I";
        ca[ai++] = g_tort_dir;
        ca[ai++] = "-o";
        ca[ai++] = r->tmp_exe;
        ca[ai++] = (char *)src_path;
        ca[ai++] = "-lm";
        ca[ai] = NULL;
        r->compile_cmdline = cmdline_from_argv(ca);
        ProcResult cr = proc_run(ca, 30, 0);

        if (cr.exit_code != 0) {
            if (contains(cr.out, "No such file") || contains(cr.out, "cannot open") ||
                contains(cr.out, "include file") || contains(cr.out, "not found")) {
                r->exit_code = -100;
                r->compile_out = cr.out;
                cr.out = NULL;
            } else {
                r->exit_code = cr.exit_code;
                r->compile_out = cr.out;
                cr.out = NULL;
            }
            proc_free(&cr);
            free(content);
            return;
        }
        proc_free(&cr);
    }
    r->did_compile = true;

    if (streq(platform, "darwin_cross")) {
        r->did_exec = false;
        free(content);
        return;
    }

    /* execute */
    {
        char *ra[32];
        int ri = 0;
        if (has_runner) {
            char *rc = strdup(runner_cmd);
            char *sv = NULL;
            char *tok = strtok_r(rc, " ", &sv);
            while (tok) {
                ra[ri++] = tok;
                tok = strtok_r(NULL, " ", &sv);
            }
        }
        ra[ri++] = r->tmp_exe;
        ra[ri] = NULL;
        r->run_cmdline = cmdline_from_argv(ra);
        ProcResult rr = proc_run(ra, has_runner ? 20 : 5, 0);
        r->exec_exit = rr.exit_code;
        r->exec_timed_out = rr.timed_out;
        if (rr.exit_code != 0 && !has_runner) {
            // emit_backtrace calls gdb via popen/fork, which is unsafe in
            // multi-threaded workers. Capture the exit output directly.
            r->exec_out = rr.out;
            rr.out = NULL;
        } else {
            r->exec_out = rr.out;
            rr.out = NULL;
        }
        proc_free(&rr);
        r->did_exec = true;
    }
    free(content);
}

/* ── torture: parallel evaluate+report ────────────────────────────── */

static void tort_evaluate_report(const char *name, ParallelResult *r, bool summary_only) {
    g_tort_total++;
    if (!summary_only)
        vlog_test_details(name, r->compile_cmdline, r->compile_out,
                          r->run_cmdline, r->exec_out);
    free(r->compile_cmdline);
    r->compile_cmdline = NULL;
    free(r->run_cmdline);
    r->run_cmdline = NULL;
    if (r->exit_code < 0 && r->exit_code > -100) {
        g_tort_skip++;
        if (!summary_only) {
            char sr[64];
            SkipReason sk = (SkipReason)(-r->exit_code - 1);
            snprintf(sr, sizeof(sr), "SKIP (%s)", skip_reason_str(sk));
            print_result(name, COL_YELLOW, sr);
        }
        return;
    }
    if (r->exit_code == -100) {
        g_tort_skip++;
        if (!summary_only)
            print_result(name, COL_YELLOW, "SKIP (missing include)");
        free(r->compile_out);
        r->compile_out = NULL;
        return;
    }
    if (r->exit_code != 0 && !r->did_compile) {
        g_tort_fail_compile++;
        tort_add_error(&g_tort_compile_errors, name);
        if (!summary_only) {
            print_result(name, COL_RED, "FAIL (compile)");
            if (r->compile_out && r->compile_out[0])
                fprintf(stderr, "%s", r->compile_out);
        }
        free(r->compile_out);
        r->compile_out = NULL;
        return;
    }
    free(r->compile_out);
    r->compile_out = NULL;

    if (!r->did_exec) {
        g_tort_pass++;
        if (!summary_only) print_result(name, COL_GREEN, "PASS (compile only)");
        unlink(r->tmp_exe);
        return;
    }

    if (r->exec_exit != 0) {
        g_tort_fail_runtime++;
        tort_add_error(&g_tort_runtime_errors, name);
        if (!summary_only) {
            print_result(name, COL_RED, "FAIL (runtime)");
            log_fail_exit(r->exec_exit, r->exec_timed_out);
            if (r->exec_out && r->exec_out[0])
                fprintf(stderr, "%s", r->exec_out);
        }
    } else {
        g_tort_pass++;
        if (!summary_only) print_result(name, COL_GREEN, "PASS");
    }
    free(r->exec_out);
    r->exec_out = NULL;
    unlink(r->tmp_exe);
}

static void run_torture_test(const char *src, bool summary_only) {
    const char *fname = strrchr(src, '/');
    if (!fname) fname = src;
    else
        fname++;
    char name[256];
    strncpy(name, fname, sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';
    char *dot = strrchr(name, '.');
    if (dot) *dot = '\0';

    g_tort_total++;
    char *content = slurp(src);
    SkipReason sk = torture_should_skip(name, content);
    if (sk != SKIP_NONE) {
        g_tort_skip++;
        if (!summary_only) {
            char sr[64];
            snprintf(sr, sizeof(sr), "SKIP (%s)", skip_reason_str(sk));
            print_result(name, COL_YELLOW, sr);
        }
        free(content);
        return;
    }

#ifdef _WIN32
    /* In-process execution skipped for torture on Windows: many tests
     * call exit(0) which kills run_tests.exe (no fork() on Windows).
     * Fall through to the external process path. */
    (void)use_rcc_lib;
#endif

    char exe_path[512];
    snprintf(exe_path, sizeof(exe_path), "%s/torture_rcc_%s", get_tmpdir(), name);
    if (is_mingw_native || (has_runner && contains(runner_cmd, "wine")))
        strcat(exe_path, ".exe");
    char *ca[16];
    int ai = 0;
    ca[ai++] = (char *)rcc;
    ca[ai++] = (char *)rccflags;
    ca[ai++] = "-I";
    ca[ai++] = ".";
    ca[ai++] = "-o";
    ca[ai++] = exe_path;
    ca[ai++] = (char *)src;
    ca[ai++] = "-lm";
    ca[ai] = NULL;
    char *compile_cmdline = cmdline_from_argv(ca);
    ProcResult cr = proc_run(ca, 30, 0);

    if (cr.exit_code != 0) {
        if (contains(cr.out, "No such file") || contains(cr.out, "cannot open") ||
            contains(cr.out, "include file") || contains(cr.out, "not found")) {
            g_tort_skip++;
            if (!summary_only) print_result(name, COL_YELLOW, "SKIP (missing include)");
        } else {
            g_tort_fail_compile++;
            tort_add_error(&g_tort_compile_errors, name);
            if (!summary_only) {
                print_result(name, COL_RED, "FAIL (compile)");
                if (cr.out && cr.out[0]) fprintf(stderr, "%s", cr.out);
            }
        }
        vlog_test_details(name, compile_cmdline, cr.out, NULL, NULL);
        free(compile_cmdline);
        proc_free(&cr);
        free(content);
        return;
    }
    proc_free(&cr);

    if (streq(platform, "darwin_cross")) {
        g_tort_pass++;
        if (!summary_only) print_result(name, COL_GREEN, "PASS (compile only)");
        vlog_test_details(name, compile_cmdline, NULL, NULL, NULL);
        free(compile_cmdline);
        unlink(exe_path);
        free(content);
        return;
    }

    char *ra[32];
    int ri = 0;
    if (has_runner) {
        char *rc = strdup(runner_cmd);
        char *sv = NULL;
        char *tok = strtok_r(rc, " ", &sv);
        while (tok) {
            ra[ri++] = tok;
            tok = strtok_r(NULL, " ", &sv);
        }
    }
    ra[ri++] = exe_path;
    ra[ri] = NULL;
    char *run_cmdline = cmdline_from_argv(ra);
    ProcResult rr = proc_run(ra, has_runner ? 20 : 5, 0);
    if (rr.exit_code != 0) {
        g_tort_fail_runtime++;
        tort_add_error(&g_tort_runtime_errors, name);
        if (!summary_only) {
            print_result(name, COL_RED, "FAIL (runtime)");
            if (!has_runner) {
                static const char *tort_extra[] = {"-I", ".", "-lm", NULL};
                emit_backtrace(exe_path, NULL, src, rcc, rccflags,
                               tort_extra, NULL, NULL, NULL);
            }
        }
    } else {
        g_tort_pass++;
        if (!summary_only) print_result(name, COL_GREEN, "PASS");
    }
    vlog_test_details(name, compile_cmdline, NULL, run_cmdline, rr.out);
    free(compile_cmdline);
    free(run_cmdline);
    proc_free(&rr);
    unlink(exe_path);
    free(content);
}

static int run_torture_suite(bool summary_only) {
    g_tort_pass = g_tort_fail_compile = g_tort_fail_runtime = g_tort_skip = g_tort_total = 0;
    free(g_tort_compile_errors);
    g_tort_compile_errors = NULL;
    free(g_tort_runtime_errors);
    g_tort_runtime_errors = NULL;

    char tort_dir[PATH_MAX];
    snprintf(tort_dir, sizeof(tort_dir), "%s/test/torture", SCRIPT_DIR);
    if (!file_exists(tort_dir)) {
        fprintf(stderr, "Torture directory not found: %s\n", tort_dir);
        return 1;
    }

    /* open log file for torture output */
    if (!only_test && !summary_only) {
        char lp[PATH_MAX];
        snprintf(lp, sizeof(lp), "%s/test/torture_report_%s.log", SCRIPT_DIR, platform_suffix);
        open_log(lp);
    }

    logprintf("\n%sGCC torture tests%s\n", COL_CYAN, COL_RESET);

    char save_cwd[PATH_MAX];
    if (!getcwd(save_cwd, sizeof(save_cwd))) save_cwd[0] = '\0';
    if (chdir(tort_dir) != 0) {
        perror("chdir torture");
        return 1;
    }

    if (only_test) {
        char sp[512];
        if (strstr(only_test, ".c")) snprintf(sp, sizeof(sp), "%s", only_test);
        else
            snprintf(sp, sizeof(sp), "%s.c", only_test);
        if (file_exists(sp)) {
            only_test_found = true;
            run_torture_test(sp, summary_only);
        }
    } else {
        char **files = list_c_files_sorted(".");
        if (files) {
            if (g_num_workers > 1 && !only_test && !summary_only) {
                /* Save tort_dir for parallel compile */
                snprintf(g_tort_dir, sizeof(g_tort_dir), "%s", tort_dir);
                /* First pass: count */
                int n_tests = 0;
                for (char **f = files; *f; f++) {
                    const char *fn = strrchr(*f, '/');
                    if (!fn) fn = *f;
                    else
                        fn++;
                    char nbuf[256];
                    strncpy(nbuf, fn, sizeof(nbuf) - 1);
                    nbuf[sizeof(nbuf) - 1] = '\0';
                    char *dot = strrchr(nbuf, '.');
                    if (dot) *dot = '\0';
                    n_tests++;
                }
                if (n_tests > 0) {
                    ParallelJob *jobs = calloc((size_t)n_tests, sizeof(ParallelJob));
                    int idx = 0;
                    for (char **f = files; *f; f++) {
                        const char *fn = strrchr(*f, '/');
                        if (!fn) fn = *f;
                        else
                            fn++;
                        char nbuf[256];
                        strncpy(nbuf, fn, sizeof(nbuf) - 1);
                        nbuf[sizeof(nbuf) - 1] = '\0';
                        char *dot = strrchr(nbuf, '.');
                        if (dot) *dot = '\0';
                        /* Build absolute path */
                        char *abs_path = malloc(2 * PATH_MAX);
                        snprintf(abs_path, 2 * PATH_MAX, "%s/%s", g_tort_dir, fn);
                        jobs[idx].suite = SUITE_TORTURE;
                        jobs[idx].src_path = abs_path;
                        jobs[idx].base = strdup(nbuf);
                        jobs[idx].summary_only = summary_only;
                        jobs[idx].index = idx;
                        idx++;
                    }
                    parallel_dispatch(jobs, n_tests);
                    for (int i = 0; i < n_tests; i++) {
                        tort_evaluate_report(jobs[i].base, &jobs[i].result, summary_only);
                        free((void *)jobs[i].src_path);
                        free((void *)jobs[i].base);
                    }
                    free(jobs);
                }
                for (char **f = files; *f; f++) free(*f);
                free(files);
            } else {
                /* Sequential path (original) */
                for (char **f = files; *f; f++) {
                    run_torture_test(*f, summary_only);
                    free(*f);
                }
                free(files);
            }
        }
    }

    if (save_cwd[0] && chdir(save_cwd) != 0) perror("chdir");

    int max_fail;
    if (only_test)
        max_fail = 1;
    else if (streq(platform, "arm64_cross"))
        max_fail = 22;
    else if (streq(platform, "arm64"))
        max_fail = 0;
    else if (streq(platform, "darwin_cross"))
        max_fail = 2;
    else if (streq(platform, "mingw_cross"))
        max_fail = 1;
    else if (streq(platform, "mingw"))
        max_fail = 1;
    else
        max_fail = 0;

    int fail = g_tort_fail_compile + g_tort_fail_runtime;
    if (!only_test) {
        int eff = g_tort_total - g_tort_skip;
        int pct = eff > 0 ? g_tort_pass * 100 / eff : 0;
        const char *red = fail > max_fail ? COL_RED : "";
        const char *rst = fail > max_fail ? COL_RESET : "";
        printf("\nTorture: %d/%d passed (%d%%), ", g_tort_pass, eff, pct);
        if (fail)
            printf("%s%d fail (%d compile/%d runtime)%s, %d skipped.\n",
                   red, fail, g_tort_fail_compile, g_tort_fail_runtime, rst, g_tort_skip);
        else
            printf("0 failed, %d skipped.\n", g_tort_skip);
        if (g_log_fp) {
            fprintf(g_log_fp, "\nTorture: %d/%d passed (%d%%), ", g_tort_pass, eff, pct);
            if (fail)
                fprintf(g_log_fp, "%d failed (%d compile/%d runtime), %d skipped.\n",
                        fail, g_tort_fail_compile, g_tort_fail_runtime, g_tort_skip);
            else
                fprintf(g_log_fp, "0 failed, %d skipped.\n", g_tort_skip);
        }
        if (g_tort_compile_errors && *g_tort_compile_errors)
            logprintf("\nCompile failures: %s\n", g_tort_compile_errors);
        if (g_tort_runtime_errors && *g_tort_runtime_errors)
            logprintf("\nRuntime failures: %s\n", g_tort_runtime_errors);

        char sp[256];
        char sc[512];
        snprintf(sp, sizeof(sp), "test-torture-%s.summary", platform_suffix);
        snprintf(sc, sizeof(sc),
                 "SUITE=torture\nTOTAL=%d\nPASS=%d\nFAIL=%d\nFAIL_COMPILE=%d\nFAIL_RUNTIME=%d\nSKIP=%d\n",
                 g_tort_total, g_tort_pass, g_tort_fail_compile + g_tort_fail_runtime,
                 g_tort_fail_compile, g_tort_fail_runtime, g_tort_skip);
        write_summary(sp, sc);
    }

    close_log();
    return fail > max_fail ? 1 : 0;
}

/* ── compliance: parallel compile+exec ───────────────────────────── */
static void comp_compile_exec(const char *src_path, const char *base,
                              const char *gcc_path, ParallelResult *r, int index) {
    (void)base;
    memset(r, 0, sizeof(*r));
    snprintf(r->gcc_exe_path, sizeof(r->gcc_exe_path), "%s/rcc_par_gcc_%d", get_tmpdir(), index);
    snprintf(r->tmp_exe, sizeof(r->tmp_exe), "%s/rcc_par_rcc_%d", get_tmpdir(), index);
    if (is_mingw_native || (has_runner && contains(runner_cmd, "wine"))) {
        strcat(r->gcc_exe_path, ".exe");
        strcat(r->tmp_exe, ".exe");
    }

    /* compile with gcc */
    {
        char *ca[] = {(char *)gcc_path, "-o", r->gcc_exe_path, (char *)src_path, NULL};
        ProcResult cr = proc_run(ca, 30, 1);
        if (cr.exit_code != 0) {
            r->exit_code = cr.exit_code;
            proc_free(&cr);
            return;
        }
        proc_free(&cr);
    }
    /* compile with rcc */
    {
        char *ca[] = {(char *)rcc, "-o", r->tmp_exe, (char *)src_path, NULL};
        r->compile_cmdline = cmdline_from_argv(ca);
        ProcResult cr = proc_run(ca, 30, 1);
        if (cr.exit_code != 0) {
            r->exit_code = -1;
            r->compile_out = cr.out;
            cr.out = NULL;
            proc_free(&cr);
            return;
        }
        proc_free(&cr);
    }
    r->did_compile = true;

    /* run gcc */
    {
        char *ga[] = {r->gcc_exe_path, NULL};
        ProcResult gr = proc_run(ga, 5, 0);
        r->gcc_exec_exit = gr.exit_code;
        r->gcc_exec_out = gr.out;
        gr.out = NULL;
        proc_free(&gr);
    }
    /* run rcc */
    {
        char *ra[] = {r->tmp_exe, NULL};
        r->run_cmdline = cmdline_from_argv(ra);
        ProcResult rr = proc_run(ra, 5, 0);
        r->exec_exit = rr.exit_code;
        r->exec_timed_out = rr.timed_out;
        r->exec_out = rr.out;
        rr.out = NULL;
        proc_free(&rr);
    }
    r->did_exec = true;
}

/* ── compliance: parallel evaluate+report ────────────────────────── */

static void comp_evaluate_report(const char *base, ParallelResult *r,
                                 int *comp_pass, int *comp_fail, int *comp_skip,
                                 const char *gcc_path) {
    vlog_test_details(base, r->compile_cmdline, r->compile_out,
                      r->run_cmdline, r->exec_out);
    free(r->compile_cmdline);
    r->compile_cmdline = NULL;
    free(r->run_cmdline);
    r->run_cmdline = NULL;
    if (!gcc_path) {
        print_result(base, COL_YELLOW, "SKIP (gcc not found)");
        (*comp_skip)++;
        return;
    }
    /* gcc compile fail */
    if (r->exit_code > 0) {
        print_result(base, COL_YELLOW, "SKIP (gcc fail)");
        (*comp_skip)++;
        return;
    }
    /* rcc compile fail */
    if (r->exit_code == -1) {
        print_result(base, COL_RED, "FAIL (rcc compile)");
        if (r->compile_out && r->compile_out[0])
            fprintf(stderr, "    %s\n", r->compile_out);
        (*comp_fail)++;
        free(r->compile_out);
        r->compile_out = NULL;
        unlink(r->gcc_exe_path);
        return;
    }
    free(r->compile_out);
    r->compile_out = NULL;

    bool ok = (r->gcc_exec_exit == r->exec_exit) &&
        streq(r->gcc_exec_out ? r->gcc_exec_out : "", r->exec_out ? r->exec_out : "");
    if (ok) {
        print_result(base, COL_GREEN, "PASS");
        (*comp_pass)++;
    } else {
        print_result(base, COL_RED, "FAIL (output mismatch)");
        if (r->gcc_exec_out && r->gcc_exec_out[0]) printf("    gcc: %s", r->gcc_exec_out);
        if (r->exec_out && r->exec_out[0]) printf("    rcc: %s", r->exec_out);
        (*comp_fail)++;
    }
    free(r->gcc_exec_out);
    r->gcc_exec_out = NULL;
    free(r->exec_out);
    r->exec_out = NULL;
    unlink(r->gcc_exe_path);
    unlink(r->tmp_exe);
}

/* ═══════════════════════════════════════════════════════════════════
 * COMPLIANCE TEST SUITE  (gcc vs rcc output comparison, from ncc)
 * ═══════════════════════════════════════════════════════════════════ */

static int run_compliance_suite(void) {
    if (streq(platform, "darwin_cross")) {
        printf("\n%sNCC Compliance tests (test/compliance/)%s\n", COL_CYAN, COL_RESET);
        printf("SKIP (darwin-cross: compile+link only, no execution)\n");
        return 0;
    }

    char comp_dir[PATH_MAX];
    snprintf(comp_dir, sizeof(comp_dir), "%s/test/compliance", SCRIPT_DIR);
    if (!file_exists(comp_dir)) {
        printf("\nNCC Compliance directory not found: %s\n", comp_dir);
        return 0;
    }

    printf("\n%sNCC Compliance tests (test/compliance/)%s\n", COL_CYAN, COL_RESET);

    /* locate gcc */
    const char *gcc_path = getenv("GCC_FOR_TESTS");
    if (!gcc_path || !gcc_path[0]) {
#ifdef _WIN32
        /* Let CreateProcess search PATH for gcc.exe (e.g. the MinGW-w64
         * toolchain installed under the wine prefix's C:\mingw64\bin). The
         * Unix candidate paths below would resolve through wine's Z:\ drive
         * to a Linux ELF gcc, which can't be run directly here. */
        gcc_path = "gcc.exe";
#else
        static const char *gcc_cands[] = {"/usr/bin/gcc", "/usr/local/bin/gcc", NULL};
        for (const char **c = gcc_cands; *c; c++) {
            if (access(*c, X_OK) == 0) {
                gcc_path = *c;
                break;
            }
        }
        if (!gcc_path) {
            FILE *fp = popen("command -v gcc 2>/dev/null", "r");
            static char gcc_buf[256];
            if (fp && fgets(gcc_buf, sizeof(gcc_buf), fp)) {
                gcc_buf[strcspn(gcc_buf, "\n")] = '\0';
                if (gcc_buf[0]) gcc_path = gcc_buf;
            }
            if (fp) pclose(fp);
        }
#endif
    }

    int comp_pass = 0, comp_fail = 0, comp_skip = 0;
    struct dirent **nl;
    int n = scandir(comp_dir, &nl, NULL, alphasort);
    if (n < 0) return 0;

    if (g_num_workers > 1 && !only_test && gcc_path) {
        /* Parallel path: collect, dispatch, evaluate */
        struct {
            const char *fname;
            char base[256];
        } *entries = NULL;
        int n_tests = 0, n_alloc = 0;
        for (int i = 0; i < n; i++) {
            const char *fname = nl[i]->d_name;
            size_t flen = strlen(fname);
            if (flen < 3 || strcmp(fname + flen - 2, ".c") != 0)
                continue;
            char base[256];
            strncpy(base, fname, sizeof(base) - 1);
            base[sizeof(base) - 1] = '\0';
            char *dot = strrchr(base, '.');
            if (dot) *dot = '\0';
            /* skip mingw-specific test on native windows */
            if (is_mingw_native && streq(base, "15_long_double_conv"))
                continue;
            if (n_tests >= n_alloc) {
                n_alloc = n_alloc ? n_alloc * 2 : 16;
                entries = xrealloc(entries, (size_t)n_alloc * sizeof(*entries));
            }
            entries[n_tests].fname = nl[i]->d_name;
            strncpy(entries[n_tests].base, base, sizeof(entries[n_tests].base) - 1);
            entries[n_tests].base[sizeof(entries[n_tests].base) - 1] = '\0';
            n_tests++;
        }
        /* Report skips for entries we excluded */
        for (int i = 0; i < n; i++) {
            const char *fname = nl[i]->d_name;
            size_t flen = strlen(fname);
            if (flen < 3 || strcmp(fname + flen - 2, ".c") != 0)
                continue;
            char base[256];
            strncpy(base, fname, sizeof(base) - 1);
            base[sizeof(base) - 1] = '\0';
            char *dot = strrchr(base, '.');
            if (dot) *dot = '\0';
            if (is_mingw_native && streq(base, "15_long_double_conv")) {
                print_result(base, COL_YELLOW, "SKIP (%%Lf unsupported by MSVCRT)");
                comp_skip++;
            }
        }
        if (n_tests > 0) {
            ParallelJob *jobs = calloc((size_t)n_tests, sizeof(ParallelJob));
            for (int i = 0; i < n_tests; i++) {
                char *sp = malloc(2 * PATH_MAX);
                snprintf(sp, 2 * PATH_MAX, "%s/%s", comp_dir, entries[i].fname);
                jobs[i].suite = SUITE_COMPLIANCE;
                jobs[i].src_path = sp;
                jobs[i].base = strdup(entries[i].base);
                jobs[i].gcc_path = gcc_path;
                jobs[i].index = i;
            }
            parallel_dispatch(jobs, n_tests);
            for (int i = 0; i < n_tests; i++) {
                comp_evaluate_report(jobs[i].base, &jobs[i].result,
                                     &comp_pass, &comp_fail, &comp_skip, gcc_path);
                free((void *)jobs[i].src_path);
                free((void *)jobs[i].base);
            }
            free(jobs);
        }
        free(entries);
        for (int i = 0; i < n; i++) free(nl[i]);
        free(nl);
    } else {
        /* Sequential path (original) */
        for (int i = 0; i < n; i++) {
            const char *fname = nl[i]->d_name;
            size_t flen = strlen(fname);
            if (flen < 3 || strcmp(fname + flen - 2, ".c") != 0) {
                free(nl[i]);
                continue;
            }

            char base[256];
            strncpy(base, fname, sizeof(base) - 1);
            base[sizeof(base) - 1] = '\0';
            char *dot = strrchr(base, '.');
            if (dot) *dot = '\0';

            if (only_test) {
                if (!streq(base, only_test)) {
                    free(nl[i]);
                    continue;
                }
                only_test_found = true;
            }

            char src_path[PATH_MAX + NAME_MAX + 2];
            snprintf(src_path, sizeof(src_path), "%s/%s", comp_dir, fname);

            if (is_mingw_native && streq(base, "15_long_double_conv")) {
                print_result(base, COL_YELLOW, "SKIP (%%Lf unsupported by MSVCRT)");
                comp_skip++;
                free(nl[i]);
                continue;
            }

            if (!gcc_path) {
                print_result(base, COL_YELLOW, "SKIP (gcc not found)");
                comp_skip++;
                free(nl[i]);
                continue;
            }

            char gcc_exe[2 * PATH_MAX], rcc_exe[2 * PATH_MAX];
            snprintf(gcc_exe, sizeof(gcc_exe), "%s/compliance_gcc_%d_%s", get_tmpdir(), getpid(), base);
            snprintf(rcc_exe, sizeof(rcc_exe), "%s/compliance_rcc_%d_%s", get_tmpdir(), getpid(), base);

            char *rcc_compile_cmdline = NULL;
            char *rcc_compile_out = NULL;
            char *rcc_run_cmdline = NULL;
            char *rcc_run_out = NULL;

            { /* compile with gcc */
                char *ca[] = {(char *)gcc_path, "-o", gcc_exe, src_path, NULL};
                ProcResult cr = proc_run(ca, 30, 1);
                if (cr.exit_code != 0) {
                    print_result(base, COL_YELLOW, "SKIP (gcc fail)");
                    comp_skip++;
                    proc_free(&cr);
                    free(nl[i]);
                    continue;
                }
                proc_free(&cr);
            }
            { /* compile with rcc */
                char *ca[] = {(char *)rcc, "-o", rcc_exe, src_path, NULL};
                rcc_compile_cmdline = cmdline_from_argv(ca);
                ProcResult cr = proc_run(ca, 30, 1);
                if (cr.exit_code != 0) {
                    print_result(base, COL_RED, "FAIL (rcc compile)");
                    if (cr.out && cr.out[0]) fprintf(stderr, "    %s\n", cr.out);
                    vlog_test_details(base, rcc_compile_cmdline, cr.out, NULL, NULL);
                    free(rcc_compile_cmdline);
                    comp_fail++;
                    proc_free(&cr);
                    unlink(gcc_exe);
                    free(nl[i]);
                    continue;
                }
                proc_free(&cr);
            }

            char *ga[] = {gcc_exe, NULL};
            ProcResult gr = proc_run(ga, 5, 0);
            char *ra[] = {rcc_exe, NULL};
            rcc_run_cmdline = cmdline_from_argv(ra);
            ProcResult rr = proc_run(ra, 5, 0);
            rcc_run_out = rr.out;

            bool ok = (gr.exit_code == rr.exit_code) &&
                streq(gr.out ? gr.out : "", rr.out ? rr.out : "");
            if (ok) {
                print_result(base, COL_GREEN, "PASS");
                comp_pass++;
            } else {
                print_result(base, COL_RED, "FAIL (output mismatch)");
                if (gr.out && gr.out[0]) printf("    gcc: %s", gr.out);
                if (rr.out && rr.out[0]) printf("    rcc: %s", rr.out);
                comp_fail++;
            }
            vlog_test_details(base, rcc_compile_cmdline, rcc_compile_out,
                              rcc_run_cmdline, rcc_run_out);
            free(rcc_compile_cmdline);
            free(rcc_compile_out);
            free(rcc_run_cmdline);
            proc_free(&gr);
            proc_free(&rr);
            unlink(gcc_exe);
            unlink(rcc_exe);
            free(nl[i]);
        }
        free(nl);
    }
    /* Print summary and write test-compliance-<platform>.summary for both the
     * parallel and sequential paths, so `make check-all` (--parallel) gets a
     * compliance section in the unified report just like the sequential run. */
    int comp_total = comp_pass + comp_fail;
    if (!only_test) {
        int comp_pct = comp_total > 0 ? comp_pass * 100 / comp_total : 0;
        const char *red = comp_fail > 0 ? COL_RED : "";
        const char *rst = comp_fail > 0 ? COL_RESET : "";
        printf("\nCompliance: %d/%d passed (%d%%), %s%d failed%s",
               comp_pass, comp_total, comp_pct, red, comp_fail, rst);
        if (comp_skip) printf(", %d skipped", comp_skip);
        printf(".\n");

        char sp[256], sc[256];
        snprintf(sp, sizeof(sp), "test-compliance-%s.summary", platform_suffix);
        snprintf(sc, sizeof(sc), "SUITE=compliance\nTOTAL=%d\nPASS=%d\nFAIL=%d\n",
                 comp_total, comp_pass, comp_fail);
        write_summary(sp, sc);
    }
    return comp_fail > 0 ? 1 : 0;
}

/* ═══════════════════════════════════════════════════════════════════
 * C-TESTSUITE https://github.com/c-testsuite/c-testsuite
 * ═══════════════════════════════════════════════════════════════════ */

/* "210", "00210" or "00210.c" -> "00210"; false if not a c-testsuite name */
static bool ctest_test_num(char *out, size_t outsz) {
    char buf[32];
    strncpy(buf, only_test, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    char *dot = strstr(buf, ".c");
    if (dot) *dot = '\0';
    size_t len = strlen(buf);
    if (len == 0 || len > 5) return false;
    for (size_t i = 0; i < len; i++)
        if (!isdigit((unsigned char)buf[i])) return false;
    snprintf(out, outsz, "%05d", atoi(buf));
    return true;
}

/* c-testsuite/tests/single-exec is a flat numbered suite (00001.c..00220.c)
 * compile and run each test independently with a native C runner
 * requested test directly, the same way run_one_test() does for the TCC
 * suite (proc_run() to compile, run_exe() to execute under any runner). */
static int run_ctest_one(const char *ctest_dir) {
    char num[6];
    if (!ctest_test_num(num, sizeof(num))) return -1;

    char src_path[2 * PATH_MAX], exp_path[2 * PATH_MAX], bin_path[2 * PATH_MAX];
    snprintf(src_path, sizeof(src_path), "%s/tests/single-exec/%s.c", ctest_dir, num);
    snprintf(exp_path, sizeof(exp_path), "%s/tests/single-exec/%s.c.expected", ctest_dir, num);
    if (!file_exists(src_path)) return -1;

    printf("\n%sC-testsuite%s\n", COL_CYAN, COL_RESET);
    printf("Start c-testsuite with %s -O1 -lm\n", rcc);

    snprintf(bin_path, sizeof(bin_path), "%s/rcc_ctest_%d", get_tmpdir(), getpid());
    if (is_mingw_native || (has_runner && contains(runner_cmd, "wine")))
        strcat(bin_path, ".exe");

    char *ca[] = {(char *)rcc, "-O1", "-lm", "-o", bin_path, src_path, NULL};
    char *compile_cmdline = cmdline_from_argv(ca);
    ProcResult cr = proc_run(ca, 30, 1);
    int fail = 0;
    char *run_cmdline = NULL;
    ProcResult rr = {0};
    if (cr.exit_code != 0 || access(bin_path, X_OK) != 0) {
        print_result(num, COL_RED, "COMPILE FAIL");
        fail = 1;
    } else {
        rr = run_exe_with_cmdline(bin_path, "", 30, &run_cmdline);
        if (rr.exit_code != 0) {
            print_result(num, COL_RED, "FAIL (non-zero exit)");
            fail = 1;
        } else {
            char *expected_raw = slurp(exp_path);
            char *expected = normalize_output(expected_raw, num);
            char *actual = normalize_output(rr.out, num);
            free(expected_raw);
            if (!diff_strings(expected, actual, num)) {
                print_result(num, COL_RED, "FAIL");
                fail = 1;
            } else {
                print_result(num, COL_GREEN, "PASS");
            }
            free(expected);
            free(actual);
        }
    }
    vlog_test_details(num, compile_cmdline, cr.out, run_cmdline, rr.out);
    free(compile_cmdline);
    free(run_cmdline);
    if (rr.out) proc_free(&rr);
    proc_free(&cr);
    if (!fail) unlink(bin_path);
    return fail;
}

/* ── c-testsuite: parallel compile+exec ──────────────────────────── */

static void ctest_compile_exec(const char *src_path, const char *name,
                               ParallelResult *r, int index) {
    memset(r, 0, sizeof(*r));
    snprintf(r->tmp_exe, sizeof(r->tmp_exe), "%s/rcc_ctest_%d_%s", get_tmpdir(), index, name);
    if (is_mingw_native || (has_runner && contains(runner_cmd, "wine")))
        strcat(r->tmp_exe, ".exe");

#if 0 && defined(_WIN32) /* FIXME: rcc_lib not yet thread-safe for parallel workers */
    /* Fast path: compile+run in-process via rcc_lib.dll */
    if (!has_runner || is_mingw_native) {
        ProcResult rp;
        if (run_test_inprocess(src_path, name, NULL, "-O1 -lm", &rp, 30) == 0) {
            r->did_compile = true;
            r->did_exec = true;
            r->exec_exit = rp.exit_code;
            r->exec_timed_out = rp.timed_out;
            r->exec_out = rp.out;
            rp.out = NULL;
            return;
        }
    }
#endif
    char *ca[] = {(char *)rcc, "-O1", "-lm", "-o", r->tmp_exe, (char *)src_path, NULL};
    r->compile_cmdline = cmdline_from_argv(ca);
    ProcResult cr = proc_run(ca, 30, 1);
    if (cr.exit_code != 0 || access(r->tmp_exe, X_OK) != 0) {
        r->exit_code = cr.exit_code != 0 ? cr.exit_code : -1;
        r->compile_out = cr.out;
        cr.out = NULL;
        proc_free(&cr);
        return;
    }
    proc_free(&cr);
    r->did_compile = true;

    ProcResult rr = run_exe_with_cmdline(r->tmp_exe, "", 30, &r->run_cmdline);
    r->exec_exit = rr.exit_code;
    r->exec_timed_out = rr.timed_out;
    r->exec_out = rr.out;
    rr.out = NULL;
    proc_free(&rr);
    r->did_exec = true;
}

/* ── c-testsuite: parallel evaluate+report — returns true on PASS ───── */

static bool ctest_evaluate_report(const char *name, const char *exp_path, ParallelResult *r) {
    vlog_test_details(name, r->compile_cmdline, r->compile_out,
                      r->run_cmdline, r->exec_out);
    free(r->compile_cmdline);
    r->compile_cmdline = NULL;
    free(r->run_cmdline);
    r->run_cmdline = NULL;
    if (!r->did_compile) {
        print_result(name, COL_RED, "COMPILE FAIL");
        free(r->compile_out);
        r->compile_out = NULL;
        return false;
    }
    free(r->compile_out);
    r->compile_out = NULL;

    bool pass = false;
    if (r->exec_exit != 0) {
        print_result(name, COL_RED, "FAIL (non-zero exit)");
        log_fail_exit(r->exec_exit, r->exec_timed_out);
    } else {
        char *expected_raw = slurp(exp_path);
        char *expected = normalize_output(expected_raw, name);
        char *actual = normalize_output(r->exec_out, name);
        free(expected_raw);
        if (!diff_strings(expected, actual, name)) {
            print_result(name, COL_RED, "FAIL");
        } else {
            print_result(name, COL_GREEN, "PASS");
            pass = true;
        }
        free(expected);
        free(actual);
    }
    free(r->exec_out);
    r->exec_out = NULL;
    if (pass) unlink(r->tmp_exe);
    return pass;
}

static int run_ctest_suite(void) {
    if (streq(platform, "darwin_cross")) {
        printf("\n%sC-testsuite%s\n", COL_CYAN, COL_RESET);
        printf("SKIP (darwin-cross: compile+link only, no execution)\n");
        return 0;
    }

    char ctest_dir[PATH_MAX];
    snprintf(ctest_dir, sizeof(ctest_dir), "%s/c-testsuite", SCRIPT_DIR);

    /* the native runner handles per-test filters directly; for a numeric test
     * name, compile and run just that one test directly */
    if (only_test) {
        int r = run_ctest_one(ctest_dir);
        if (r >= 0) {
            only_test_found = true;
            return r;
        }
        return 0;
    }

    /* Native C runner: compile + run each .c file directly.
     * No shell dependency — works on all platforms incl. mingw/wine. */
    char test_dir[PATH_MAX + 32];
    snprintf(test_dir, sizeof(test_dir), "%s/tests/single-exec", ctest_dir);
    if (!file_exists(test_dir)) {
        if (!has_runner && !streq(platform, "darwin_cross") && !file_exists(ctest_dir)) {
            char gcmd[512];
            snprintf(gcmd, sizeof(gcmd), "git -C '%s' submodule update --init --recursive c-testsuite 2>/dev/null", SCRIPT_DIR);
            if (system(gcmd) != 0)
                fprintf(stderr, "warning: failed to fetch c-testsuite submodule\n");
        }
        if (!file_exists(test_dir)) {
            printf("\nc-testsuite not found, skipping\n");
            return 0;
        }
    }

    /* Detect wine early — git and system() call cmd.exe which can't
     * find native Linux binaries. */
    bool under_wine = false;
#ifdef _WIN32
    under_wine = GetProcAddress(GetModuleHandleA("ntdll.dll"), "wine_get_version") != NULL;
#else
    under_wine = (has_runner && contains(runner_cmd, "wine"));
#endif

    /* clean stale state left by a previous test run */
    if (!under_wine && !has_runner && !streq(platform, "darwin_cross")) {
        char fred[PATH_MAX + 32];
        snprintf(fred, sizeof(fred), "%s/fred.txt", ctest_dir);
        if (file_exists(fred)) {
            char gcmd[PATH_MAX + 64];
            snprintf(gcmd, sizeof(gcmd), "git -C '%s' clean -dxf . 2>/dev/null", ctest_dir);
            if (system(gcmd) != 0)
                fprintf(stderr, "warning: failed to clean c-testsuite directory\n");
        }
    }

    /* open log file for c-testsuite output */
    if (!only_test) {
        char lp[PATH_MAX];
        snprintf(lp, sizeof(lp), "%s/test/ctest_report_%s.log", SCRIPT_DIR, platform_suffix);
        open_log(lp);
    }

    logprintf("\n%sC-testsuite%s\n", COL_CYAN, COL_RESET);

    if (is_mingw_native || under_wine || (g_num_workers > 1 && !streq(platform, "darwin_cross"))) {
        /* Native per-test execution — no POSIX shell needed.
         * Same compile+run+compare as run_ctest_one(), iterated
         * over all tests in single-exec/ via list_c_files_sorted().
         * Always used for mingw/wine (no POSIX shell); also used for
         * --parallel/--jobs since the popen/TAP path below cannot be
         * parallelized. */
        logprintf("Start c-testsuite with %s -O1 -lm\n", rcc);

        char src_dir[PATH_MAX + 64];
        snprintf(src_dir, sizeof(src_dir), "%s/tests/single-exec", ctest_dir);
        char **files = list_c_files_sorted(src_dir);
        if (!files) {
            printf("  No c-testsuite tests found\n");
            close_log();
            return 0;
        }

        int ctest_pass = 0, ctest_fail2 = 0, ctest_skip2 = 0, ctest_total = 0;

        if (g_num_workers > 1) {
            int n_files = 0;
            for (char **f = files; *f; f++) n_files++;
            ParallelJob *jobs = calloc((size_t)n_files, sizeof(ParallelJob));
            int n_tests = 0;
            for (char **f = files; *f; f++) {
                /* list_c_files_sorted returns full paths; extract basename */
                const char *base = strrchr(*f, '/');
                base = base ? base + 1 : *f;
                char *dot = strrchr(base, '.');
                if (!dot || strcmp(dot, ".c") != 0) continue;

                char name[16];
                size_t nlen = (size_t)(dot - base);
                if (nlen >= sizeof(name)) nlen = sizeof(name) - 1;
                memcpy(name, base, nlen);
                name[nlen] = '\0';

                char exp_path[2 * PATH_MAX];
                snprintf(exp_path, sizeof(exp_path), "%s/%s.c.expected", src_dir, name);

                jobs[n_tests].suite = SUITE_CTEST;
                jobs[n_tests].src_path = strdup(*f);
                jobs[n_tests].base = strdup(name);
                jobs[n_tests].p_src = strdup(exp_path);
                jobs[n_tests].index = n_tests;
                n_tests++;
            }
            ctest_total = n_tests;
            parallel_dispatch(jobs, n_tests);
            for (int i = 0; i < n_tests; i++) {
                if (ctest_evaluate_report(jobs[i].base, jobs[i].p_src, &jobs[i].result))
                    ctest_pass++;
                else
                    ctest_fail2++;
                free((void *)jobs[i].src_path);
                free((void *)jobs[i].base);
                free((void *)jobs[i].p_src);
            }
            free(jobs);
            fflush(stdout);
        } else {
            for (char **f = files; *f; f++) {
                /* list_c_files_sorted returns full paths; extract basename */
                const char *base = strrchr(*f, '/');
                base = base ? base + 1 : *f;
                char *dot = strrchr(base, '.');
                if (!dot || strcmp(dot, ".c") != 0) continue;
                ctest_total++;

                char name[16];
                size_t nlen = (size_t)(dot - base);
                if (nlen >= sizeof(name)) nlen = sizeof(name) - 1;
                memcpy(name, base, nlen);
                name[nlen] = '\0';

                char exp_path[2 * PATH_MAX], bin_path[2 * PATH_MAX];
                snprintf(exp_path, sizeof(exp_path), "%s/%s.c.expected", src_dir, name);
                snprintf(bin_path, sizeof(bin_path), "%s/rcc_ctest_%d_%s",
                         get_tmpdir(), getpid(), name);
                if (is_mingw_native || (has_runner && contains(runner_cmd, "wine")))
                    strcat(bin_path, ".exe");

                char *ca[] = {(char *)rcc, "-O1", "-lm", "-o", bin_path, (char *)*f, NULL};
                ProcResult cr = proc_run(ca, 30, 1);
                bool test_pass = false;
                if (cr.exit_code != 0 || access(bin_path, X_OK) != 0) {
                    print_result(name, COL_RED, "COMPILE FAIL");
                    ctest_fail2++;
                } else {
                    ProcResult rr = run_exe(bin_path, "", 30);
                    if (rr.exit_code != 0) {
                        print_result(name, COL_RED, "FAIL (non-zero exit)");
                        ctest_fail2++;
                    } else {
                        char *expected_raw = slurp(exp_path);
                        char *expected = normalize_output(expected_raw, name);
                        char *actual = normalize_output(rr.out, name);
                        free(expected_raw);
                        if (!diff_strings(expected, actual, name)) {
                            print_result(name, COL_RED, "FAIL");
                            ctest_fail2++;
                        } else {
                            print_result(name, COL_GREEN, "PASS");
                            ctest_pass++;
                            test_pass = true;
                        }
                        free(expected);
                        free(actual);
                    }
                    proc_free(&rr);
                }
                proc_free(&cr);
                if (test_pass) unlink(bin_path);
                fflush(stdout);
            }
        }
        for (char **f = files; *f; f++) free(*f);
        free(files);

        int ctest_pct = ctest_total > 0 ? ctest_pass * 100 / ctest_total : 0;
        const char *red = ctest_fail2 > 0 ? COL_RED : "";
        const char *rst = ctest_fail2 > 0 ? COL_RESET : "";
        printf("\nC-testsuite: %d/%d passed (%d%%), %s%d failed%s, %d skipped.\n",
               ctest_pass, ctest_total, ctest_pct, red, ctest_fail2, rst, ctest_skip2);
        if (g_log_fp)
            fprintf(g_log_fp, "\nC-testsuite: %d/%d passed (%d%%), %d failed, %d skipped.\n",
                    ctest_pass, ctest_total, ctest_pct, ctest_fail2, ctest_skip2);

        if (!only_test) {
            char sp[256], sc[256];
            snprintf(sp, sizeof(sp), "test-ctest-%s.summary", platform_suffix);
            snprintf(sc, sizeof(sc), "SUITE=c-testsuite\nTOTAL=%d\nPASS=%d\nFAIL=%d\nSKIP=%d\n",
                     ctest_total, ctest_pass, ctest_fail2, ctest_skip2);
            write_summary(sp, sc);
        }
        close_log();
        return ctest_fail2 > 0 ? 1 : 0;
    }

    logprintf("Start c-testsuite with %s -O1 -lm\n", rcc);

    char cmd[PATH_MAX * 2 + 128];
    const char *stdbuf = access("/usr/bin/stdbuf", X_OK) == 0 ? "stdbuf -oL " : "";
    snprintf(cmd, sizeof(cmd),
             "cd '%s' && env CC='%s' CFLAGS='-O1 -lm' %s./single-exec posix 2>/dev/null",
             ctest_dir, rcc, stdbuf);

    FILE *fp = popen(cmd, "r");

    if (!fp) {
        fprintf(stderr, "c-testsuite: failed to run\n");
        close_log();
        return 1;
    }

    int ctest_pass = 0, ctest_fail = 0, ctest_skip = 0, ctest_total = 0;
    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        if (strncmp(line, "not ok ", 7) == 0) {
            ctest_fail++;
            const char *tname = line + 7;
            const char *sp = strchr(tname, ' ');
            if (sp) tname = sp + 1;
            if (tname[0] == '-' && tname[1] == ' ') tname += 2;
            print_result(tname, COL_RED, "FAIL");
            fflush(stdout);
        } else if (strncmp(line, "ok ", 3) == 0) {
            const char *tname = line + 3;
            const char *sp = strchr(tname, ' ');
            if (sp) tname = sp + 1;
            if (tname[0] == '-' && tname[1] == ' ') tname += 2;
            if (strstr(line, "# SKIP") || strstr(line, "# skip")) {
                ctest_skip++;
                print_result(tname, COL_YELLOW, "SKIP");
            } else {
                ctest_pass++;
                print_result(tname, COL_GREEN, "PASS");
                char outpath[2 * PATH_MAX], binpath[2 * PATH_MAX];
                snprintf(outpath, sizeof(outpath), "%s/%s.output", ctest_dir, tname);
                snprintf(binpath, sizeof(binpath), "%s/%s.bin", ctest_dir, tname);
                unlink(outpath);
                unlink(binpath);
            }
            fflush(stdout);
        } else if (strncmp(line, "1..", 3) == 0) {
            sscanf(line + 3, "%d", &ctest_total);
        }
    }
    pclose(fp);

    int ctest_pct = ctest_total > 0 ? ctest_pass * 100 / ctest_total : 0;
    const char *red = ctest_fail > 0 ? COL_RED : "";
    const char *rst = ctest_fail > 0 ? COL_RESET : "";
    printf("\nC-testsuite: %d/%d passed (%d%%), %s%d failed%s, %d skipped.\n",
           ctest_pass, ctest_total, ctest_pct, red, ctest_fail, rst, ctest_skip);
    if (g_log_fp)
        fprintf(g_log_fp, "\nC-testsuite: %d/%d passed (%d%%), %d failed, %d skipped.\n",
                ctest_pass, ctest_total, ctest_pct, ctest_fail, ctest_skip);

    if (!only_test) {
        char sp[256], sc[256];
        snprintf(sp, sizeof(sp), "test-ctest-%s.summary", platform_suffix);
        snprintf(sc, sizeof(sc), "SUITE=c-testsuite\nTOTAL=%d\nPASS=%d\nFAIL=%d\nSKIP=%d\n",
                 ctest_total, ctest_pass, ctest_fail, ctest_skip);
        write_summary(sp, sc);
    }
    close_log();
    return ctest_fail > 0 ? 1 : 0;
}


/* ═══════════════════════════════════════════════════════════════════
 * UNIFIED REPORT GENERATOR
 * ═══════════════════════════════════════════════════════════════════ */

static void generate_report(void) {
    static const struct {
        const char *id;
        const char *label;
    } suite_meta[] = {
        {"tcc", "TCC Compatibility Tests"},
        {"units", "RCC Unit Tests"},
        {"ctest", "c-testsuite"},
        {"compliance", "NCC Compliance Tests (vs GCC)"},
        {"torture", "GCC Torture Tests"},
    };
#define NSUITE sizeof(suite_meta)/sizeof(*suite_meta)

    const char *desc =
        streq(platform, "linux") ? "Linux x86_64" : streq(platform, "mingw_cross") ? "Windows x86_64 (mingw cross)"
        : streq(platform, "arm64_cross")                                           ? "Linux ARM64 (aarch64 cross)"
        : streq(platform, "darwin_cross")                                          ? "macOS ARM64 (darwin cross, compile+link only)"
        : streq(platform, "arm64")                                                 ? "macOS ARM64 (native)"
        : streq(platform, "mingw")                                                 ? "Windows x86_64 (native)"
                                                                                   : platform;

    char report_path[PATH_MAX];
    snprintf(report_path, sizeof(report_path), "%s/test_report_%s.md", SCRIPT_DIR, platform_suffix);
    char tmp_path[PATH_MAX + 5];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", report_path);

    struct {
        int total, pass, fail, skip, fail_compile, fail_runtime;
        bool found;
    } s[NSUITE] = {0};

    for (size_t i = 0; i < NSUITE; i++) {
        char sp[PATH_MAX];
        snprintf(sp, sizeof(sp), "%s/test-%s-%s.summary", SCRIPT_DIR, suite_meta[i].id, platform_suffix);
        char *c = slurp(sp);
        if (!c) continue;
        s[i].found = true;
        char *p = c;
        while (*p) {
            char *nl = strchr(p, '\n');
            if (nl) *nl = '\0';
            int v;
            if (sscanf(p, "TOTAL=%d", &v) == 1) s[i].total = v;
            else if (sscanf(p, "PASS=%d", &v) == 1)
                s[i].pass = v;
            else if (sscanf(p, "FAIL=%d", &v) == 1)
                s[i].fail = v;
            else if (sscanf(p, "SKIP=%d", &v) == 1)
                s[i].skip = v;
            else if (sscanf(p, "FAIL_COMPILE=%d", &v) == 1)
                s[i].fail_compile = v;
            else if (sscanf(p, "FAIL_RUNTIME=%d", &v) == 1)
                s[i].fail_runtime = v;
            if (!nl) break;
            p = nl + 1;
        }
        free(c);
    }

    int ov_total = 0, ov_pass = 0, ov_fail = 0;
    for (size_t i = 0; i < NSUITE; i++) {
        if (!s[i].found) continue;
        ov_total += s[i].total;
        ov_pass += s[i].pass;
        ov_fail += s[i].fail;
    }

    FILE *rf = fopen(tmp_path, "wb");
    if (!rf) {
        perror(tmp_path);
        return;
    }

    time_t t = time(NULL);
    char datebuf[64];
    strftime(datebuf, sizeof(datebuf), "%B %d %Y %H:%M", localtime(&t));

    fprintf(rf, "# RCC Test Suite Report\n\n");
    fprintf(rf, "**Platform**: %s\n\n", desc);
    if (compiler_name)
        fprintf(rf, "**Compiler**: %s\n\n", rcc);
    fprintf(rf, "Generated: %s\n\n", datebuf);

    fprintf(rf, "## Overall Summary\n\n");
    fprintf(rf, "- **Total**: %d\n", ov_total);
    fprintf(rf, "- **Passed**: %d\n", ov_pass);
    fprintf(rf, "- **Failed**: %d\n", ov_fail);
    if (ov_total > 0)
        fprintf(rf, "- **Overall Pass Rate**: %d%%\n", ov_pass * 100 / ov_total);

    for (size_t i = 0; i < NSUITE; i++) {
        if (!s[i].found || s[i].total == 0) continue;
        fprintf(rf, "\n## %s\n\n", suite_meta[i].label);
        fprintf(rf, "- **Total**: %d\n", s[i].total);
        fprintf(rf, "- **Passed**: %d\n", s[i].pass);
        fprintf(rf, "- **Failed**: %d\n", s[i].fail);
        if (s[i].skip > 0)
            fprintf(rf, "- **Skipped**: %d\n", s[i].skip);
        if (s[i].fail_compile > 0)
            fprintf(rf, "- **Fail Compile**: %d\n", s[i].fail_compile);
        if (s[i].fail_runtime > 0)
            fprintf(rf, "- **Fail Runtime**: %d\n", s[i].fail_runtime);
        int eff = s[i].total - s[i].skip;
        if (s[i].skip > 0 && eff > 0)
            fprintf(rf, "- **Pass Rate (excl. skip)**: %d%%\n", s[i].pass * 100 / eff);
        else if (s[i].total > 0)
            fprintf(rf, "- **Pass Rate**: %d%%\n", s[i].pass * 100 / s[i].total);
    }

    fclose(rf);
    move_file_atomic(tmp_path, report_path);
    printf("Unified report saved to test_report_%s.md\n", platform_suffix);
}

/* ═══════════════════════════════════════════════════════════════════
 * MAIN
 * ═══════════════════════════════════════════════════════════════════ */

int main(int argc, char **argv) {
#ifdef _WIN32
    /* enable ANSI escape sequence processing (COL_GREEN etc.) on the
     * native console; ansi.sys is not loaded by default on Windows */
    HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    if (GetConsoleMode(hout, &mode))
        SetConsoleMode(hout, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif

    /* resolve script dir and chdir to it */
    {
        char *dir = strdup(argv[0]);
        static char script_dir_buf[PATH_MAX];
        if (realpath(dirname(dir), script_dir_buf)) SCRIPT_DIR = script_dir_buf;
        else
            SCRIPT_DIR = dirname(dir);
        if (chdir(SCRIPT_DIR) != 0) {
            perror("chdir");
            return 1;
        }
        free(dir);
    }

    rccflags = "-O1";
    bool run_tcc = false, run_units = false, run_torture = false;
    bool run_compliance = false, run_ctest = false;
    bool summary_only = false;

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (streq(a, "--all")) {
            run_tcc = run_units = run_torture = run_compliance = run_ctest = true;
        } else if (streq(a, "--tcc"))
            run_tcc = true;
        else if (streq(a, "--unit-tests") || streq(a, "--units"))
            run_units = true;
        else if (streq(a, "--torture"))
            run_torture = true;
        else if (streq(a, "--compliance"))
            run_compliance = true;
        else if (streq(a, "--ctest"))
            run_ctest = true;
        else if (streq(a, "--summary"))
            summary_only = true;
        else if (streq(a, "-v") || streq(a, "--verbose"))
            g_verbose = true;
        else if (streq(a, "--no-color"))
            g_no_color = true;
        else if (streq(a, "--parallel")) {
            g_num_workers = 4;
#ifdef _WIN32
            DWORD n = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
            if (n > 0) g_num_workers = (int)n;
#elif defined(_SC_NPROCESSORS_ONLN)
            long n = sysconf(_SC_NPROCESSORS_ONLN);
            if (n > 0) g_num_workers = (int)n;
#endif
        } else if (streq(a, "--jobs") && i + 1 < argc) {
            g_num_workers = atoi(argv[++i]);
            if (g_num_workers < 1) g_num_workers = 1;
        } else if (streq(a, "--help") || streq(a, "-h")) {
            printf("Usage: ./run_tests [rcc-binary] [options] [test-names...]\n\n");
            printf("Options (default: --tcc --unit-tests --compliance --ctest):\n");
            printf("  --tcc         TCC compatibility tests (tinycc/tests/tests2/)\n");
            printf("  --unit-tests  RCC Unit tests (test/test_*.c)\n");
            printf("  --torture     GCC torture tests (test/torture/)\n");
            printf("  --compliance  NCC Compliance tests (gcc vs rcc output comparison)\n");
            printf("  --ctest       C-testsuite (native C runner)\n");
            printf("  --all         All test suites\n");
            printf("  -v, --verbose Show compile/run command lines and output for each test\n");
            printf("  --summary     Torture summary-only (no per-test output)\n");
            printf("  --no-color    Disable ANSI color output\n");
            printf("  --parallel    Run tests in parallel (auto-detect worker count)\n");
            printf("  --jobs N      Run tests with N worker threads (--jobs 1 = sequential)\n\n");
            printf("rcc-binary      cc, with optional options (in-proc or auto if not given)\n");
            printf("test-names...   Run only these tests\n");
            return 0;
        }
        /* A single arg containing whitespace is a "compiler + flags" string,
           e.g. "ccc -O2": split into binary (rcc) + flags (rccflags).  This
           always uses an external process — never the in-proc rcc_lib. */
        else if (!rcc && strchr(a, ' ') != NULL) {
            char *copy = strdup(a);
            char *sp = strchr(copy, ' ');
            *sp = '\0';
            rcc = copy;
            char *flags = sp + 1;
            while (*flags == ' ') flags++;
            if (*flags) rccflags = flags;
        }
        /* first non-flag arg that names a compiler (rcc, gcc, tcc, *-cross):
           treat as the rcc binary.  Otherwise it's a test-name filter and
           rcc stays unset → in-process rcc_lib mode. */
        else if (!rcc && (contains(a, "rcc") || contains(a, "gcc") || contains(a, "tcc") || contains(a, "-cross")))
            rcc = a;
        else if (!only_test)
            only_test = a;
    }

    if (!rcc) {
        /* No rcc-binary arg given: default to in-process compile+run via
         * rcc_lib (see run_test_inprocess()).  `rcc` is still resolved to
         * an external binary for detect_platform() and as a fallback. */
        use_rcc_lib = true;
#ifdef _WIN32
        /* Under wine/Windows, prefer .exe over native binary */
        if (access("./rcc.exe", X_OK) == 0)
            rcc = "./rcc.exe";
#else
        if (access("./rcc", X_OK) == 0)
            rcc = "./rcc";
#endif
    }
    if (!rcc) {
        fprintf(stderr, "rcc binary not found\n");
        return 1;
    }

    /* External/unknown compiler (not one of rcc's own binaries: rcc,
     * rcc.exe, rcc-arm64, rcc-darwin, ...): remember its basename (as
     * given, before realpath resolution turns e.g. /usr/bin/gcc into a
     * versioned symlink target like x86_64-linux-gnu-gcc-13) so report
     * filenames can be suffixed with it, e.g. test_report_linux_gcc.md,
     * to avoid clobbering rcc's own reports for the same platform. */
    {
        const char *rb = strrchr(rcc, '/');
        rb = rb ? rb + 1 : rcc;
        if (!contains(rb, "rcc")) {
            static char name_buf[256];
            snprintf(name_buf, sizeof(name_buf), "%s", rb);
            compiler_name = name_buf;
        }
    }

    /* resolve to absolute path */
    {
        char resolved[PATH_MAX];
        static char rcc_buf[PATH_MAX];
        if (realpath(rcc, resolved)) {
            memcpy(rcc_buf, resolved, strlen(resolved) + 1);
            rcc = rcc_buf;
        }
    }

    /* rewrite shorthand cross-compiler paths.
       FIXME: should be able to run rcc-arm64 via gcc-aarch64 */
    /* rewrite shorthand cross-compiler paths.
       Only when cross-compiling — not on native ARM64 or x86 builds */
#if !defined(ARM64_NATIVE) && !defined(__aarch64__)
    if (contains(rcc, "rcc-arm64") && !contains(rcc, "arm64-cross")) {
        static char buf[PATH_MAX + 32];
        snprintf(buf, sizeof(buf), "%s/arm64-cross.sh", SCRIPT_DIR);
        rcc = buf;
    }
#endif

    if (contains(rcc, "rcc-darwin") && !contains(rcc, "darwin-cross")) {
        static char buf[PATH_MAX + 32];
        snprintf(buf, sizeof(buf), "%s/darwin-cross.sh", SCRIPT_DIR);
        rcc = buf;
    }

    detect_platform(rcc);
    if (g_verbose)
        printf("rcc=%s, platform=%s\n", rcc, platform);

    if (compiler_name) {
        static char suffix_buf[PATH_MAX];
        snprintf(suffix_buf, sizeof(suffix_buf), "%s_%s", platform, compiler_name);
        platform_suffix = suffix_buf;
    } else {
        platform_suffix = platform;
    }

    /* in-proc execution requires a native, same-architecture rcc; disable
     * it for cross-compile targets (runner needed, or darwin-cross) */
    if (has_runner || is_darwin_cross)
        use_rcc_lib = false;
    if (use_rcc_lib && g_verbose)
        printf("in-process mode: compiling+running tests via rcc_lib\n");

    /* suppress wine fixme noise so it doesn't pollute captured test output */
    if (has_runner && contains(runner_cmd, "wine"))
        setenv("WINEDEBUG", "fixme-all", 0);

    /* wine's console mangles ANSI SGI sequences (the ESC byte of e.g.
     * "\033[0;36m" gets eaten, leaving literal "[0;36m" in the output) */
    if (streq(platform, "mingw_cross") || (has_runner && contains(runner_cmd, "wine")))
        g_no_color = true;
#ifdef _WIN32
    if (GetProcAddress(GetModuleHandleA("ntdll.dll"), "wine_get_version"))
        g_no_color = true;
#endif

    /* for cross-compilers, suites that require native execution don't apply */
    if (!run_tcc && !run_units && !run_torture && !run_compliance && !run_ctest) {
        if (only_test) {
            /* single-test mode: search the filterable suites in order and
             * stop at the first one containing the test */
            run_tcc = run_units = run_compliance = run_ctest = run_torture = true;
        } else if (has_runner || streq(platform, "darwin_cross")) {
            run_tcc = run_torture = true;
        } else {
            run_tcc = run_units = run_compliance = run_ctest = true;
        }
    }

    int exit_code = 0;
    if (run_tcc && run_tcc_suite() != 0)
        exit_code = 1;
    if (only_test && only_test_found)
        return exit_code;
    if (run_units && run_unit_tests() != 0)
        exit_code = 1;
    if (only_test && only_test_found)
        return exit_code;
    if (run_compliance && run_compliance_suite() != 0)
        exit_code = 1;
    if (only_test && only_test_found)
        return exit_code;
    if (run_ctest && run_ctest_suite() != 0)
        exit_code = 1;
    if (only_test && only_test_found)
        return exit_code;
    if (run_torture && run_torture_suite(summary_only) != 0)
        exit_code = 1;

    if (only_test) {
        if (!only_test_found) {
            fprintf(stderr, "Test '%s' not found in any suite\n", only_test);
            return 1;
        }
        return exit_code;
    }

    generate_report();

    return exit_code;
}
