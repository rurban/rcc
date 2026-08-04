/* GCC Bug #90181 - Feature request: provide a way to explicitly select specific named registers in constraints
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=90181
 */
/* { dg-do compile } */


void call_ecall(size_t num)
  {
    register size_t r_a7 __asm("a7") = num;
    __asm volatile("ecall" : : "r" (r_a7) : "memory");
  }

// This gets awkward fast. It adds a lot of extra noise if you have many registers to pass (the ecall instruction provides an example where this may be needed).

// The semantics are also not entirely clear: will r_a7 occupy the a7 register for the entire function (suppose there is more C code around it)? What if call_ecall gets inlined into a larger function? I think the intended (and actual) semantics are that it's effective only at the points where it's passed with register inline asm constraints.
  void call_ecall(size_t num)
  {
    __asm volatile("ecall" : : "a7" (num) : "memory");
  }

// Some architectures do support this (like x86), but not all.


