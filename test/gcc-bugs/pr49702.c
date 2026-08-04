/* GCC Bug #49702 - Undefined static functions resolve to external definitions
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=49702
 */


static int foo(); int main () { return foo(); }
// t.c:
// ----
int foo() { return 0; }
// <span class="quote">> gcc -o t t1.c t.c</span >
// <span class="quote">> ./t
// > echo $?</span >
// 0
// <span class="quote">> g++ -o t t1.c t.c</span >
// For the C frontend you need -pedantic-errors to make it reject main.c.
// Without -pedantic-errors wrong code is emitted:
// Relocation section '.rela.eh_frame' at offset 0x5a8 contains 1 entries:
//   Offset          Info           Type           Sym. Value    Sym. Name + Addend
// 000000000020  00020000000a R_X86_64_32       0000000000000000 .text + 0
// Symbol table '.symtab' contains 11 entries:
//    Num:    Value          Size Type    Bind   Vis      Ndx Name
// ...
//      9: 0000000000000000    16 FUNC    GLOBAL DEFAULT    1 main
//     10: 0000000000000000     0 NOTYPE  GLOBAL DEFAULT  UND foo
// but that's hardly possible to improve without erroring out by default.
// Is there a way to only make the assembler reject the code (if maybe
// a toplevel asm contains the local symbol?)?


