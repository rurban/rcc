/* GCC Bug #57627 - -Wsizeof-pointer-memaccess should make an exception for character types
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=57627
 */


void f1(int *dst, int *src)
{ __builtin_memcpy(dst, src, sizeof(int *)); }

void f2(char *dst, char *src)
{ __builtin_memcpy(dst, src, sizeof(char *)); }

void f3(unsigned char *dst, unsigned char *src)
{ __builtin_memcpy(dst, src, sizeof(unsigned char *)); }
// test.c: In function ‘f1’:
// test.c:2:36: warning: argument to ‘sizeof’ in ‘__builtin_memcpy’ call is the same pointer type ‘int *’ as the destination; expected ‘int’ or an explicit length [-Wsizeof-pointer-memaccess]
 { __builtin_memcpy(dst, src, sizeof(int *)); }
//                                     ^
// test.c: In function ‘f2’:
// test.c:5:36: warning: argument to ‘sizeof’ in ‘__builtin_memcpy’ call is the same pointer type ‘char *’ as the destination; expected ‘char’ or an explicit length [-Wsizeof-pointer-memaccess]
 { __builtin_memcpy(dst, src, sizeof(char *)); }
//                                     ^
// test.c: In function ‘f3’:
// test.c:8:36: warning: argument to ‘sizeof’ in ‘__builtin_memcpy’ call is the same pointer type ‘unsigned char *’ as the destination; expected ‘unsigned char’ or an explicit length [-Wsizeof-pointer-memaccess]
 { __builtin_memcpy(dst, src, sizeof(unsigned char *)); }
//                                     ^
// Copyright (C) 2013 Free Software Foundation, Inc.
// This is free software; see the source for copying conditions.  There is NO
// warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

// The warning makes sense for most types (f1), but for character types, I don't think GCC should give any warning (f2 and f3). I'm seeing this warning for correct code that stores pointers in raw byte buffers, and I would be surprised if that is less common than code that really means to memcpy a single byte.


