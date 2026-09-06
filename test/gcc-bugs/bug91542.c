/* GCC Bug #91542 - internal representation of pointer reference shown in error message
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=91542
 */
/* { dg-do compile } */


struct Node {
    int n_successors;
};

int foo(int n, struct Node *nodes[])
{
//     nodes[n].n_successors; // `n_successors` should be accessed through a struct dereference expression.
}
#include "..." search starts here:
#include <...> search starts here:
// GNU C99 (GCC) version 10.0.0 20190824 (experimental) (x86_64-pc-linux-gnu)
// GNU C99 (GCC) version 10.0.0 20190824 (experimental) (x86_64-pc-linux-gnu)
// /home/luke/code/cc/src/gcc-bug.c:7:13: error: ‘*(nodes + (sizetype)((long unsigned int)n * 8))’ is a pointer; did you mean to use ‘->’?
//     7 |     nodes[n].n_successors;


