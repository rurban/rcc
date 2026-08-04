/* GCC Bug #107091 - Misleading error message "incompatible types when returning ..."
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=107091
 */


double bad(void) {
    return (void*)0;
}

double good(void) {
    return 42;
}

// I get this output from gcc 12.2.0 (and from earlier versions):
// c.c: In function ‘bad’:
// c.c:2:12: error: incompatible types when returning type ‘void *’ but ‘double’ was expected
//     2 |     return (void*)0;
//       |            ^
// It's true that it's a constraint violation, and it's true that void* and double are incompatible, but type compatibility is not required here. The problem is that there is no implicit conversion from void* to double.
// In the "good" function, int and double are also incompatible types, but there is an implicit conversion so the statement is valid.
// Note that g++ produces this correct message for the equivalent C++ code:
// c.cpp: In function ‘double bad()’:
// c.cpp:2:19: error: cannot convert ‘void*’ to ‘double’ in return
//     2 |     return (void*)0;
//       |          
// This was brought to my attention by this post on Stack Overflow:
// <a href="https://stackoverflow.com/q/73899947/827263">https://stackoverflow.com/q/73899947/827263</a>
// Reference for compatible types: C11 (or N1570) 6.2.7.


