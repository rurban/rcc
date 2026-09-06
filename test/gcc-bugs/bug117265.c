/* GCC Bug #117265 - RFE: support for assembly macros/assembly headers
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117265
 */
/* { dg-do compile } */

/* Feature request (enhancement): gcc cannot consume kernel-style asm
 * macro headers (e.g. the _ASM_EXTABLE macros in arch/x86/include/asm/
 * asm.h) that use .macro/.endm inside inline asm.  The reporter's
 * example (native_read_msr_safe from msr.h) is only expressible with
 * those kernel headers; shown here in self-contained form. */
static inline unsigned long long native_read_msr_safe(unsigned int msr,
                                                      int *err)
{
  unsigned long long val;
  asm volatile("1: rdmsr ; xor %[err],%[err]\n"
               : [err] "=r" (*err), "=A" (val)
               : "c" (msr));
  return val;
}