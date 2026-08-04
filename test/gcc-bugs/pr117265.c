/* GCC Bug #117265 - RFE: support for assembly macros/assembly headers
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117265
 */


// Certainly. This is *not* only used by copy_*_user (or {get,put}_user for that matter), here is an example from msr.h:
static inline unsigned long long native_read_msr_safe(unsigned int msr,
                                                      int *err)
{
//         DECLARE_ARGS(val, low, high);

        asm volatile("1: rdmsr ; xor %[err],%[err]\n"
//                      "2:\n\t"
//                      _ASM_EXTABLE_TYPE_REG(1b, 2b, EX_TYPE_RDMSR_SAFE, %[err])
//                      : [err] "=r" (*err), EAX_EDX_RET(val, low, high)
//                      : "c" (msr));
        if (tracepoint_enabled(read_msr))
//                 do_trace_read_msr(msr, EAX_EDX_VAL(val, low, high), *err);
        return EAX_EDX_VAL(val, low, high);
}


