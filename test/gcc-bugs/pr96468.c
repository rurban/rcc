/* GCC Bug #96468 - Warn when an empty while loop could have been a do-while
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=96468
 */


typedef int sig_atomic_t;
volatile sig_atomic_t signaled;
_Bool getX(int *);
void processX(int);
void f(void)
{
    {
        int x;
        if(getX(&x))
//             processX(x);
    }
    while(!signaled);
}

// 1. Do getX (and maybe processX) once (in a block just to minimize the scope of x), then busy-wait until signaled becomes true, and then do some other stuff. This is what it actually does.
// 2. Keep doing getX (and maybe processX) in a loop until signaled becomes true, and then do some other stuff. This isn't what it actually does, because the author forgot the "do" keyword.

// We currently emit no warnings for this code, even when compiled with "-Wall -Wextra". I propose that when we see "while(condition);", we warn that a "do" may be missing, and if it's not, that you should use "while(condition){}" instead (except in cases where it's impossible to just be missing the "do" keyword, like if the "while" is at the beginning of a block).


