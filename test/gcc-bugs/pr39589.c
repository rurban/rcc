/* GCC Bug #39589 - make -Wmissing-field-initializers=2 work with "designated initializers" ?
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=39589
 */


struct foo { int a; int b; }
   __attribute__((explicit_initialization));
// Or any better name of an attribute.
// The attribute could tell the compiler to please use its
// knowledge of missing field initialisers, and, less paradoxically,
// to warn, actually, when -Wmissing-field-initializers is in effect.
// Just as it does when there are no designator in the initialiser list.
// For software maintenance this will be a boon.
// By analogy, I'd like to illustrate this use case like this:
// I don't want the compiler to be silent by force whenever
// it notices a missing implementation of a member function/Obj-C
// method/Ada prim op just because the programmer had given some other
// of these a name---assuming the respective language were to allow
// this at all.


