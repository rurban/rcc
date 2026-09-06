/* GCC Bug #91348 - Missed optimization: not passing hidden pointer but copying memory
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=91348
 */


typedef struct { char s[64] ; } qerrst_t ;
  extern qerrst_t qerrst0(int err) ;
  #include "qerrst.h"
  #include <stdio.h>
  #include <string.h>

  extern qerrst_t
//   qerrst0(int err)
  {
    qerrst_t st ;

//     snprintf(st.s, sizeof(st.s), "errno=%d", err) ;

    return st ;
  }
  #include <stdio.h>
  #include "qerrst.h"
//   main(int argc, char* argv[])
  {
    int err = argc ;
    qerrst_t z ;

//     printf("qerrst0()='%s'\n", qerrst0(err).s) ;

    z = qerrst0(err) ;
//     printf("qerrst0()='%s'\n", z.s) ;

    return 0 ;
  }
//     lea    0x80(%rsp),%rdi         # "hidden pointer"
//     lea    0x80(%rsp),%rsi         # use "hidden pointer" for printf
//     movdqu (%rsp),%xmm0
//     lea    0x40(%rsp),%rsi         # address of 'qerrst_t z'
//     movdqu 0x10(%rsp),%xmm1
//     movdqu 0x20(%rsp),%xmm2
//     movdqu 0x30(%rsp),%xmm3
//     movaps %xmm0,0x40(%rsp)        # copy "hidden" to 'qerrst_t z'
//     movaps %xmm1,0x50(%rsp)
//     movaps %xmm2,0x60(%rsp)
//     movaps %xmm3,0x70(%rsp)

//     movdqa (%rsp),%xmm0
//     movdqa 0x10(%rsp),%xmm1
//     movdqa 0x20(%rsp),%xmm2
//     movdqa 0x30(%rsp),%xmm3
//     movups %xmm0,(%r12)        # copy st !!!
//     movups %xmm1,0x10(%r12)
//     movups %xmm2,0x20(%r12)
//     movups %xmm3,0x30(%r12)

// So, I looked at the AMD64 ABI (Draft 0.99.7 – November 17, 2014 – 15:08), Section 3.2.3 Parameter Passing, p22:
// So... why is the qerrst0() function doing a copy ? 
// In the case above, gcc fails to spot that 'qerrst_t z' in main() is not visible to anything beyond main().
// FWIW: clang (8.0.0) avoids the spurious copy in qerrst0(), but not the unnecessary copy to 'qerrst_t z'.
// I guess that functions returning large(ish) struct is not deemed worth supporting properly ?


