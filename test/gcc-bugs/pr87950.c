/* GCC Bug #87950 - GCC warns about reaching end of non-void function when all switch cases are completely handled
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=87950
 */

enum Enum {
  A,
  B,
};

int CoverMyBases(enum Enum x) {
 switch (x) {
  case A:
   return 1;
  case B:
   return 0;
 }
}

int main(int argc, const char **argv) {
	CoverMyBases(A);
	CoverMyBases(B);
 return 0;
}
