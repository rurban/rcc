/* GCC Bug #117577 - The compiler flag -w does disable pederror errors
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117577
 */
/* { dg-options "-w" } */


typedef void (*PFUNC)(int);

void func(double);

int main()
{
    PFUNC x = func; /* { dg-error "incompatible pointer type" } */
}
// Compiling with "gcc -w main.c", I still get:
// <source>: In function 'main':
// <source>:7:15: error: initialization of 'PFUNC' {aka 'void (*)(int)'} from incompatible pointer type 'void (*)(double)' [-Wincompatible-pointer-types]
//     7 |     PFUNC x = func;
//       |               ^~~~
// Compiler returned: 1
// <a href="https://godbolt.org/z/s4eo9Wz7z">https://godbolt.org/z/s4eo9Wz7z</a>
// I can still suppress the warning explicitly via -Wno-incompatible-pointer-types, but I would expect "-w" to work here, since this is a warning (not an error) and -w "Inhibit all warning messages" as per the docs.

// I have seen this issue with other warnings as well. It seems like warnings that are always active by default (e.g. not opted-in via -Wall, etc) cannot be suppressed via -w.
// Why is that?


