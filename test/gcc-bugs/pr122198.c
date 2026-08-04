/* GCC Bug #122198 - gcc ICEs on large number of variables each initializing to the previous one
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=122198
 */
/* { dg-do compile } */


// Tried to synthesize an example with many section references for ld.bfd to explore quadratic behaviour, but crashed `gcc` instead:
// ```
$ printf "int var_0 __attribute__ ((section (\".data.0\"))) = { $i };\n" > main.c; for (( i=1; i<1000000; i++ )); do printf "void * var_$i __attribute__ ((section (\".data.$i\"))) = { &var_$((i-1)) };\n"; done >> main.c; printf "int main() {}" >> main.c; gcc -c main.c -o main.o
// gcc: internal compiler error: Segmentation fault signal terminated program cc1
// Please submit a full bug report, with preprocessed source (by using -freport-bug).
// See <<a href="https://gcc.gnu.org/bugs/">https://gcc.gnu.org/bugs/</a>> for instructions.
// ```
// Backtrace looks like it might be a stack overflow in gc traversal:
// ```
// Program received signal SIGSEGV, Segmentation fault.
// 0x000000000075f80e in gt_ggc_mx_lang_tree_node(void*) ()
// (gdb) bt
#0  0x000000000075f80e in gt_ggc_mx_lang_tree_node(void*) ()
#1  0x00000000007605bf in gt_ggc_mx_lang_tree_node(void*) ()
#2  0x00000000007605da in gt_ggc_mx_lang_tree_node(void*) ()
#3  0x000000000075fc14 in gt_ggc_mx_lang_tree_node(void*) ()
#4  0x00000000007605da in gt_ggc_mx_lang_tree_node(void*) ()
#5  0x00000000007605da in gt_ggc_mx_lang_tree_node(void*) ()
#6  0x000000000075fc14 in gt_ggc_mx_lang_tree_node(void*) ()
#7  0x00000000007605da in gt_ggc_mx_lang_tree_node(void*) ()
#8  0x00000000007605da in gt_ggc_mx_lang_tree_node(void*) ()
#9  0x000000000075fc14 in gt_ggc_mx_lang_tree_node(void*) ()
#10 0x00000000007605da in gt_ggc_mx_lang_tree_node(void*) ()
// ...
// ```
// I wonder if `gcc` could have handled with more grace. Adding more stack does help:
// $ ulimit -s unlimited
$ printf "int var_0 __attribute__ ((section (\".data.0\"))) = { $i };\n" > main.c; for (( i=1; i<1000000; i++ )); do printf "void * var_$i __attribute__ ((section (\".data.$i\"))) = { &var_$((i-1)) };\n"; done >> main.c; printf "int main() {}" >> main.c; gcc -c main.c -o main.o
# all ok


