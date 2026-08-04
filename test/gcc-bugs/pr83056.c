/* GCC Bug #83056 - GCC suggests the use of previously reported undeclared identifiers when reporting new undeclared identifiers
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=83056
 */


enum {
//     TYPE_A,
}

int fn(void)
{
    int b = TYPE_B;
    int c = TYPE_C;
    int d = TYPE_D;
    return 0;
}
// suggest.c: In function 'fn':
// suggest.c:7:13: error: 'TYPE_B' undeclared (first use in this function); did you mean 'TYPE_A'?
     int b = TYPE_B;
//              ^~~~~~
//              TYPE_A
// suggest.c:7:13: note: each undeclared identifier is reported only once for each function it appears in
// suggest.c:8:13: error: 'TYPE_C' undeclared (first use in this function); did you mean 'TYPE_B'?
     int c = TYPE_C;
//              ^~~~~~
//              TYPE_B
// suggest.c:9:13: error: 'TYPE_D' undeclared (first use in this function); did you mean 'TYPE_C'?
     int d = TYPE_D;
//              ^~~~~~
//              TYPE_C
// For some reason, for every new undeclared identifier gcc suggest to use a previously reported (but apparently similar) undeclared identifier.
// Shouldn't it always suggest TYPE_A, the only defined identifier?


