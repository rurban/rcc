/* GCC Bug #57855 - passing unsafe function as transaction_safe function pointer does not generate error
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=57855
 */
/* { dg-options "-fgnu-tm" } */

#include <stdio.h>

// typedef of a function pointer that is transaction_safe
typedef void (*ADD_STAT)(const char *key, const int klen,
                         const char *val, const int vlen,
                         const void *cookie) __attribute__((transaction_safe));

// function that uses a transaction to call a function that is supposed to be
// safe
void bar(ADD_STAT f)
{
  __transaction_atomic {
    f(NULL, 0, NULL, 0, NULL);
  }
}

// this function is not safe, and is not annotated as being safe
void my_func(const char *key, const int klen,
             const char *val, const int vlen,
             const void *cookie)
{
  printf("Hello World\n");
}

// uh-oh!  I can pass my_func to bar, and it doesn't break!
int main()
{
  bar(my_func); /* { dg-error "incompatible pointer type" } */
}
// Compilation: gcc -std=gnu11 -DHAVE_CONFIG_H -DNDEBUG -g -O2 -pthread -fgnu-tm -MD -MP -o demo demo.c
// The issue here is that my_func is not transaction_safe, but when my_func is passed to bar, which expects a transaction_safe function as its first parameter, the compiler does not reject the code.  It appears as though the transaction_safe attribute in the typedef is being ignored when determining if my_func is the correct type to be passed to bar, but not when determining if my_func can be called from within bar's transaction.


