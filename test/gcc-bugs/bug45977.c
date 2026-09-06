/* GCC Bug #45977 - "warning: 'i' initialized and declared 'extern'" could use a separate warning flag controlling it
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=45977
 */


#include <crt0.h>
extern "C" int _crt0_startup_flags = 0 | _CRT0_FLAG_LOCK_MEMORY;
int main(int argc, char * argv[]) {
    _crt0_startup_flags &= ~_CRT0_FLAG_LOCK_MEMORY;
//     static_cast<void>(argc);
//     static_cast<void>(argv);
    return 0;
}
// manx@appendix:~/tmp$ i386-pc-msdosdjgpp-g++ -c -std=gnu++17 -O2 -Wall -Wextra -Wpedantic djgpp-main.cpp
// djgpp-main.cpp:2:16: warning: '_crt0_startup_flags' initialized and declared 'extern'
//     2 | extern "C" int _crt0_startup_flags = 0 | _CRT0_FLAG_LOCK_MEMORY;
//       |                ^~~~~~~~~~~~~~~~~~~
// manx@appendix:~/tmp$
// ```
// minimal test case:
// ```
// manx@appendix:~/tmp$ cat warn.cpp
extern "C" {
extern int foo;
}
extern "C" int foo = 23;
// manx@appendix:~/tmp$ g++ -c -std=c++17 -O2 -Wall -Wextra -Wpedantic warn.cpp
// warn.cpp:4:16: warning: ‘foo’ initialized and declared ‘extern’
//     4 | extern "C" int foo = 23;
//       |                ^~~
// manx@appendix:~/tmp$
// ```
// also happens with no -W flags:
// ```
// manx@appendix:~/tmp$ g++ -c -std=c++17 -O2 warn.cpp
// warn.cpp:4:16: warning: ‘foo’ initialized and declared ‘extern’
//     4 | extern "C" int foo = 23;
//       |                ^~~
// ```
// However, I am not seeing the warning in C code:
// ```
// manx@appendix:~/tmp$ cat warn.c
extern int foo;
int foo = 23;
// manx@appendix:~/tmp$ gcc -c -std=c17 -O2 -Wall -Wextra -Wpedantic warn.c
// manx@appendix:~/tmp$
// ```
// I would really appreciate an option to disable this warning in C++.
// I am not 100% sure if my issue is really identical, but it certainly looks related.
// Also, as the original issue was about C, do you want me to report a separate issue for C++?
// GCC versions:
// manx@appendix:~/tmp$ gcc --version
// manx@appendix:~/tmp$ i386-pc-msdosdjgpp-gcc --version
// i386-pc-msdosdjgpp-gcc (GCC) 10.3.0


