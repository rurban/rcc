// volatile accesses in the -O2 IR backend (ir_build.c models them as volatile
// LOAD/STOREs; ir_opt.c / ir_x86.c must keep each access exactly once, in order,
// at its declared width). Read a cell twice, unused reads, loop conditions,
// store sequences, volatile pointers to globals / locals / struct members /
// bitfields, compound assignment and ++/--, reads across an inlined callee and
// in a tail-recursive function. The values below were checked against gcc; they
// catch a wrong result, but a plain load gives the same single-threaded value as
// a volatile one, so the second half observes the accesses themselves (POSIX only,
// no signal handlers): forked children run each probe against PROT_NONE / PROT_READ
// pages, where an access that must happen (an unused read, a read-modify-write
// store) faults and one that must stay inside its declared width (next to a
// guard page) does not, and a sibling process flips a shared flag that polling
// loops have to re-read. Each check returns a distinct nonzero code on mismatch.
#include <stdio.h>
#include <string.h>

#if (defined(__unix__) || defined(__APPLE__)) && !defined(__wasm__)
#define HAVE_PROCS 1
#include <signal.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif
#endif

#define NOINL __attribute__((noinline))

volatile int g1, g2;
volatile unsigned char gb;
volatile short gs;
volatile long long gll;
volatile float gf;
volatile double gd;
volatile int arr[8];
int plainv;

struct Dev {
    volatile unsigned status;
    volatile unsigned data;
    volatile unsigned char flags[4];
    int plainmem;
};
struct Dev dev;
volatile struct Dev vdev;

struct BF {
    volatile unsigned a : 3;
    unsigned b : 5;
    volatile unsigned c : 8;
    int d;
};
struct BF gbf;
volatile struct BF vgbf;

// ---- the "device": a cell that changes between two volatile reads
NOINL static void tick_by(volatile int *p, int n) { *p += n; }

// 1. two reads of the same cell with a device update in between
NOINL static int read_twice_ptr(volatile int *p) {
    int a = *p;
    tick_by(p, 10);
    int b = *p;
    return a * 100 + b;
}
// 2. repeated reads without anything in between still yield the same cell value
NOINL static int read_twice(void) {
    int a = g1;
    int b = g1;
    return a * 3 + b;
}
// 3. a read whose result is unused must not change anything either way
NOINL static int unused_reads(volatile int *p) {
    (void)g1;
    g2;
    *p;
    (void)*p;
    return 1;
}
// 4. volatile read in a loop condition
NOINL static int cond_loop(void) {
    int n = 0;
    while (g1 < 5) {
        n++;
        g1 = g1 + 1;
    }
    return n;
}
// 5. store sequences, final value and ordering against a read
NOINL static int store_seq(void) {
    g1 = 1;
    g1 = 2;
    g1 = 3;
    return g1;
}
NOINL static void store_seq_ptr(volatile int *p) {
    *p = 1;
    *p = 2;
    *p = 3;
}
// 6. volatile through a struct member, global and via pointer
NOINL static int member_ptr(struct Dev *d) {
    d->status = 1;
    d->data = d->status + 2;
    return (int)d->data;
}
NOINL static int member_global(void) {
    dev.status = 4;
    dev.data = dev.status << 1;
    dev.flags[2] = 9;
    return (int)dev.data + dev.flags[2];
}
NOINL static int volatile_struct(void) {
    vdev.status = 7;
    vdev.plainmem = 8;
    int a = vdev.plainmem;
    int b = vdev.plainmem;
    return (int)vdev.status + a + b;
}
// 7. compound assignment and ++/-- (one read, one write each)
NOINL static int compound(void) {
    g1 = 3;
    g1 += 4;
    g1 *= 2;
    g1 -= 1;
    g1 <<= 1;
    g1 ^= 5;
    return g1;
}
NOINL static int incdec(void) {
    g1 = 0;
    g1++;
    ++g1;
    g1--;
    int a = g1++;
    int b = ++g1;
    return a * 10 + b + g1;
}
NOINL static int incdec_ptr(volatile int *p) {
    (*p)++;
    ++*p;
    return (*p)--;
}
// 8. every width / kind
NOINL static int widths(void) {
    gb = 200;
    gs = -3;
    gll = 1LL << 40;
    gf = 1.5f;
    gd = 2.25;
    return gb + gs + (int)(gll >> 38) + (int)gf + (int)gd;
}
NOINL static int array_cells(int i) {
    arr[i & 7] = i;
    arr[(i + 1) & 7] = arr[i & 7] + 1;
    return arr[i & 7] + arr[(i + 1) & 7];
}
// narrowing conversions of a volatile read must still read the whole object
NOINL static int narrow_sext(volatile int *p) { return (signed char)*p; }
NOINL static int narrow_zext(volatile unsigned *p) { return (unsigned char)*p; }
NOINL static int narrow_trunc(volatile long long *p) { return (int)*p; }
NOINL static int fold_add(volatile int *p, int x) { return x + *p; }
NOINL static int fold_cmp(volatile int *p, int x) { return x < *p; }
// 9. pointer-to-volatile held in a volatile pointer, pointer to pointer
volatile int *volatile vpp;
volatile int *vp0;
NOINL static int ptr_to_ptr(void) {
    vpp = &g1;
    *vpp = 41;
    return *vpp + 1;
}
NOINL static int ptrptr(volatile int **pp) { return **pp + **pp; }
// 10. plain object mixed with volatile ones
NOINL static int plain_mix(int *q) {
    plainv = 1;
    int a = plainv;
    g1 = 2;
    plainv = 3;
    int b = plainv;
    *q = 4;
    return a + b + g1 + plainv;
}
NOINL static int via_volatile_cast(void) {
    g1 = 12;
    return *(volatile int *)&g1 + 1;
}
// 11. bitfields: unit read-modify-write, neighbours preserved
NOINL static int bitfields(void) {
    gbf.b = 5;
    gbf.a = 3;
    gbf.c = 200;
    gbf.d = 9;
    int r = (int)gbf.a + (int)gbf.b + (int)gbf.c + gbf.d;
    gbf.a += 2;
    gbf.c++;
    return r + (int)gbf.a + (int)gbf.c + (int)gbf.b;
}
NOINL static int volatile_bitfields(void) {
    vgbf.b = 7;
    vgbf.a = 5;
    vgbf.c = 99;
    return (int)vgbf.a + (int)vgbf.b + (int)vgbf.c;
}
NOINL static int bitfield_ptr(struct BF *p) {
    p->a = 1;
    p->c = p->a + 4;
    return (int)p->c;
}
NOINL static int bitfield_reads(void) {
    int x = (int)gbf.a;
    int y = (int)gbf.c;
    return x + y;
}
// 12. reads across an inlined callee (the IR inliner copies the volatile loads)
static int rd2(volatile int *p) {
    int a = *p;
    int b = *p;
    if (a > 1000) return 0;
    return a + b;
}
static int poll(volatile int *p, int n) {
    int s = 0;
    for (int i = 0; i < n; i++) s += *p;
    return s;
}
static void wr3(volatile int *p) {
    *p = 1;
    *p = 2;
    *p = 3;
}
static int pickv(void) { return g2; }
NOINL static int inlined_callees(void) {
    g1 = 6;
    wr3(&g1);
    return rd2(&g1) * 100 + poll(&g1, 3);
}
NOINL static int inlined_unused(void) {
    g2 = 3;
    pickv();
    return pickv() + 1;
}
// 13. tail recursion: the loop keeps one volatile access per iteration
NOINL static int tail_store(volatile int *p, int n) {
    if (n == 0) return *p;
    *p = n;
    return tail_store(p, n - 1);
}
NOINL static int tail_acc(int n, int acc) {
    if (n == 0) return acc;
    return tail_acc(n - 1, acc + g1);
}
// 14. loops: invariant volatile read stays in the loop; read + write through a pointer
NOINL static int loop_invariant(int n) {
    int s = 0;
    for (int i = 0; i < n; i++) s += g1;
    return s;
}
NOINL static int loop_ptr(volatile int *p, int n) {
    int s = 0;
    for (int i = 0; i < n; i++) {
        s += *p;
        *p = i;
    }
    return s;
}
NOINL static int loop_device(volatile int *p, int n) {
    int s = 0;
    for (int i = 0; i < n; i++) {
        s += *p;
        tick_by(p, 1); // the cell changes under the loop each iteration
    }
    return s;
}
NOINL static int cond_value(int c) { return c ? g1 : g2; }
NOINL static int short_circuit(int c) { return c && g1 || g2; }
NOINL static double read_fp_twice(volatile double *p) {
    double a = *p;
    double b = *p;
    return a + b;
}
NOINL static int swap_cells(volatile int *a, volatile int *b) {
    int t = *a;
    *a = *b;
    *b = t;
    return *a - *b;
}
NOINL static void incr_cell(volatile unsigned *c) {
    *c = 1;
    *c = *c + 1;
    *c = *c + 1;
}
NOINL static int sink_loop(volatile int *p, int n) {
    int s = 0;
    while (n-- > 0) {
        *p = n;
        s += *p;
    }
    return s;
}
// volatile local: kept in memory, every access stays
NOINL static int local_volatile(void) {
    volatile int v = 5;
    v = v + 1;
    v += 2;
    v++;
    ++v;
    return v;
}
NOINL static int local_unused(void) {
    volatile int v = 5;
    int x = v;
    v = 1;
    return x;
}


#ifdef HAVE_PROCS
// ---------------------------------------------------------------- process probes
// Every probe is a noinline function of one pointer; the child runs it once and the parent
// looks at how it ended. pg is [ok page][guard page]; "end" is the first byte of the guard.
struct B1 { // 4 bytes, one storage unit: its access must stay 4 bytes wide at the end of a page
    volatile unsigned a : 3;
    volatile unsigned c : 8;
};
struct Dev2 { volatile unsigned status; volatile unsigned data; };

volatile int psink; // keeps the narrowed/folded value of a probe live
#define PROBE(name, body) NOINL static void name(char *e) { body; }
// ---- must fault: the access happens even though nothing uses its result (p -> PROT_NONE page)
PROBE(pf_stmt, *(volatile int *)e)
PROBE(pf_void, (void)*(volatile int *)e)
PROBE(pf_unused_local, int x = *(volatile int *)e; (void)x)
PROBE(pf_loop, for (int i = 0; i < 3; i++) *(volatile int *)e)
PROBE(pf_member, ((volatile struct Dev2 *)e)->data)
PROBE(pf_bitfield, ((volatile struct B1 *)e)->a)
PROBE(pf_char, *(volatile char *)e)
PROBE(pf_double, *(volatile double *)e)
PROBE(pf_ll, *(volatile long long *)e)
static int rd_branchy(volatile int *p) { // inlined by the IR inliner; the read feeds only a folded branch
    int a = *p;
    if (a == 12345) return 1;
    return 0;
}
PROBE(pf_inlined_unused, rd_branchy((volatile int *)e))
NOINL static int tail_probe(volatile int *p, int n) {
    if (n == 0) return 0;
    *p;
    return tail_probe(p, n - 1);
}
PROBE(pf_tail, tail_probe((volatile int *)e, 2))
// ---- must fault: a read-modify-write / store to a read-only page
PROBE(pr_store, *(volatile int *)e = 5)
PROBE(pr_incdec, (*(volatile int *)e)++)
PROBE(pr_compound, *(volatile int *)e += 2)
PROBE(pr_store_bf, ((volatile struct B1 *)e)->c = 5)
PROBE(pr_store_member, ((volatile struct Dev2 *)e)->status = 1)
// ---- must fault: the access covers the whole object, so it reaches into the guard page
PROBE(wf_sext8, volatile int *p = (volatile int *)(e - 1); int x = (signed char)*p; psink = (int)x)
PROBE(wf_zext8, volatile unsigned *p = (volatile unsigned *)(e - 1); int x = (unsigned char)*p; psink = (int)x)
PROBE(wf_zext16, volatile unsigned *p = (volatile unsigned *)(e - 2); int x = (unsigned short)*p; psink = (int)x)
PROBE(wf_trunc, volatile long long *p = (volatile long long *)(e - 4); int x = (int)*p; psink = (int)x)
PROBE(wf_add, volatile int *p = (volatile int *)(e - 2); int x = 5 + *p; psink = (int)x)
PROBE(wf_cmp, volatile int *p = (volatile int *)(e - 2); int x = 5 < *p; psink = (int)x)
PROBE(wf_fp, volatile double *p = (volatile double *)(e - 4); double x = *p; psink = (int)x)
PROBE(wf_store, *(volatile int *)(e - 2) = 1)
// ---- must NOT fault: exactly the declared width, ending at the guard page
PROBE(wo_int, volatile int *p = (volatile int *)(e - 4); int x = *p; psink = (int)x)
PROBE(wo_ll, volatile long long *p = (volatile long long *)(e - 8); long long x = *p; psink = (int)x)
PROBE(wo_short, volatile short *p = (volatile short *)(e - 2); int x = *p; psink = (int)x)
PROBE(wo_char, volatile char *p = (volatile char *)(e - 1); int x = *p + 1; psink = (int)x)
PROBE(wo_float, volatile float *p = (volatile float *)(e - 4); float x = *p; psink = (int)x)
PROBE(wo_double, volatile double *p = (volatile double *)(e - 8); double x = *p; psink = (int)x)
PROBE(wo_sext, volatile char *p = (volatile char *)(e - 1); int x = (signed char)*p; psink = (int)x)
PROBE(wo_zext, volatile unsigned short *p = (volatile unsigned short *)(e - 2); int x = (unsigned char)*p; psink = (int)x)
PROBE(wo_store, *(volatile int *)(e - 4) = 1; *(volatile short *)(e - 2) = 2; *(volatile char *)(e - 1) = 3)
PROBE(wo_incdec, volatile char *p = (volatile char *)(e - 1); (*p)++; ++*p; *p += 2)
PROBE(wo_bitfield, volatile struct B1 *p = (volatile struct B1 *)(e - 4); p->a = 3; p->c += 1; int x = (int)p->a + (int)p->c; psink = (int)x)

typedef void (*probe_fn)(char *);

static void child_limits(void) {
    struct rlimit rl = {0, 0};
    setrlimit(RLIMIT_CORE, &rl); // a faulting probe must not leave a core file
    alarm(20); // default SIGALRM action: a hung probe kills only itself
}

// 1: the probe died of SIGSEGV/SIGBUS, 0: it ran to completion, -1: anything else
static int run_probe(probe_fn fn, char *e) {
    fflush(stdout);
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        child_limits();
        fn(e);
        _exit(0);
    }
    int st = 0;
    if (waitpid(pid, &st, 0) != pid) return -1;
    if (WIFSIGNALED(st)) return WTERMSIG(st) == SIGSEGV || WTERMSIG(st) == SIGBUS ? 1 : -1;
    return WIFEXITED(st) && WEXITSTATUS(st) == 0 ? 0 : -1;
}

typedef struct {
    probe_fn fn;
    const char *name;
    int page; // 0: ok page then PROT_NONE page; 1: the same, the ok page is read-only; 2: probe address is inside a PROT_NONE page
    int fault;
} Probe;
#define P(f, pg, fl) {f, #f, pg, fl}
static const Probe probes[] = {
    P(pf_stmt, 2, 1), P(pf_void, 2, 1), P(pf_unused_local, 2, 1), P(pf_loop, 2, 1), P(pf_member, 2, 1),
    P(pf_bitfield, 2, 1), P(pf_char, 2, 1), P(pf_double, 2, 1), P(pf_ll, 2, 1), P(pf_inlined_unused, 2, 1),
    P(pf_tail, 2, 1),
    P(pr_store, 1, 1), P(pr_incdec, 1, 1), P(pr_compound, 1, 1), P(pr_store_bf, 1, 1), P(pr_store_member, 1, 1),
    P(wf_sext8, 0, 1), P(wf_zext8, 0, 1), P(wf_zext16, 0, 1), P(wf_trunc, 0, 1), P(wf_add, 0, 1), P(wf_cmp, 0, 1),
    P(wf_fp, 0, 1), P(wf_store, 0, 1),
    P(wo_int, 0, 0), P(wo_ll, 0, 0), P(wo_short, 0, 0), P(wo_char, 0, 0), P(wo_float, 0, 0), P(wo_double, 0, 0),
    P(wo_sext, 0, 0), P(wo_zext, 0, 0), P(wo_store, 0, 0), P(wo_incdec, 0, 0), P(wo_bitfield, 0, 0),
};

static int run_probes(void) {
    long pgsz = sysconf(_SC_PAGESIZE);
    char *base = mmap(NULL, (size_t)pgsz * 2, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED) return 0; // cannot set up the guard pages: nothing to check here
    if (mprotect(base + pgsz, (size_t)pgsz, PROT_NONE) != 0) return 0;
    char *guard = base + pgsz;
    for (unsigned k = 0; k < sizeof probes / sizeof *probes; k++) {
        const Probe *pr = &probes[k];
        mprotect(base, (size_t)pgsz, pr->page == 1 ? PROT_READ : PROT_READ | PROT_WRITE);
        int got = run_probe(pr->fn, pr->page == 2 ? guard : guard); // e = first byte of the guard page
        if (got != pr->fault) {
            printf("FAIL probe %s: %s (want %s)\n", pr->name, got < 0 ? "bad exit" : got ? "faulted" : "did not fault",
                   pr->fault ? "fault" : "no fault");
            return 1 + (int)k;
        }
    }
    munmap(base, (size_t)pgsz * 2);
    return 0;
}

// ---------------------------------------------------------------- polling across processes
NOINL static void wait_until_set(volatile int *c) {
    while (c[0] == 0) {
    }
    c[1] = 7;
}
NOINL static void wait_do_while(volatile int *c) {
    int n = 0;
    do {
        n++;
    } while (c[0] != 5);
    c[1] = n > 0 ? 8 : 9;
}
NOINL static void wait_for_break(volatile int *c) {
    for (;;) {
        if (c[0] > 2) break;
    }
    c[1] = 10;
}
static int load_cell(volatile int *p) { return *p; }
NOINL static void wait_inlined(volatile int *c) {
    while (!load_cell(c)) {
    }
    c[1] = 11;
}
NOINL static void wait_for_struct(volatile int *c) {
    volatile struct Dev2 *d = (volatile struct Dev2 *)c;
    while (d->status != 3) {
    }
    d->data = 12;
}
NOINL static void ping_pong_child(volatile int *c) {
    for (int i = 1; i <= 25; i++) {
        while (c[0] != i) {
        }
        c[1] = i;
    }
}
typedef void (*poll_fn)(volatile int *);
// each poll function runs in a child; the parent sets c[0] (after the child had its chance to
// spin) and waits for the answer in c[1] with its own polling loop
static int run_poll(poll_fn fn, int set, int want) {
    volatile int *c = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (c == (volatile int *)MAP_FAILED) return 0;
    c[0] = c[1] = 0;
    fflush(stdout);
    pid_t pid = fork();
    if (pid < 0) return 0;
    if (pid == 0) {
        child_limits();
        fn(c);
        _exit(0);
    }
    alarm(20);
    c[0] = set;
    while (c[1] != want) {
    }
    int st = 0;
    int ok = waitpid(pid, &st, 0) == pid && WIFEXITED(st) && WEXITSTATUS(st) == 0;
    munmap((void *)c, 4096);
    return ok ? 0 : 1;
}
static int run_pingpong(void) {
    volatile int *c = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (c == (volatile int *)MAP_FAILED) return 0;
    c[0] = c[1] = 0;
    fflush(stdout);
    pid_t pid = fork();
    if (pid < 0) return 0;
    if (pid == 0) {
        child_limits();
        ping_pong_child(c);
        _exit(0);
    }
    alarm(20);
    for (int i = 1; i <= 25; i++) {
        c[0] = i;
        while (c[1] != i) {
        }
    }
    int st = 0;
    int ok = waitpid(pid, &st, 0) == pid && WIFEXITED(st) && WEXITSTATUS(st) == 0;
    munmap((void *)c, 4096);
    return ok ? 0 : 1;
}
static int run_polls(void) {
    if (run_poll(wait_until_set, 1, 7)) return 1;
    if (run_poll(wait_do_while, 5, 8)) return 2;
    if (run_poll(wait_for_break, 3, 10)) return 3;
    if (run_poll(wait_inlined, 1, 11)) return 4;
    if (run_poll(wait_for_struct, 3, 12)) return 5;
    if (run_pingpong()) return 6;
    return 0;
}
#endif // HAVE_PROCS

volatile int lcl;
volatile long long m_ll;
volatile unsigned m_u, m_c;
volatile double m_dd;
volatile int m_a, m_b;

int main(void) {
    int r[96], n = 0;
    int qq = 0;
    g1 = 7;
    g2 = 8;
    r[n++] = read_twice();
    r[n++] = read_twice_ptr(&g1);
    r[n++] = g1;
    r[n++] = unused_reads(&g2);
    g1 = 0;
    r[n++] = cond_loop();
    r[n++] = g1;
    r[n++] = store_seq();
    store_seq_ptr(&lcl);
    r[n++] = lcl;
    r[n++] = member_ptr(&dev);
    r[n++] = (int)dev.status;
    r[n++] = member_global();
    r[n++] = volatile_struct();
    r[n++] = compound();
    r[n++] = incdec();
    lcl = 3;
    r[n++] = incdec_ptr(&lcl);
    r[n++] = lcl;
    r[n++] = widths();
    for (int i = 0; i < 9; i++) r[n++] = array_cells(i);
    lcl = 0x1ff;
    r[n++] = narrow_sext(&lcl);
    m_u = 0x1ff;
    r[n++] = narrow_zext(&m_u);
    m_ll = 0x123456789LL;
    r[n++] = narrow_trunc(&m_ll);
    lcl = 9;
    r[n++] = fold_add(&lcl, 4);
    r[n++] = fold_cmp(&lcl, 4) + 10 * fold_cmp(&lcl, 30);
    r[n++] = ptr_to_ptr();
    vp0 = &g2;
    g2 = 21;
    r[n++] = ptrptr(&vp0);
    r[n++] = plain_mix(&qq);
    r[n++] = qq;
    r[n++] = via_volatile_cast();
    r[n++] = bitfields();
    r[n++] = volatile_bitfields();
    r[n++] = bitfield_ptr(&gbf);
    r[n++] = bitfield_ptr((struct BF *)&vgbf);
    gbf.a = 2;
    gbf.c = 9;
    r[n++] = bitfield_reads();
    r[n++] = inlined_callees();
    r[n++] = inlined_unused();
    lcl = 0;
    r[n++] = tail_store(&lcl, 6);
    r[n++] = lcl;
    g1 = 3;
    r[n++] = tail_acc(5, 0);
    r[n++] = loop_invariant(4);
    lcl = 5;
    r[n++] = loop_ptr(&lcl, 5);
    r[n++] = lcl;
    lcl = 10;
    r[n++] = loop_device(&lcl, 4);
    r[n++] = lcl;
    g1 = 7;
    g2 = 8;
    r[n++] = cond_value(0) + cond_value(1) * 100;
    r[n++] = short_circuit(0) + short_circuit(1) * 3;
    m_dd = 1.25;
    r[n++] = (int)(read_fp_twice(&m_dd) * 4);
    m_a = 3;
    m_b = 9;
    r[n++] = swap_cells(&m_a, &m_b);
    r[n++] = m_a * 10 + m_b;
    m_c = 0;
    incr_cell(&m_c);
    r[n++] = (int)m_c;
    lcl = 4;
    r[n++] = sink_loop(&lcl, 5);
    r[n++] = local_volatile();
    r[n++] = local_unused();

    static const int want[] = {28,717,17,1,5,5,3,3,3,1,17,23,31,16,5,4,204,1,3,5,7,9,11,13,15,17,-1,255,591751049,13,1,42,42,9,4,13,428,111,5,5,11,609,4,1,1,15,12,11,4,46,14,708,4,10,6,93,3,10,10,5};
    int nwant = (int)(sizeof want / sizeof want[0]);
    if (n != nwant) {
        printf("FAIL: %d results, expected %d\n", n, nwant);
        return 1;
    }
    for (int i = 0; i < n; i++) {
        if (r[i] != want[i]) {
            printf("FAIL: result %d is %d, expected %d\n", i, r[i], want[i]);
            return 2 + i;
        }
    }
#ifdef HAVE_PROCS
    int rc = run_probes();
    if (rc) return 100 + rc;
    rc = run_polls();
    if (rc) return 200 + rc;
#endif
    printf("OK\n");
    return 0;
}
