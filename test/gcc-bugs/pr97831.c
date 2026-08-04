/* GCC Bug #97831 - Lack of disable_tail_calls attribute
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=97831
 */
/* { dg-do compile } */


static void disabletailcallfunc(void*) __attribute__((noipa));
static void disabletailcallfunc(void *x){}
#define disabletailcall() do {int a; disabletailcallfunc(&a);}while(0);

int functionwhichIwantToDisableTailCallFrom(...)
{
// disabletailcall();

}


