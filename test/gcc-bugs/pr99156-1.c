/* https://bugs.llvm.org/show_bug.cgi?id=49239#c3 */
/* { dg-do compile } */


int a;
void b() {
  a = ({
    while (1)
      ;
//     0;
  });
}


