/* GCC Bug #98630 - Seg-fault when using a goto after condition (if)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=98630
 */


unsigned b=0; 
unsigned d=0;
static unsigned g = 1;
unsigned foo (unsigned ui1, unsigned ui2 ) { return (ui2 == 0) ? (ui1) : (ui1 % ui2); }
unsigned j() {
  {
    unsigned *k = &b;
    unsigned *m = &d;
    unsigned **n = &m;
    unsigned ***o = &n;
    if (g)
      ;
    p : {
      if (foo(2, *k)) {
//         ***o = 0;
        return 2;
      }
    }
  }
  goto p;
}
int main() { j(); }
// When compiling with GCC-10 (gcc-10 (Ubuntu 10.2.0-5ubuntu1~20.04) 10.2.0):
// > Segmentation fault (core dumped)</span >


