/* `xchg reg, mem` (GNU/AT&T inline-asm order, e.g. "xchgb %0,%1" with
 * %1 constrained "+m") is the textbook x86 spinlock TEST-AND-SET
 * primitive -- real GCC/Clang/kernel/postgres code:
 *
 *   static inline int tas(volatile slock_t *lock) {
 *       slock_t _res = 1;
 *       __asm__ __volatile__("lock; xchgb %0,%1"
 *                             : "+q"(_res), "+m"(*lock) :: "memory", "cc");
 *       return (int) _res;
 *   }
 *
 * rcc's inline-asm `xchg` dispatch (src/asm.c) unconditionally encoded
 * the register-register form (x86_xchg_rr) even when the second operand
 * is memory, silently substituting whatever register happened to occupy
 * that operand slot instead of referencing the actual memory location.
 * The resulting instruction stream desynced from the intended
 * semantics -- postgres's own spinlock TAS crashed with SIGILL inside
 * initdb's very first bootstrap step (`lock xchg dil,r11b` instead of
 * `lock xchg BYTE PTR [rlock],r11b`).
 *
 * Fixed by adding x86_xchg_mr() (src/x86_enc.c) and dispatching to it
 * when the r/m operand is memory, mirroring the existing bts/btr/btc/
 * bt/xadd/cmpxchg dispatches right next to it in asm.c -- xchg was the
 * only one of that family missing the is_mem() check. */
#include <stdio.h>

#if !defined(__aarch64__) && !defined(_M_ARM64)

/* Byte-sized: the exact postgres/kernel spinlock TAS shape. */
typedef unsigned char slock_t;
static __inline__ int tas(volatile slock_t *lock) {
    slock_t _res = 1;
    __asm__ __volatile__(
        "	lock			\n"
        "	xchgb	%0,%1	\n"
        : "+q"(_res), "+m"(*lock)
        :
        : "memory", "cc");
    return (int) _res;
}

/* 16/32/64-bit forms, unlocked, to cover every operand-size prefix path
 * (0x66 prefix, REX.W, plain 32-bit). */
static inline void xchg64(volatile long *p, long *v) {
    __asm__ __volatile__("xchg %0,%1" : "+r"(*v), "+m"(*p));
}
static inline void xchg32(volatile int *p, int *v) {
    __asm__ __volatile__("xchg %0,%1" : "+r"(*v), "+m"(*p));
}
static inline void xchg16(volatile short *p, short *v) {
    __asm__ __volatile__("xchg %0,%1" : "+r"(*v), "+m"(*p));
}

static int run_checks(void) {
    int ok = 1;

    /* Spinlock semantics: first tas() on an unlocked (0) word acquires
     * it (returns the OLD value 0, leaves the lock word set to 1);
     * a second tas() on the now-locked word reports it already held
     * (returns 1, lock word stays 1). */
    slock_t lock = 0;
    int r1 = tas(&lock);
    int r2 = tas(&lock);
    ok = ok && r1 == 0 && r2 == 1 && lock == 1;

    long p64 = 100, v64 = 200;
    xchg64(&p64, &v64);
    ok = ok && p64 == 200 && v64 == 100;

    int p32 = 10, v32 = 20;
    xchg32(&p32, &v32);
    ok = ok && p32 == 20 && v32 == 10;

    short p16 = 1, v16 = 2;
    xchg16(&p16, &v16);
    ok = ok && p16 == 2 && v16 == 1;

    if (!ok)
        printf("FAIL: r1=%d r2=%d lock=%d p64=%ld v64=%ld p32=%d v32=%d p16=%d v16=%d\n",
               r1, r2, lock, p64, v64, p32, v32, p16, v16);

    return ok;
}
#endif /* !__aarch64__ && !_M_ARM64 */

int main(void) {
#if !defined(__aarch64__) && !defined(_M_ARM64)
    return run_checks() ? 0 : 1;
#else
    return 0;
#endif
}
