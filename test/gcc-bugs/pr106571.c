/* GCC Bug #106571 - Implement -Wsection diag
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=106571
 */
/* { dg-do compile } */

/* Feature request (enhancement): clang's -Wsection catches a section
 * attribute specified on a redeclared variable.  GCC does not implement
 * this (and per comments 1-4 the general case is intentionally not
 * diagnosed).  DECLARE_PER_CPU expands to the two declarations below. */
typedef unsigned long u64;

extern u64 x86_spec_ctrl_current;
__attribute__((section(".data..percpu" ""))) __typeof__(u64) x86_spec_ctrl_current;