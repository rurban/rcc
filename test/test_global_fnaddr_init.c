/* A bare function name used as a global scalar initializer, cast to an
 * integer type — e.g. "static unsigned long x = (unsigned long)myfunc;" —
 * relies on C's implicit function-to-pointer decay (no explicit '&'
 * needed). looks_like_address_expr() (which guards the integer-typed
 * global's relocatable-address fallback in global_init_one()/
 * global_initializer(), see test_global_int_addr_init.c) only recognized
 * an explicit ND_ADDR node (a real "&expr") as "looks like an address" —
 * a bare function-name reference (ND_LVAR with no '&') fell through to its
 * default case and was rejected, even though extract_reloc() itself
 * already handles ND_LVAR (any reference to a non-local variable) as an
 * address correctly. The result was a plain "unsupported global
 * initializer" error for a construct GCC/Clang accept without complaint.
 *
 * Found via a real Linux kernel build: arch/x86/events/core.c pulls in
 * NOKPROBE_SYMBOL(), whose __NOKPROBE_SYMBOL() expansion (kernel/kprobes
 * blacklist registration) declares
 * "static unsigned long __used __section("_kprobe_blacklist")
 *  _kbl_addr_fname = (unsigned long)fname;" for a real function fname.
 */
void myfunc(void) {}
static unsigned long addr_of_myfunc = (unsigned long)myfunc;

/* Also exercise it as a struct member initializer (global_init_one's
 * shared fallback), matching how __NOKPROBE_SYMBOL's expansion sits
 * alongside other data in some usages. */
struct holder { unsigned long addr; int tag; };
struct holder h = { .addr = (unsigned long)myfunc, .tag = 7 };

int main(void)
{
    if (addr_of_myfunc != (unsigned long)myfunc) return 1;
    if (h.addr != (unsigned long)myfunc) return 2;
    if (h.tag != 7) return 3;
    return 0;
}
