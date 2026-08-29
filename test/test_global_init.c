/* Global-initializer address-constant regressions, merged into one file
 * since they all exercise the same hand-rolled mini-parsers/folders that
 * resolve "address of a global, plus/minus a constant" shaped initializers
 * to a real relocation: read_global_label_initializer(), extract_reloc(),
 * and looks_like_address_expr(). Each section below documents a distinct
 * bug found via a real Linux kernel build.
 */
#include <stdint.h>

/* ---------------------------------------------------------------------
 * 1. "arr + N" / "arr + N + M + ..." -- array-to-pointer decay, then
 *    pointer arithmetic, as a global initializer.
 *
 * Regression: read_global_label_initializer() already handled a trailing
 * "+ const"/"- const" after a *string literal*, and a leading
 * "[index]"/".member" chain after a plain identifier, but never a
 * trailing "+ const" directly after a plain identifier (with or without
 * that chain) -- so it returned as if the initializer were already
 * complete right after the bare identifier, leaving the caller staring
 * at an unexpected "+" and reporting a confusing "expected ';' or ','"/
 * "expected specific operator" several tokens away from the real cause.
 *
 * Found via a real Linux kernel build: arch/x86/kernel/alternative.c's
 *   static const unsigned char x86nops[] = { ...11 NOP bytes... };
 *   const unsigned char * const x86_nops[ASM_NOP_MAX+1] = {
 *       NULL, x86nops, x86nops + 1, x86nops + 1 + 2, ...
 *   };
 *
 * A second bug in the same addend path: the trailing "+ const" was added
 * as a raw byte count instead of being scaled by the pointee's element
 * size, so it only happened to work for byte-sized elements. Found via
 * PostgreSQL's guc_tables.c:
 *   static const struct config_enum_entry ssl_protocol_versions_info[] = {...};
 *   ... ssl_protocol_versions_info + 1 ...
 * (struct config_enum_entry is 16 bytes) landed the resulting pointer
 * 1 byte past the table instead of 16, so config_enum_lookup_by_value()
 * scanned garbage from the wrong, misaligned base pointer.
 */
static const unsigned char table[] = {10, 20, 30, 40, 50, 60};

static const unsigned char *const offsets[] = {
    table,
    table + 1,
    table + 1 + 2,
    table + 1 + 2 + 3,
};

/* Same idiom for a single (non-array-of-pointers) global. */
static const unsigned char *single = table + 2;

struct entry { const char *name; int val; _Bool hidden; };

static const struct entry struct_table[] = {
    {"zero", 0, 0},
    {"one", 1, 0},
    {"two", 2, 0},
    {0, 0, 0},
};

static const struct entry *const struct_offset = struct_table + 1;

static int test_array_plus_const(void) {
    if (offsets[0] != &table[0]) return 101;
    if (offsets[1] != &table[1]) return 102;
    if (offsets[2] != &table[3]) return 103;
    if (offsets[3] != &table[6]) return 104;
    if (single != &table[2]) return 105;
    if (struct_offset != &struct_table[1]) return 106;
    if (struct_offset->val != 1) return 107;
    if (*offsets[2] != 40) return 108;
    return 0;
}

/* ---------------------------------------------------------------------
 * 2. "&global + CONST1 - CONST2" -- an address, plus a constant, minus
 *    another constant -- must resolve via a real relocation with the net
 *    addend, not fail with "unsupported global initializer".
 *
 * Regression: extract_reloc()'s ND_ADD case already recognized
 * "label + const" / "const + label" on either side, but ND_SUB was
 * grouped with several purely-arithmetic operators (SHL, BITAND, DIV,
 * ...) whose shared fallback just calls eval_const_expr() on the whole
 * subtraction node -- which can't fold an address at all, so it always
 * failed the moment *either* side of a "-" held a label, even when the
 * right-hand side was a plain constant offset (a perfectly valid address
 * constant expression, same as "+"). Subtraction isn't commutative like
 * addition, so only "label - const" (not "const - label") is meaningful.
 *
 * Found via a real Linux kernel build: arch/x86/kernel/cpu/common.c's
 *   #define TOP_OF_INIT_STACK ((unsigned long)&init_stack + \
 *                              sizeof(init_stack) - \
 *                              TOP_OF_KERNEL_STACK_PADDING)
 *   DEFINE_PER_CPU_CACHE_HOT(unsigned long, cpu_current_top_of_stack) =
 *       TOP_OF_INIT_STACK;
 */
struct big {
    char buf[64];
};
static struct big blob;

#define PADDING 8

/* Integer-typed target (matches the real per-cpu case exactly, modulo
 * "unsigned long" vs uintptr_t -- the real kernel idiom is LP64-only
 * (Linux never targets LLP64), where "unsigned long" already is
 * pointer-width; uintptr_t keeps this test meaningful on LLP64 hosts
 * too, since the fix under test -- extract_reloc()'s ND_SUB handling --
 * isn't itself LP64-specific). */
uintptr_t top = (uintptr_t)&blob + sizeof(blob) - PADDING;

/* Pointer-typed target, and a chained "+const-const+const" shape. */
static char *ptop = (char *)&blob + sizeof(blob) - PADDING;
static char *ptop2 = (char *)&blob + 10 - 3 + 1;

static int test_addr_minus_const(void) {
    if (top != (uintptr_t)((char *)&blob + sizeof(blob) - PADDING)) return 201;
    if (ptop != (char *)&blob + sizeof(blob) - PADDING) return 202;
    if (ptop2 != (char *)&blob + 8) return 203;
    return 0;
}

/* ---------------------------------------------------------------------
 * 3. A bare array name in a global initializer, cast to an integer type
 *    (array-to-pointer decay, no explicit '&'), must still resolve to
 *    the array's address via a real relocation -- same as the
 *    already-working "(unsigned long)&some_scalar" case.
 *
 * Regression: looks_like_address_expr() gated the integer-scalar
 * extract_reloc() fallback in global_init_one() on "is this expression
 * unambiguously an address, not a value read" -- correctly rejecting a
 * bare *scalar* global reference (int vi = other_global;    reads a
 * value, not an address) but its ND_LVAR case only ever recognized a
 * bare *function* name as implicitly address-like, never a bare *array*
 * name -- even though arrays decay to their address exactly like
 * functions do, with no other valid meaning in this context. A cast
 * straight from a bare array name (no '&') therefore fell through to
 * "expected constant expression in initializer".
 *
 * Found via a real Linux kernel build: arch/x86/kernel/idt.c's
 *   static struct desc_ptr idt_descr = {
 *       .size = IDT_TABLE_SIZE - 1,
 *       .address = (unsigned long) idt_table,   // idt_table is an array
 *   };
 */
struct gate {
    int x;
};
static struct gate gate_table[16];

struct desc_ptr {
    unsigned short size;
    // Real kernel field is "unsigned long" (LP64-only, Linux never
    // targets LLP64); uintptr_t keeps this test meaningful on LLP64
    // hosts too, since the fix under test -- looks_like_address_expr()'s
    // bare-array-decay case -- isn't itself LP64-specific.
    uintptr_t address;
};
static struct desc_ptr descr = {
    .size = 10,
    .address = (uintptr_t)gate_table,
};

/* Same shape for a plain (non-struct-member) global, and via an
 * intermediate typedef'd array type. */
typedef int arr16_t[16];
static arr16_t other_table;
static uintptr_t other_addr = (uintptr_t)other_table;

static int test_array_addr_cast(void) {
    if (descr.address != (uintptr_t)&gate_table[0]) return 301;
    if (descr.size != 10) return 302;
    if (other_addr != (uintptr_t)&other_table[0]) return 303;
    return 0;
}

/* ---------------------------------------------------------------------
 * 4. An integer-typed (not pointer-typed) global — or struct member —
 *    whose initializer is a cast address of another symbol, e.g.
 *    "unsigned long x = (unsigned long)&some_array;", needs a real
 *    relocation just like a pointer-typed global does: the address
 *    isn't known until link time, so it can't be constant-folded, but
 *    it's not an error either.
 *
 * Regression: the global-initializer code already handled this shape
 * for pointer-typed variables/members (extract_reloc() + append_reloc()),
 * both at file scope and nested inside a struct's designated initializer
 * -- but the parallel scalar/integer code paths (both the top-level
 * global_initializer() and the nested-member global_init_one()) only
 * ever tried eval_const_expr()/eval_double_const_expr() and then gave
 * up, reporting "unsupported global initializer" or "expected constant
 * expression in initializer".
 *
 * Found via a real Linux kernel build: arch/x86/include/asm/processor.h's
 * INIT_THREAD macro (x86-64) is
 * "{ .sp = (unsigned long)&__top_init_kernel_stack }" -- struct
 * thread_struct's sp field is a plain `unsigned long`, not a pointer --
 * used as init/init_task.c's init_task.thread field's initializer.
 *
 * "unsigned long" is deliberately part of the real-world shape: it's
 * what the kernel source uses, and it's pointer-width on every target
 * the Linux kernel actually builds for. It is NOT pointer-width on
 * LLP64 (Windows, where "unsigned long" is 4 bytes) -- real GCC itself
 * rejects this exact cast there as "initializer element is not
 * constant" (confirmed against x86_64-w64-mingw32-gcc), since a 4-byte
 * field can't be guaranteed to hold a real 64-bit address. addr_uint_t
 * below is "unsigned long" everywhere except Windows, where uint64_t
 * -- unlike "unsigned long" -- actually is 8 bytes, so the
 * address-cast-to-integer-scalar codegen path under test still gets
 * covered there too.
 */
#ifdef _WIN32
typedef uint64_t addr_uint_t;
#else
typedef unsigned long addr_uint_t;
#endif

addr_uint_t backing_array[4];

/* File-scope integer global, not nested in a struct. */
addr_uint_t flat_addr = (addr_uint_t)&backing_array;

struct thread_struct {
    addr_uint_t sp;
};

struct task_struct {
    int x;
    struct thread_struct thread;
    int y;
};

/* The exact real-world shape: an integer field nested inside a struct's
 * designated initializer, holding a cast address of another global. */
struct task_struct init_task = {
    .x = 1,
    .thread = {
        .sp = (addr_uint_t)&backing_array,
    },
    .y = 2,
};

static int test_int_addr_init(void) {
    if (flat_addr != (addr_uint_t)&backing_array) return 401;
    if (init_task.thread.sp != (addr_uint_t)&backing_array) return 402;
    if (init_task.x != 1 || init_task.y != 2) return 403;
    return 0;
}

int main(void) {
    int r;
    if ((r = test_array_plus_const()) != 0) return r;
    if ((r = test_addr_minus_const()) != 0) return r;
    if ((r = test_array_addr_cast()) != 0) return r;
    if ((r = test_int_addr_init()) != 0) return r;
    return 0;
}
