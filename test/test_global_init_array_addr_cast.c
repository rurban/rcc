/* A bare array name in a global initializer, cast to an integer type
 * (array-to-pointer decay, no explicit '&'), must still resolve to the
 * array's address via a real relocation -- same as the already-working
 * "(unsigned long)&some_scalar" case.
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
#include <stdint.h>

struct gate {
    int x;
};
static struct gate table[16];

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
    .address = (uintptr_t)table,
};

/* Same shape for a plain (non-struct-member) global, and via an
 * intermediate typedef'd array type. */
typedef int arr16_t[16];
static arr16_t other_table;
static uintptr_t other_addr = (uintptr_t)other_table;

int main(void) {
    if (descr.address != (uintptr_t)&table[0]) return 1;
    if (descr.size != 10) return 2;
    if (other_addr != (uintptr_t)&other_table[0]) return 3;
    return 0;
}
