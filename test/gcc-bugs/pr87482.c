/* GCC Bug #87482 - Clarify behaviour of resolvers with parameters in  for __attribute__((ifunc))
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=87482
 */


#include <stdlib.h>
void *my_memcpy (void *dst, const void *src, size_t len)
{
  return dst;
}

void *my_memcpy2 (void *dst, const void *src, size_t len)
{
  return dst;
}

static void * (*resolve_memcpy (int hwcap))(void *, const void *, size_t)
{
    if (hwcap == 1)
        return my_memcpy;
//     else
        return my_memcpy2;
}

void *memcpy(void *, const void*, size_t) __attribute__((ifunc ("resolve_memcpy")));
// Compiles without a problem.
// From what I can see glibc doesn't use __attribute__((ifunc("resolver"))) instead it uses __asm__ (".type " resolver, %gnu_indirect_function). 
// For context clang will give an error message if the ifunc resolver has a parameter.
// ifunc2.c:20:58: error: ifunc resolver function must have no parameters
void *memcpy(void *, const void*, size_t) __attribute__((ifunc ("resolve...
// There is a thread on cfe-dev asking if the GCC documentation is correct and to see if clang should be following the documentation or the implementation: <a href="http://lists.llvm.org/pipermail/cfe-dev/2018-September/059548.html">http://lists.llvm.org/pipermail/cfe-dev/2018-September/059548.html</a>  
// There was a recent change for 8.0 that made "The resolver should be declared to be a function taking no arguments" more explicit.
// <a href="https://gcc.gnu.org/bugzilla/show_bug.cgi?id=81882">https://gcc.gnu.org/bugzilla/show_bug.cgi?id=81882</a>


