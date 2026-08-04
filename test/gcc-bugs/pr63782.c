/* GCC Bug #63782 - avoid implicit declaration warning for incompatible builtin implicit declaration
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=63782
 */


void foo(char *str) {
//   strcpy(str, "foo");
}

// [t@ws-520 examples]$ clang-3.5 -c -Wall test.c
// test.c:2:3: warning: implicitly declaring library function 'strcpy' with
// type 'char *(char *, const char *)'
// strcpy(str, "foo");
// ^
// test.c:2:3: note: include the header <string.h> or explicitly provide a
// declaration for 'strcpy'
// 1 warning generated.
// test.c: In function ‘foo’:
// test.c:2:3: warning: implicit declaration of function ‘strcpy’ [-Wimplicit-function-declaration]
//    strcpy(str, "foo");
//    ^
// test.c:2:3: warning: incompatible implicit declaration of built-in function ‘strcpy’
// test.c:2:3: note: include ‘<string.h>’ or provide a declaration of ‘strcpy’
// The first warning is superfluous if the second is given.
// (from a talk by Samsung at LinuxCon Europe: <a href="http://events.linuxfoundation.org/sites/events/files/slides/linuxcon-europe-2014-llvm.pdf">http://events.linuxfoundation.org/sites/events/files/slides/linuxcon-europe-2014-llvm.pdf</a>)


