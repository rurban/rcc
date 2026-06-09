/* SPDX-License-Identifier: LGPL-2.1-or-later
 * Unified test runner for rcc.
 *
 * Replaces: run_tcc_suite.sh, test/torture/run.sh,
 *           test/compliance/run.sh, run-c-testsuite.sh
 *
 * Usage: ./run_tests [rcc-binary] [options] [test-name]
 *
 * Options (default: --tcc --unit-tests --compliance --ctest):
 *   --tcc         TCC compatibility tests (tinycc/tests/tests2/)
 *   --unit-tests  Our own Unit tests (test/test_*.c)
 *   --compliance  NCC compliance tests (gcc vs rcc output comparison)
 *   --ctest       C-testsuite (posix single-exec suite)
 *   --torture     GCC torture tests (test/torture/)
 *   --all         All test suites
 *   --summary     Torture summary-only mode
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <libgen.h>
#include <limits.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>

/* ── unified result output ───────────────────────────────────────── */

#define COL_GREEN  "\033[0;32m"
#define COL_RED    "\033[0;31m"
#define COL_YELLOW "\033[0;33m"
#define COL_RESET  "\033[0m"

static FILE *g_log_fp = NULL;

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

/* ── process execution ───────────────────────────────────────────── */

static volatile sig_atomic_t alarm_fired;
static void on_alarm(int sig) {
    (void)sig;
    alarm_fired = 1;
}

typedef struct {
    char *out;
    size_t out_len;
    int exit_code;
    bool timed_out;
    bool spawn_failed;
} ProcResult;

static void proc_free(ProcResult *r) {
    free(r->out);
    r->out = NULL;
}

/* capture: 0 = both stdout+stderr, 1 = stderr only, 2 = stdout only */
static ProcResult proc_run(char *const argv[], int timeout_sec, int capture) {
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

    pid_t pid = fork();
    if (pid < 0) {
        close(out_pipe[0]);
        close(out_pipe[1]);
        if (capture != 0) {
            close(err_pipe[0]);
            close(err_pipe[1]);
        }
        r.spawn_failed = true;
        return r;
    }
    if (pid == 0) {
        if (capture == 1) {
            close(err_pipe[0]);
            dup2(err_pipe[1], STDERR_FILENO);
            close(err_pipe[1]);
            close(out_pipe[0]);
            close(out_pipe[1]);
        } else if (capture == 2) {
            close(out_pipe[0]);
            dup2(out_pipe[1], STDOUT_FILENO);
            close(out_pipe[1]);
            close(err_pipe[0]);
            close(err_pipe[1]);
        } else {
            close(out_pipe[0]);
            dup2(out_pipe[1], STDOUT_FILENO);
            dup2(out_pipe[1], STDERR_FILENO);
            close(out_pipe[1]);
        }
        execvp(argv[0], argv);
        _exit(127);
    }

    close(out_pipe[1]);
    if (capture != 0) close(err_pipe[1]);

    struct sigaction sa = {.sa_handler = on_alarm, .sa_flags = 0};
    sigemptyset(&sa.sa_mask);
    struct sigaction old_sa;
    sigaction(SIGALRM, &sa, &old_sa);
    alarm_fired = 0;
    alarm((unsigned)timeout_sec);

    int read_fd = capture == 1 ? err_pipe[0] : out_pipe[0];
    size_t cap = 8192;
    r.out = malloc(cap);
    r.out_len = 0;
    char buf[8192];
    ssize_t n;
    while ((n = read(read_fd, buf, sizeof(buf))) > 0) {
        if (r.out_len + (size_t)n + 1 > cap) {
            cap = r.out_len + (size_t)n + 8192;
            r.out = realloc(r.out, cap);
        }
        memcpy(r.out + r.out_len, buf, (size_t)n);
        r.out_len += (size_t)n;
    }
    close(read_fd);
    if (capture != 0) close(out_pipe[0]);

    int status;
    pid_t w = waitpid(pid, &status, 0);
    alarm(0);
    sigaction(SIGALRM, &old_sa, NULL);

    if (w == pid) {
        if (WIFEXITED(status)) r.exit_code = WEXITSTATUS(status);
        else if (WIFSIGNALED(status))
            r.exit_code = 128 + WTERMSIG(status);
    }
    if (alarm_fired && r.exit_code == -1) {
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        r.timed_out = true;
    }

    if (!r.out) r.out = strdup("");
    else {
        r.out = realloc(r.out, r.out_len + 1);
        r.out[r.out_len] = '\0';
    }
    return r;
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

static char *strappend(char *buf, size_t *len, size_t *cap, const char *fmt, ...)
    __attribute__((format(printf, 4, 5)));
static char *strappend(char *buf, size_t *len, size_t *cap, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int need = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (need <= 0) return buf;
    size_t n = (size_t)need;
    if (*len + n + 1 > *cap) {
        *cap = *len + n + 4096;
        buf = realloc(buf, *cap);
    }
    va_start(ap, fmt);
    vsnprintf(buf + *len, n + 1, fmt, ap);
    va_end(ap);
    *len += n;
    return buf;
}

/* ── platform detection ──────────────────────────────────────────── */

static const char *PLATFORM = "linux";
static bool is_arm64, is_darwin_cross, is_mingw_native;
static char runner_cmd[512];
static bool has_runner;
static char *arm64_sysroot;

static void detect_platform(const char *rcc_path) {
    struct utsname u;
    uname(&u);

    if (contains(rcc_path, "arm64-cross") || contains(rcc_path, "rcc-arm64")) {
        PLATFORM = "arm64_cross";
        is_arm64 = true;
        static const char *sysroots[] = {
            "/usr/aarch64-linux-gnu",
            "/usr/aarch64-redhat-linux/sys-root/fc43",
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
    } else if (contains(rcc_path, "darwin-cross") || contains(rcc_path, "rcc-darwin")) {
        PLATFORM = "darwin_cross";
        is_darwin_cross = true;
        is_arm64 = true;
    } else if (contains(rcc_path, "mingw-cross")) {
        PLATFORM = "mingw_cross";
        snprintf(runner_cmd, sizeof(runner_cmd), "wine");
        has_runner = true;
    } else if (contains(rcc_path, ".exe")) {
        PLATFORM = "mingw";
        is_mingw_native = true;
        if (strcmp(u.sysname, "Linux") == 0 && access("/usr/bin/wine", X_OK) == 0) {
            snprintf(runner_cmd, sizeof(runner_cmd), "wine");
            has_runner = true;
        }
    } else {
        if (strcmp(u.sysname, "Darwin") == 0) PLATFORM = "arm64";
        else if (contains(u.sysname, "MINGW") || contains(u.sysname, "MSYS") ||
                 contains(u.sysname, "CYGWIN"))
            PLATFORM = "mingw";
        else
            PLATFORM = "linux";
        if (strcmp(u.machine, "aarch64") == 0 || strcmp(u.machine, "arm64") == 0)
            is_arm64 = true;
    }
}

static ProcResult run_exe(const char *exe_path, const char *args, int timeout_sec) {
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
                        char **tmp = realloc(names, (size_t)ncap * sizeof(char *));
                        if (!tmp) {
                            fprintf(stderr, "realloc: %s\n", strerror(errno));
                            free(names);
                            return NULL;
                        }
                        names = tmp;
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
    char **tmp = realloc(names, (size_t)(ncount + 1) * sizeof(char *));
    if (!tmp) {
        fprintf(stderr, "realloc: %s\n", strerror(errno));
        free(names);
        return NULL;
    }
    names = tmp;
    names[ncount] = NULL;
    return names;
}

/* extra: NULL-terminated extra compile flags (e.g. {"-I",".","-lm",NULL}); may be NULL.
 * ob/ol/oc: output buffer for TCC suite (append mode); pass all NULL to print directly. */
static void emit_backtrace(const char *exe_path, const char *args,
                           const char *src_file, const char *rcc, const char *cflags,
                           const char *const extra[],
                           char **ob, size_t *ol, size_t *oc) {
    char dbg[512];
    snprintf(dbg, sizeof(dbg), "%s.dbg", exe_path);
    bool have_dbg = false;
    if (src_file && file_exists(src_file)) {
        char *a[32];
        int ai = 0;
        a[ai++] = (char *)rcc;
        if (cflags && *cflags) a[ai++] = (char *)cflags;
        if (extra)
            for (int i = 0; extra[i] && ai < 28; i++) a[ai++] = (char *)extra[i];
        a[ai++] = "-g";
        a[ai++] = "-o";
        a[ai++] = dbg;
        a[ai++] = (char *)src_file;
        a[ai] = NULL;
        ProcResult cr = proc_run(a, 30, 1);
        if (cr.exit_code == 0) have_dbg = true;
        proc_free(&cr);
    }
    const char *dp = have_dbg ? dbg : exe_path;
#define BT_OUT(fmt, ...) do { \
    if (ob) *ob = strappend(*ob, ol, oc, fmt, ##__VA_ARGS__); \
    else logprintf(fmt, ##__VA_ARGS__); \
} while (0)
    BT_OUT("\n=== EXEC_FAIL backtrace ===\n");
    BT_OUT("command: %s %s\n", dp, args ? args : "");
    if (access("/usr/bin/lldb", X_OK) == 0) {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd),
                 "lldb --batch --one-line run --one-line-on-crash \"thread backtrace all\" "
                 "--one-line-on-crash \"register read pc lr x0 x1 x2 x16 x17\" "
                 "--one-line-on-crash \"quit 1\" -- %s %s </dev/null 2>&1",
                 dp, args ? args : "");
        FILE *fp = popen(cmd, "r");
        if (fp) {
            char ln[1024];
            while (fgets(ln, sizeof(ln), fp)) BT_OUT("%s", ln);
            pclose(fp);
        }
    } else if (access("/usr/bin/gdb", X_OK) == 0) {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd),
                 "gdb -q -batch -ex run -ex \"thread apply all bt\" --args %s %s 2>&1",
                 dp, args ? args : "");
        FILE *fp = popen(cmd, "r");
        if (fp) {
            char ln[1024];
            while (fgets(ln, sizeof(ln), fp)) BT_OUT("%s", ln);
            pclose(fp);
        }
    } else {
        BT_OUT("No debugger found.\n");
    }
#undef BT_OUT
    if (have_dbg) unlink(dbg);
}

/* ── TCC suite globals ───────────────────────────────────────────── */

static const char *RCC, *RCCFLAGS, *TEST_DIR, *REPORT_FILE, *SCRIPT_DIR;
static const char *ONLY_TEST;
static int total, passed, failed, regressions, fixes, changed_cnt;

typedef struct {
    char *name, *status, *message;
} ReportRow;
static ReportRow *report_rows;
static int nrows, rows_cap;

static void add_row(const char *name, const char *status, const char *message) {
    if (nrows >= rows_cap) {
        rows_cap = rows_cap ? rows_cap * 2 : 256;
        ReportRow *tmp = realloc(report_rows, (size_t)rows_cap * sizeof(ReportRow));
        if (!tmp) {
            fprintf(stderr, "realloc: %s\n", strerror(errno));
            return;
        }
        report_rows = tmp;
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
                OldState *tmp = realloc(old_states, (size_t)(nold + 1) * sizeof(OldState));
                if (!tmp) {
                    fprintf(stderr, "realloc: %s\n", strerror(errno));
                    break;
                }
                old_states = tmp;
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
    const char *old = old_status_for(base);
    if (!old) return;
    const char *oc = streq(old, "COMPILE_OK") ? "PASS" : old;
    const char *nc = streq(new_status, "COMPILE_OK") ? "PASS" : new_status;
    if (streq(oc, nc)) return;
    if (streq(nc, "PASS")) {
        printf("    \033[0;32m-> FIXED\033[0m (was %s)\n", old);
        fixes++;
    } else if (streq(oc, "PASS")) {
        printf("    \033[0;31m-> REGRESSION\033[0m (was PASS)\n");
        regressions++;
    } else {
        printf("    \033[0;33m-> CHANGED\033[0m (%s -> %s)\n", old, new_status);
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
    const char *orig_src = src_path, *orig_rcc = RCC;
    const char *fname_only = strrchr(orig_src, '/');
    if (!fname_only) fname_only = orig_src;
    else
        fname_only++;

    if (is_cd_test(base)) {
        char rcc_abs[PATH_MAX];
        if (realpath(RCC, rcc_abs)) {
            static char buf[PATH_MAX];
            strncpy(buf, rcc_abs, sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';
            RCC = buf;
        }
        chdir(TEST_DIR);
        src_path = fname_only;
        in_cd_dir = true;
    }

    char tmp_exe[256];
    snprintf(tmp_exe, sizeof(tmp_exe), "/tmp/rcc_test_%d", getpid());
    if (is_mingw || (has_runner && contains(runner_cmd, "wine")))
        strcat(tmp_exe, ".exe");

    char *out_buf = NULL;
    size_t out_len = 0, out_cap = 0;

    /* DT tests */
    if (is_dt_test(base)) {
        char **dt = extract_dt_tests(src_path);
        if (dt) {
            for (char **tn = dt; *tn; tn++) {
                char *ca[16];
                int ai = 0;
                ca[ai++] = (char *)RCC;
                ca[ai++] = (char *)RCCFLAGS;
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
                ProcResult cr = proc_run(ca, 30, 0);
                out_buf = strappend(out_buf, &out_len, &out_cap, "[%s]\n", *tn);
                if (cr.exit_code == 0) {
                    if (cr.out_len > 0) {
                        strip_ansi(cr.out);
                        out_buf = strappend(out_buf, &out_len, &out_cap, "%s", cr.out);
                    }
                    if (!is_darwin_cross) {
                        ProcResult rr = run_exe(tmp_exe, "", 20);
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
            chdir(SCRIPT_DIR);
            src_path = orig_src;
            RCC = orig_rcc;
            in_cd_dir = false;
        }
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
                    char sp[512];
                    snprintf(sp, sizeof(sp), "%s/test/%s.out", SCRIPT_DIR, base);
                    FILE *sf = fopen(sp, "w");
                    if (sf) {
                        if (out_buf) fputs(out_buf, sf);
                        fclose(sf);
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

    /* 128_run_atexit special handling */
    if (streq(base, "128_run_atexit")) {
        const char *tests[] = {"test_128_return", "test_128_exit", NULL};
        int exp_rc[] = {1, 2};
        for (int t = 0; tests[t]; t++) {
            char *ca[8];
            int ai = 0;
            ca[ai++] = (char *)RCC;
            ca[ai++] = (char *)RCCFLAGS;
            char df[64];
            snprintf(df, sizeof(df), "-D%s", tests[t]);
            ca[ai++] = df;
            ca[ai++] = "-o";
            ca[ai++] = tmp_exe;
            ca[ai++] = (char *)src_path;
            ca[ai] = NULL;
            ProcResult cr = proc_run(ca, 30, 0);
            out_buf = strappend(out_buf, &out_len, &out_cap, "[%s]\n", tests[t]);
            if (is_darwin_cross) {
                out_buf = strappend(out_buf, &out_len, &out_cap, "[linked]\n");
            } else {
                ProcResult rr = run_exe(tmp_exe, "", 10);
                int rc = rr.exit_code;
                out_buf = strappend(out_buf, &out_len, &out_cap, "%s", rr.out);
                out_buf = strappend(out_buf, &out_len, &out_cap, "[returns %d]\n", rc);
                if (rc != exp_rc[t])
                    emit_backtrace(tmp_exe, "", src_path, RCC, RCCFLAGS, NULL, &out_buf, &out_len, &out_cap);
                proc_free(&rr);
            }
            if (t == 0) out_buf = strappend(out_buf, &out_len, &out_cap, "\n");
            proc_free(&cr);
        }
        if (in_cd_dir) {
            chdir(SCRIPT_DIR);
            src_path = orig_src;
            RCC = orig_rcc;
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
        free(out_buf);
        return;
    }

    /* normal test: compile */
    {
        char *ca[16];
        int ai = 0;
        ca[ai++] = (char *)RCC;
        ca[ai++] = (char *)RCCFLAGS;
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
        ProcResult cr = proc_run(ca, 30, 0);
        if (cr.exit_code != 0) {
            print_result(base, COL_RED, "COMPILE FAIL");
            failed++;
            add_row(base, "COMPILE_FAIL", "rcc returned non-zero");
            print_change(base, "COMPILE_FAIL");
            if (cr.out && cr.out[0]) printf("%s", cr.out);
            if (in_cd_dir) {
                chdir(SCRIPT_DIR);
                src_path = orig_src;
                RCC = orig_rcc;
            }
            proc_free(&cr);
            return;
        }
        if (cr.out && cr.out[0]) out_buf = strappend(out_buf, &out_len, &out_cap, "%s", cr.out);
        proc_free(&cr);
    }

    if (in_cd_dir) {
        chdir(SCRIPT_DIR);
        src_path = orig_src;
        RCC = orig_rcc;
        in_cd_dir = false;
    }

    if (access(tmp_exe, X_OK) != 0) {
        print_result(base, COL_RED, "COMPILE FAIL");
        failed++;
        add_row(base, "COMPILE_FAIL", "executable missing");
        print_change(base, "COMPILE_FAIL");
        return;
    }

    if (is_darwin_cross) {
        print_result(base, COL_GREEN, "PASS (compile only)");
        passed++;
        add_row(base, "COMPILE_OK", "linked, (execution skipped)");
        print_change(base, "COMPILE_OK");
        return;
    }

    const char *args = test_args(base);
    int expected_exit = test_expected_exit(base);
    int actual_exit;

    if (streq(base, "46_grep")) {
        char save_cwd[PATH_MAX];
        getcwd(save_cwd, sizeof(save_cwd));
        chdir(TEST_DIR);
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
        ProcResult rr = proc_run(ga, 20, 2);
        out_buf = strappend(out_buf, &out_len, &out_cap, "%s", rr.out);
        actual_exit = rr.exit_code;
        proc_free(&rr);
        chdir(save_cwd);
    } else {
        ProcResult rr = run_exe(tmp_exe, args, args && *args ? 20 : 5);
        out_buf = strappend(out_buf, &out_len, &out_cap, "%s", rr.out);
        actual_exit = rr.exit_code;
        proc_free(&rr);
    }

    if (actual_exit != expected_exit) {
        print_result(base, COL_RED, "EXEC FAIL");
        failed++;
        add_row(base, "EXEC_FAIL", "non-zero exit");
        print_change(base, "EXEC_FAIL");
        emit_backtrace(tmp_exe, args, src_path, RCC, RCCFLAGS, NULL, &out_buf, &out_len, &out_cap);
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
                char sp[512];
                snprintf(sp, sizeof(sp), "%s/test/%s.out", SCRIPT_DIR, base);
                FILE *sf = fopen(sp, "w");
                if (sf) {
                    if (out_buf) fputs(out_buf, sf);
                    fclose(sf);
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

static void run_unit_tests(void) {
    static char unit_path[512];
    snprintf(unit_path, sizeof(unit_path), "%s/test", SCRIPT_DIR);
    if (!file_exists(unit_path)) return;
    printf("\n\033[0;36mUnit tests (test/)\033[0m\n");

    struct dirent **nl;
    int n = scandir(unit_path, &nl, NULL, alphasort);
    if (n < 0) return;

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

        if (ONLY_TEST && !streq(base, ONLY_TEST)) {
            free(nl[i]);
            continue;
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

        /* test_err: expect compile failure */
        if (streq(base, "test_err")) {
            char tmp[256];
            snprintf(tmp, sizeof(tmp), "/tmp/rcc_test_%d", getpid());
            if (is_mingw_native || (has_runner && contains(runner_cmd, "wine")))
                strcat(tmp, ".exe");
            char *ca[] = {(char *)RCC, (char *)RCCFLAGS, "-o", tmp, src_path, NULL};
            ProcResult cr = proc_run(ca, 30, 1);
            if (cr.exit_code == 0) {
                print_result(base, COL_RED, "SHOULD FAIL");
                failed++;
                add_row(base, "FAIL", "expected compile error but succeeded");
                print_change(base, "FAIL");
                unlink(tmp);
            } else {
                print_result(base, COL_GREEN, "PASS (compile error)");
                passed++;
                add_row(base, "PASS", "compile error as expected");
                print_change(base, "PASS");
            }
            proc_free(&cr);
            free(nl[i]);
            continue;
        }

        int expected_exit = test_unit_expected_exit(base);
        char tmp[256];
        snprintf(tmp, sizeof(tmp), "/tmp/rcc_test_%d", getpid());
        if (is_mingw_native || (has_runner && contains(runner_cmd, "wine")))
            strcat(tmp, ".exe");
        {
            char *ca[] = {(char *)RCC, (char *)RCCFLAGS, "-o", tmp, src_path, NULL};
            ProcResult cr = proc_run(ca, 30, 1);
            if (cr.exit_code != 0 || access(tmp, X_OK) != 0) {
                print_result(base, COL_RED, "COMPILE FAIL");
                failed++;
                add_row(base, "COMPILE_FAIL", cr.exit_code != 0 ? "rcc returned non-zero" : "executable missing");
                print_change(base, "COMPILE_FAIL");
                if (cr.out && cr.out[0]) printf("%s", cr.out);
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
            unlink(tmp);
            free(nl[i]);
            continue;
        }

        ProcResult rr = run_exe(tmp, "", 5);
        int ae = rr.exit_code;
        proc_free(&rr);
        unlink(tmp);
        if (ae != expected_exit) {
            print_result(base, COL_RED, "EXEC FAIL");
            failed++;
            add_row(base, "EXEC_FAIL", "");
            free(report_rows[nrows - 1].message);
            char msg[64];
            snprintf(msg, sizeof(msg), "exit=%d", ae);
            report_rows[nrows - 1].message = strdup(msg);
            print_change(base, "EXEC_FAIL");
        } else {
            print_result(base, COL_GREEN, "PASS");
            passed++;
            add_row(base, "PASS", "");
            print_change(base, "PASS");
        }
        free(nl[i]);
    }
    free(nl);
}

/* ── markdown report ─────────────────────────────────────────────── */

static void generate_tcc_report(void) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", SCRIPT_DIR, REPORT_FILE);
    FILE *rf = fopen(path, "w");
    if (!rf) return;

    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char date_buf[64];
    strftime(date_buf, sizeof(date_buf), "%B %Y", tm);
    int pct = total > 0 ? passed * 100 / total : 0;

    fprintf(rf, "# TCC Test Suite Report for RCC\n\nGenerated: %s\n\n", date_buf);
    fprintf(rf, "## Summary\n\n- **Total**: %d\n- **Passed**: %d\n- **Failed**: %d\n- **Pass Rate**: %d%%\n\n",
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

    if (!file_exists("tinycc"))
        system("git submodule update --init --recursive tinycc 2>/dev/null");
    if (!file_exists("tcc_tests") && file_exists("tinycc/tests/tests2"))
        symlink("tinycc/tests/tests2", "tcc_tests");

    TEST_DIR = "tinycc/tests/tests2";
    if (!file_exists(TEST_DIR)) {
        fprintf(stderr, "TCC test directory not found: %s\n", TEST_DIR);
        return 1;
    }

    static char rf_buf[256];
    if (streq(PLATFORM, "arm64_cross")) snprintf(rf_buf, sizeof(rf_buf), "test/tcc_test_arm64_cross.md");
    else if (streq(PLATFORM, "darwin_cross"))
        snprintf(rf_buf, sizeof(rf_buf), "test/tcc_test_darwin_cross.md");
    else if (streq(PLATFORM, "mingw_cross"))
        snprintf(rf_buf, sizeof(rf_buf), "test/tcc_test_mingw_cross.md");
    else if (streq(PLATFORM, "mingw"))
        snprintf(rf_buf, sizeof(rf_buf), "test/tcc_test_mingw.md");
    else if (streq(PLATFORM, "arm64"))
        snprintf(rf_buf, sizeof(rf_buf), "test/tcc_test_arm64.md");
    else
        snprintf(rf_buf, sizeof(rf_buf), "test/tcc_test_linux.md");
    REPORT_FILE = rf_buf;

    load_old_states(REPORT_FILE);
    bool is_mingw = is_mingw_native || contains(RCC, "mingw-cross");

    printf("\033[0;36mTCC compatibility tests\033[0m\n");

    char **files = list_c_files_sorted(TEST_DIR);
    const char *p_src = NULL;
    if (files) {
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

            if (ONLY_TEST && !streq(base, ONLY_TEST)) {
                p_src = NULL;
                continue;
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
        for (char **f = files; *f; f++) free(*f);
        free(files);
    }
    if (p_src) free((void *)p_src);

    if (!ONLY_TEST) {
        char sp[256];
        snprintf(sp, sizeof(sp), "test-tcc-%s.summary", PLATFORM);
        FILE *sf = fopen(sp, "w");
        if (sf) {
            fprintf(sf, "SUITE=tcc\nTOTAL=%d\nPASS=%d\nFAIL=%d\n", total, passed, failed);
            fclose(sf);
        }
    }

    int pct = total > 0 ? passed * 100 / total : 0;
    printf("\n\033[0;36mTCC Results: %d/%d passed (%d%%), %d failed.\033[0m\n",
           passed, total, pct, failed);
    if (regressions + fixes + changed_cnt > 0) {
        printf("Changes vs previous:");
        if (regressions) printf("  \033[0;31m%d regression(s)\033[0m", regressions);
        if (fixes) printf("  \033[0;32m%d fixed\033[0m", fixes);
        if (changed_cnt) printf("  \033[0;33m%d changed\033[0m", changed_cnt);
        printf("\n");
    }

    if (!ONLY_TEST) generate_tcc_report();
    return failed > 0 ? 1 : 0;
}

/* ═══════════════════════════════════════════════════════════════════
 * TORTURE TEST SUITE
 * ═══════════════════════════════════════════════════════════════════ */

typedef enum {
    SKIP_NONE,
    SKIP_X86_ONLY,
    SKIP_TMPNAM,
    SKIP_COMPLEX,
    SKIP_TRAMPOLINES,
    SKIP_SCALAR_STORAGE,
    SKIP_FINSTRUMENT,
    SKIP_NESTED,
    SKIP_NOT_IMPL,
    SKIP_VECTOR,
    SKIP_MODE,
    SKIP_MISSING_INCLUDE,
} SkipReason;

static const char *skip_reason_str(SkipReason r) {
    switch (r) {
    case SKIP_X86_ONLY: return "x86-only";
    case SKIP_TMPNAM: return "tmpnam-macOS";
    case SKIP_COMPLEX: return "complex";
    case SKIP_TRAMPOLINES: return "trampolines";
    case SKIP_SCALAR_STORAGE: return "scalar_storage_order";
    case SKIP_FINSTRUMENT: return "finstrument";
    case SKIP_NESTED: return "nested-func";
    case SKIP_NOT_IMPL: return "not-implemented";
    case SKIP_VECTOR: return "vector_size";
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
    if (streq(PLATFORM, "arm64") && contains(content, "#include \"gcc_tmpnam.h\""))
        return SKIP_TMPNAM;
    if (contains(content, "__complex__") || contains(content, "Complex"))
        return SKIP_COMPLEX;
    if (contains(content, "dg-require-effective-target trampolines"))
        return SKIP_TRAMPOLINES;
    if (contains(content, "scalar_storage_order")) return SKIP_SCALAR_STORAGE;
    if (contains(content, "dg-options") && contains(content, "-finstrument-functions"))
        return SKIP_FINSTRUMENT;
    if (contains(content, "dg-require-effective-target nested") ||
        streq(name, "20061220-1") || streq(name, "nest-align-1") || streq(name, "920415-1") ||
        streq(name, "pr22061-3") || streq(name, "pr22061-4") || streq(name, "pr51447") || streq(name, "pr71494"))
        return SKIP_NESTED;
    if (streq(name, "pr70460") || streq(name, "pr41935")) return SKIP_NOT_IMPL;
    if (contains(content, "vector_size") || streq(name, "pr71626-2")) return SKIP_VECTOR;
    if (contains(content, "__attribute__((mode")) return SKIP_MODE;
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
    *list = realloc(*list, need);
    strcat(*list, " ");
    strcat(*list, name);
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

    char exe_path[512];
    snprintf(exe_path, sizeof(exe_path), "/tmp/torture_rcc_%s", name);
    if (is_mingw_native || (has_runner && contains(runner_cmd, "wine")))
        strcat(exe_path, ".exe");
    char *ca[16];
    int ai = 0;
    ca[ai++] = (char *)RCC;
    ca[ai++] = (char *)RCCFLAGS;
    ca[ai++] = "-I";
    ca[ai++] = ".";
    ca[ai++] = "-o";
    ca[ai++] = exe_path;
    ca[ai++] = (char *)src;
    ca[ai++] = "-lm";
    ca[ai] = NULL;
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
                if (cr.out && cr.out[0]) printf("%s", cr.out);
            }
        }
        proc_free(&cr);
        free(content);
        return;
    }
    proc_free(&cr);

    if (streq(PLATFORM, "darwin_cross")) {
        g_tort_pass++;
        if (!summary_only) print_result(name, COL_GREEN, "PASS (compile only)");
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
    ProcResult rr = proc_run(ra, has_runner ? 20 : 5, 0);
    if (rr.exit_code != 0) {
        g_tort_fail_runtime++;
        tort_add_error(&g_tort_runtime_errors, name);
        if (!summary_only) {
            print_result(name, COL_RED, "FAIL (runtime)");
            if (!has_runner) {
                static const char *tort_extra[] = {"-I", ".", "-lm", NULL};
                emit_backtrace(exe_path, NULL, src, RCC, RCCFLAGS,
                               tort_extra, NULL, NULL, NULL);
            }
        }
    } else {
        g_tort_pass++;
        if (!summary_only) print_result(name, COL_GREEN, "PASS");
    }
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
    if (!ONLY_TEST && !summary_only) {
        char lp[PATH_MAX];
        snprintf(lp, sizeof(lp), "%s/test/torture_report_%s.log", SCRIPT_DIR, PLATFORM);
        g_log_fp = fopen(lp, "w");
    }

    logprintf("\n\033[0;36mGCC torture tests\033[0m\n");

    char save_cwd[PATH_MAX];
    getcwd(save_cwd, sizeof(save_cwd));
    if (chdir(tort_dir) != 0) {
        perror("chdir torture");
        return 1;
    }

    if (ONLY_TEST) {
        char sp[512];
        if (strstr(ONLY_TEST, ".c")) snprintf(sp, sizeof(sp), "%s", ONLY_TEST);
        else
            snprintf(sp, sizeof(sp), "%s.c", ONLY_TEST);
        run_torture_test(sp, summary_only);
    } else {
        char **files = list_c_files_sorted(".");
        if (files) {
            for (char **f = files; *f; f++) {
                run_torture_test(*f, summary_only);
                free(*f);
            }
            free(files);
        }
    }

    chdir(save_cwd);

    if (!ONLY_TEST) {
        if (!summary_only) logprintf("\n");
        logprintf("=== TOTAL=%d PASS=%d FAIL_COMPILE=%d FAIL_RUNTIME=%d SKIP=%d ===\n",
                  g_tort_total, g_tort_pass, g_tort_fail_compile, g_tort_fail_runtime, g_tort_skip);
        int eff = g_tort_total - g_tort_skip;
        logprintf("Pass rate (excl. skip): %d%%\n", eff > 0 ? g_tort_pass * 100 / eff : 0);
        if (g_tort_compile_errors && *g_tort_compile_errors)
            logprintf("\nCompile failures: %s\n", g_tort_compile_errors);
        if (g_tort_runtime_errors && *g_tort_runtime_errors)
            logprintf("\nRuntime failures: %s\n", g_tort_runtime_errors);

        char sp[256];
        snprintf(sp, sizeof(sp), "test-torture-%s.summary", PLATFORM);
        FILE *sf = fopen(sp, "w");
        if (sf) {
            fprintf(sf, "SUITE=torture\nTOTAL=%d\nPASS=%d\nFAIL=%d\nFAIL_COMPILE=%d\nFAIL_RUNTIME=%d\nSKIP=%d\n",
                    g_tort_total, g_tort_pass, g_tort_fail_compile + g_tort_fail_runtime,
                    g_tort_fail_compile, g_tort_fail_runtime, g_tort_skip);
            fclose(sf);
        }
    }

    int fail = g_tort_fail_compile + g_tort_fail_runtime;
    int max_fail;
    if (ONLY_TEST) max_fail = 1;
    else if (streq(PLATFORM, "arm64_cross"))
        max_fail = 28;
    else if (streq(PLATFORM, "arm64"))
        max_fail = 27;
    else if (streq(PLATFORM, "mingw_cross"))
        max_fail = 63;
    else if (streq(PLATFORM, "darwin_cross"))
        max_fail = 2;
    else if (streq(PLATFORM, "mingw"))
        max_fail = 83;
    else
        max_fail = 15;

    if (g_log_fp) {
        fclose(g_log_fp);
        g_log_fp = NULL;
    }
    return fail > max_fail ? 1 : 0;
}

/* ═══════════════════════════════════════════════════════════════════
 * COMPLIANCE TEST SUITE  (gcc vs rcc output comparison, from ncc)
 * ═══════════════════════════════════════════════════════════════════ */

static int run_compliance_suite(void) {
    char comp_dir[PATH_MAX];
    snprintf(comp_dir, sizeof(comp_dir), "%s/test/compliance", SCRIPT_DIR);
    if (!file_exists(comp_dir)) {
        printf("\nNCC Compliance directory not found: %s\n", comp_dir);
        return 0;
    }

    printf("\n\033[0;36mNCC Compliance tests (test/compliance/)\033[0m\n");

    /* locate gcc */
    static const char *gcc_cands[] = {"/usr/bin/gcc", "/usr/local/bin/gcc", NULL};
    const char *gcc_path = NULL;
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

    int comp_pass = 0, comp_fail = 0, comp_skip = 0;
    struct dirent **nl;
    int n = scandir(comp_dir, &nl, NULL, alphasort);
    if (n < 0) return 0;

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

        if (ONLY_TEST && !streq(base, ONLY_TEST)) {
            free(nl[i]);
            continue;
        }

        char src_path[PATH_MAX + NAME_MAX + 2];
        snprintf(src_path, sizeof(src_path), "%s/%s", comp_dir, fname);

        if (!gcc_path) {
            print_result(base, COL_YELLOW, "SKIP (gcc not found)");
            comp_skip++;
            free(nl[i]);
            continue;
        }

        char gcc_exe[PATH_MAX], rcc_exe[PATH_MAX];
        snprintf(gcc_exe, sizeof(gcc_exe), "/tmp/compliance_gcc_%d_%s", getpid(), base);
        snprintf(rcc_exe, sizeof(rcc_exe), "/tmp/compliance_rcc_%d_%s", getpid(), base);

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
            char *ca[] = {(char *)RCC, "-o", rcc_exe, src_path, NULL};
            ProcResult cr = proc_run(ca, 30, 1);
            if (cr.exit_code != 0) {
                print_result(base, COL_RED, "FAIL (rcc compile)");
                if (cr.out && cr.out[0]) printf("    %s\n", cr.out);
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
        ProcResult rr = proc_run(ra, 5, 0);

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
        proc_free(&gr);
        proc_free(&rr);
        unlink(gcc_exe);
        unlink(rcc_exe);
        free(nl[i]);
    }
    free(nl);

    int comp_total = comp_pass + comp_fail;
    printf("\nCompliance: %d/%d passed", comp_pass, comp_total);
    if (comp_skip) printf(", %d skipped", comp_skip);
    printf("\n");

    if (!ONLY_TEST) {
        char sp[256];
        snprintf(sp, sizeof(sp), "test-compliance-%s.summary", PLATFORM);
        FILE *sf = fopen(sp, "w");
        if (sf) {
            fprintf(sf, "SUITE=compliance\nTOTAL=%d\nPASS=%d\nFAIL=%d\n", comp_total, comp_pass, comp_fail);
            fclose(sf);
        }
    }
    return comp_fail > 0 ? 1 : 0;
}

/* ═══════════════════════════════════════════════════════════════════
 * C-TESTSUITE https://github.com/c-testsuite/c-testsuite
 * ═══════════════════════════════════════════════════════════════════ */

static int run_ctest_suite(void) {
    char ctest_dir[PATH_MAX];
    snprintf(ctest_dir, sizeof(ctest_dir), "%s/c-testsuite", SCRIPT_DIR);

    if (!file_exists(ctest_dir)) {
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "git -C '%s' submodule update --init --recursive c-testsuite 2>/dev/null", SCRIPT_DIR);
        system(cmd);
    }

    char single_exec[PATH_MAX + 32];
    snprintf(single_exec, sizeof(single_exec), "%s/single-exec", ctest_dir);
    if (!file_exists(single_exec)) {
        printf("\nc-testsuite not found, skipping\n");
        return 0;
    }

    /* clean stale state left by a previous test run */
    char fred[PATH_MAX + 32];
    snprintf(fred, sizeof(fred), "%s/fred.txt", ctest_dir);
    if (file_exists(fred)) {
        char cmd[PATH_MAX + 64];
        snprintf(cmd, sizeof(cmd), "git -C '%s' clean -dxf . 2>/dev/null", ctest_dir);
        system(cmd);
    }

    printf("\n\033[0;36mC-testsuite\033[0m\n");
    printf("Start c-testsuite with %s -O1 -lm\n", RCC);

    /* run: CC=<rcc> CFLAGS="-O1 -lm" ./single-exec posix — parse TAP directly */
    char cmd[PATH_MAX * 2 + 128];
    /* stdbuf -oL forces line-buffered stdout in the subprocess so results
     * arrive immediately rather than in 4K chunks */
    const char *stdbuf = access("/usr/bin/stdbuf", X_OK) == 0 ? "stdbuf -oL " : "";
    snprintf(cmd, sizeof(cmd),
             "cd '%s' && env CC='%s' CFLAGS='-O1 -lm' %s./single-exec posix 2>/dev/null",
             ctest_dir, RCC, stdbuf);

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        fprintf(stderr, "c-testsuite: failed to run\n");
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
            }
            fflush(stdout);
        } else if (strncmp(line, "1..", 3) == 0) {
            sscanf(line + 3, "%d", &ctest_total);
        }
    }
    pclose(fp);

    printf("\n");
    printf("  pass  %d\n", ctest_pass);
    printf("  skip  %d\n", ctest_skip);
    if (ctest_fail > 0)
        printf("  " COL_RED "fail  %d" COL_RESET "\n", ctest_fail);
    else
        printf("  fail  %d\n", ctest_fail);
    printf("  total %d\n", ctest_total);

    if (ctest_fail > 0)
        printf("\nFAIL: got %d failures, maximum allowed is 0\n", ctest_fail);
    else
        printf("\nOK: %d failures, within limit of 0\n", ctest_fail);

    if (!ONLY_TEST) {
        char sp[256];
        snprintf(sp, sizeof(sp), "test-ctest-%s.summary", PLATFORM);
        FILE *sf = fopen(sp, "w");
        if (sf) {
            fprintf(sf, "SUITE=c-testsuite\nTOTAL=%d\nPASS=%d\nFAIL=%d\nSKIP=%d\n",
                    ctest_total, ctest_pass, ctest_fail, ctest_skip);
            fclose(sf);
        }
    }
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
        {"tcc", "TCC Compatibility Tests + Unit tests"},
        {"units", "RCC Unit tests"},
        {"ctest", "c-testsuite"},
        {"compliance", "NCC Compliance Tests (vs GCC)"},
        {"torture", "GCC Torture Tests"},
    };
    static const int NSUITE = 5;

    const char *desc =
        streq(PLATFORM, "linux") ? "Linux x86_64" : streq(PLATFORM, "mingw_cross") ? "Windows x86_64 (mingw cross)"
        : streq(PLATFORM, "arm64_cross")                                           ? "Linux ARM64 (aarch64 cross)"
        : streq(PLATFORM, "darwin_cross")                                          ? "macOS ARM64 (darwin cross, compile+link only)"
        : streq(PLATFORM, "arm64")                                                 ? "macOS ARM64 (native)"
        : streq(PLATFORM, "mingw")                                                 ? "Windows x86_64 (native)"
                                                                                   : PLATFORM;

    char report_path[PATH_MAX];
    snprintf(report_path, sizeof(report_path), "%s/test_report_%s.md", SCRIPT_DIR, PLATFORM);

    struct {
        int total, pass, fail, skip, fail_compile, fail_runtime;
        bool found;
    } s[4] = {0};

    for (int i = 0; i < NSUITE; i++) {
        char sp[PATH_MAX];
        snprintf(sp, sizeof(sp), "%s/test-%s-%s.summary", SCRIPT_DIR, suite_meta[i].id, PLATFORM);
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
    for (int i = 0; i < NSUITE; i++) {
        if (!s[i].found) continue;
        ov_total += s[i].total;
        ov_pass += s[i].pass;
        ov_fail += s[i].fail;
    }

    FILE *rf = fopen(report_path, "w");
    if (!rf) {
        perror(report_path);
        return;
    }

    time_t t = time(NULL);
    char datebuf[64];
    strftime(datebuf, sizeof(datebuf), "%B %d %Y %H:%M", localtime(&t));

    fprintf(rf, "# RCC Test Suite Report\n\n");
    fprintf(rf, "**Platform**: %s\n\n", desc);
    fprintf(rf, "Generated: %s\n\n", datebuf);

    fprintf(rf, "## Overall Summary\n\n");
    fprintf(rf, "- **Total**: %d\n", ov_total);
    fprintf(rf, "- **Passed**: %d\n", ov_pass);
    fprintf(rf, "- **Failed**: %d\n", ov_fail);
    if (ov_total > 0)
        fprintf(rf, "- **Overall Pass Rate**: %d%%\n", ov_pass * 100 / ov_total);

    for (int i = 0; i < NSUITE; i++) {
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
    printf("Unified report saved to test_report_%s.md\n", PLATFORM);
}

/* ═══════════════════════════════════════════════════════════════════
 * MAIN
 * ═══════════════════════════════════════════════════════════════════ */

int main(int argc, char **argv) {
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

    RCCFLAGS = "-O1";
    bool run_tcc = false, run_units = false, run_torture = false;
    bool run_compliance = false, run_ctest = false;
    bool summary_only = false;
    RCC = NULL;
    ONLY_TEST = NULL;

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
        else if (streq(a, "--help") || streq(a, "-h")) {
            printf("Usage: ./run_tests [rcc-binary] [options] [test-name]\n\n");
            printf("Options (default: --tcc --unit-tests --compliance --ctest):\n");
            printf("  --tcc         TCC compatibility tests (tinycc/tests/tests2/)\n");
            printf("  --unit-tests  RCC Unit tests (test/test_*.c)\n");
            printf("  --torture     GCC torture tests (test/torture/)\n");
            printf("  --compliance  NCC Compliance tests (gcc vs rcc output comparison)\n");
            printf("  --ctest       C-testsuite (posix single-exec suite)\n");
            printf("  --all         All test suites\n");
            printf("  --summary     Torture summary-only (no per-test output)\n\n");
            printf("rcc-binary  Path to rcc binary (auto-detected if not given)\n");
            printf("test-name   Run only this one test\n");
            return 0;
        }
        /* first non-flag arg that looks like an executable path is the rcc binary;
           otherwise treat it as a test-name filter (rcc is auto-detected) */
        else if (!RCC && (access(a, X_OK) == 0 || strchr(a, '/') != NULL || strchr(a, '\\') != NULL))
            RCC = a;
        else if (!ONLY_TEST)
            ONLY_TEST = a;
    }

    /* auto-detect rcc */
    if (!RCC) {
        if (access("./rcc", X_OK) == 0) RCC = "./rcc";
        else if (access("./rcc.exe", X_OK) == 0)
            RCC = "./mingw-cross.sh";
    }
    if (!RCC) {
        fprintf(stderr, "rcc binary not found\n");
        return 1;
    }

    /* resolve to absolute path */
    {
        char resolved[PATH_MAX];
        static char rcc_buf[PATH_MAX];
        if (realpath(RCC, resolved)) {
            memcpy(rcc_buf, resolved, strlen(resolved) + 1);
            RCC = rcc_buf;
        }
    }

    /* rewrite shorthand cross-compiler paths */
    if (contains(RCC, "rcc-arm64") && !contains(RCC, "arm64-cross")) {
        static char buf[PATH_MAX + 32];
        snprintf(buf, sizeof(buf), "%s/arm64-cross.sh", SCRIPT_DIR);
        RCC = buf;
    }
    if (contains(RCC, "rcc-darwin") && !contains(RCC, "darwin-cross")) {
        static char buf[PATH_MAX + 32];
        snprintf(buf, sizeof(buf), "%s/darwin-cross.sh", SCRIPT_DIR);
        RCC = buf;
    }

    detect_platform(RCC);

    /* suppress wine fixme noise so it doesn't pollute captured test output */
    if (has_runner && contains(runner_cmd, "wine"))
        setenv("WINEDEBUG", "fixme-all", 0);

    /* for cross-compilers, suites that require native execution don't apply */
    if (!run_tcc && !run_units && !run_torture && !run_compliance && !run_ctest) {
        if (has_runner || streq(PLATFORM, "darwin_cross")) {
            run_tcc = run_torture = true;
        } else {
            run_tcc = run_units = run_compliance = run_ctest = true;
        }
    }

    int exit_code = 0;
    if (run_tcc && run_tcc_suite() != 0) exit_code = 1;
    if (run_units) run_unit_tests();
    if (run_compliance && run_compliance_suite() != 0) exit_code = 1;
    if (run_ctest && run_ctest_suite() != 0) exit_code = 1;
    if (run_torture && run_torture_suite(summary_only) != 0) exit_code = 1;

    if (!ONLY_TEST) generate_report();

    return exit_code;
}
