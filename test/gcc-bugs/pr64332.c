/* GCC Bug #64332 - wrong location for Wattributes warning
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=64332
 */


#define __constructor __attribute__((constructor))
// Here is a simple example (also archived in attach):
#define __constructor __attribute__((constructor))

#pragma GCC system_header

typedef void (*__cb_type)(void *);
int foo(__cb_type __constructor);
#include "c.h"
#include "c-impl.h"
 #define __constructor __attribute__((constructor))

// g++ (Debian 4.9.1-16) 4.9.1
// Copyright (C) 2014 Free Software Foundation, Inc.
// This is free software; see the source for copying conditions.  There is NO
// warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.


