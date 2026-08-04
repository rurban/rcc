/* GCC Bug #39843 - -funsigned-bitfields discards "aligned" attribute
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=39843
 */
/* { dg-do compile } */


// % gcc-snapshot --version
// gcc-snapshot (Ubuntu 20081013-0ubuntu2) 4.3.3 20081014 (prerelease)
// Copyright (C) 2008 Free Software Foundation, Inc.
// This is free software; see the source for copying conditions.  There is NO
// warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// % gcc-snapshot -funsigned-bitfields testsuite/gcc.dg/bitfld-3.c
// % ./a.out
// Abort
// A bitfield defined with type "la", where "la" is a typedef for "long __attribute__((aligned (8)))", is correctly aligned... until you pass -funsigned-bitfields. If you do that, the bitfield changes from "long" to "unsigned long" (correctly), and all its attributes are discarded (incorrectly).


