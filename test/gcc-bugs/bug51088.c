/* GCC Bug #51088 - undefined label with statement expression and cond expression
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=51088
 */
/* { dg-do compile } */


int test()
{
 return ({ for ( void* L[] = {
				(1?&&A:({goto *(*L?&&A:&&end); (void*)0;})),
				((1||({B: goto *1[L]; 1;})),&&C),
				({if (0) C: goto *&&end; &&B;}),
   }; ({A: goto *L[2]; 1;}); ({end: return 42; 0;}))
  goto *(0[L]); -1; });
}

// * gcc (Debian 4.4.5-8) 4.4.5 (x86_64)
// /tmp/ccWabCoL.o: In function `test':
// blah.c:(.text+0x5): undefined reference to `.L2'
// collect2: ld returned 1 exit status
// * gcc (Debian 4.6.1-4) 4.6.1 (x86_64)
// /tmp/ccOX241S.o:blah.c:function test: error: undefined reference to '.L4'
// collect2: ld returned 1 exit status


