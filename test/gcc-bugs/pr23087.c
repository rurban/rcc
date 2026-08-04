/* GCC Bug #23087 - Misleading warning, "... differ in signedness" with the character types
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=23087
 */


void func(char *s);
// the call
//     func("hello");
// would be a constraint violation if string literals were const.
// (As it is, it's merely dangerous, causing undiagnosed undefined
// behavior if func attempts to modify the string.)
// But all this is beside the point.  I get the same warning
// messages with this:
void foo(void)
{
    const signed char   *ps = "signed?";
    const unsigned char *pu = "unsigned?";
}
// Plain char, unsigned char, and signed char are all compatible,
// in the sense that you can use a value of any of the three
// types to initialize an object of any of the three types
// (possibly with an implicit conversion).
// But because they are all distinct types, pointers to those types
// are distinct and incompatible types.
// The correct warning message should be
//     "initialization from incompatible pointer type",
// the same message produced for the initialization of lptr in
// the following:
void bar(void)
{
    int i = 42;
    long l = i;         /* ok, implicit conversion */
    int *iptr = &i;
    long *lptr = iptr;  /* incompatible types */
}
// even though int and long happen to have the same representation
// on the system I'm using, just as char and signed char happen
// to have the same representation.
// The initializations of ps and pu in foo() are invalid,
// but this has nothing to do with signedness or constness;
// it's just because the types are incompatible.


