/* GCC Bug #91952 - [rfe] __attribute__((__default_value__()))
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=91952
 */
/* { dg-do compile } */


#include <stdbool.h>
#include <stddef.h>

void cleanup_func (void **ptr);

void *init (void);

void
function (bool condition)
{
  if (condition)
    goto out;

  __attribute__((__cleanup__(cleanup_func)))
    __attribute__((__default_value__((NULL))))
    void *x = init ();

out:
  return;
}
// On the start of the function, when we allocate stack space for 'x', we would ensure that a NULL value is written into that space.  If 'condition' is true, then we exit the function immediately and the cleanup_func is called on the address of 'x' (and x will contain NULL).

// If condition is false then we will proceed to call init() and then write that value into the variable 'x' and when the function completes we'll clean up that value.
// Of course, in many cases optimisation would see that redundant writes are never performed.  Even in the given case we might imagine the compiler moves the write of NULL to after the check of 'condition' and only does it in the case where it won't call init().


