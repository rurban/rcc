/* GCC Bug #82179 - Optionally compile free calls in such a way that the passed pointer is clobbered after the call
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=82179
 */
/* { dg-do compile } */


#define SAFE_FREE(x) do { if((x) != 0x0) { free(x); (x) = (void *)0x1; } } while(0)

// After free(x), we set x to an address that will crash when dereferenced
// (use-after-free), and will also crash when it's an argument to free().
// Note that NULL isn't used, because free(NULL) does nothing, which might
// hide potential double-free bugs.
// This will detect use-after-free and double-free bugs by having the program crash instead of allowing various heap grooms and further exploitation.
// After discussion with Martin Sebor and Florian Weimer in the libc-alpha list, it was pointed out to me to post the request here for further interest.
// https://sourceware.org/ml/libc-alpha/2017-09/msg00238.html
// https://sourceware.org/ml/libc-alpha/2017-09/msg00423.html
// Below are some examples of how it works,
// Crashes when use-after-free:
#include <unistd.h>
#include <stdlib.h>

struct unicorn_counter { int num; };

int main() {
    struct unicorn_counter* p_unicorn_counter;
    int* run_calc = malloc(sizeof(int));
    *run_calc = 0;
    SAFE_FREE(run_calc);
    //SAFE_FREE(run_calc); // uncommenting this crashes with a double-free instead
    p_unicorn_counter = malloc(sizeof(struct unicorn_counter));
    p_unicorn_counter->num = 42;
    if (*run_calc) // use-after-free
        execlp("/usr/bin/id", "id", 0);
}
// Segmentation fault
// gdb$ r
// ...
// 0x4005ed <main+87>:	mov    eax,DWORD PTR [rax]
// Stopped reason: SIGSEGV
// 0x00000000004005ed in main ()
// gdb$ print /x $rax
// $1 = 0x1
// Crashes when double-free, by uncommenting the 2nd SAFE_FREE(run_calc) above:
// gdb$ r
// ...
// 0x7ffff7aad614 <__GI___libc_free+20>:	mov    rax,QWORD PTR [rdi-0x8]
// Stopped reason: SIGSEGV
// __GI___libc_free (mem=0x1) at malloc.c:2929
// $1 = 0x1
