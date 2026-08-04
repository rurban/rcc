/* GCC Bug #44511 - Misdetects missing return with non-void return type, but only if the function is static
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=44511
 */
/* { dg-do compile } */


void *a(void) { for(;;); }
  static void *b(void) { for(;;) { } }
  void foo(void) { b(); }
// pthread_create(), but never returns because (by design) the thread

// (I assume that the infinite loop detection isn't perfect, but
// function.)
// Copyright (C) 2010 Free Software Foundation, Inc.
// This is free software; see the source for copying conditions.  There is NO
// warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.


