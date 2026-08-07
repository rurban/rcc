/* x86 spin-wait / memory-fence compiler builtins with no header
 * dependency. Real GCC/clang implement `__builtin_ia32_pause`,
 * `__builtin_ia32_{m,l,s}fence` as genuine compiler intrinsics (no
 * declaration needed, no linkable symbol) -- code can call them
 * directly without including <emmintrin.h>/<xmmintrin.h> first. curl's
 * bundled curlx headers do exactly that, and without these predefined,
 * rcc left `__builtin_ia32_pause` as an ordinary (implicitly declared)
 * function call, which then failed to link: "undefined reference to
 * `__builtin_ia32_pause'". Must compile and link with no declaration
 * in scope, and actually execute the instructions (checked via
 * disassembly of the emitted call sites). */
int main(void) {
#if !defined(__aarch64__) && !defined(_M_ARM64)
    __builtin_ia32_pause();
    __builtin_ia32_mfence();
    __builtin_ia32_lfence();
    __builtin_ia32_sfence();
#endif
    return 0;
}
