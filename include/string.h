#ifndef RCC_STRING_H
#define RCC_STRING_H
#include <stddef.h>

#if defined(_WIN32) || defined(__APPLE__)
void *memcpy(void *dst, const void *src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
void *memset(void *dst, int c, size_t n);
int memcmp(const void *lhs, const void *rhs, size_t n);
size_t strlen(const char *s);
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, size_t n);
int strcmp(const char *lhs, const char *rhs);
int strncmp(const char *lhs, const char *rhs, size_t n);
char *strcat(char *dst, const char *src);
char *strncat(char *dst, const char *src, size_t n);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
char *strdup(const char *s);
int strcoll(const char *s1, const char *s2);
size_t strxfrm(char *dest, const char *src, size_t n);
size_t strnlen(const char *s, size_t maxlen);
int strerror_r(int errnum, char *buf, size_t buflen);
void *memrchr(const void *s, int c, size_t n);
char *stpcpy(char *dst, const char *src);
char *stpncpy(char *dst, const char *src, size_t n);
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);
char *strcasestr(const char *haystack, const char *needle);
char *strsignal(int sig);
char *strerror(int errnum);
char *strstr(const char *haystack, const char *needle);
void *memchr(const void *s, int c, size_t n);
size_t strspn(const char *s, const char *accept);
size_t strcspn(const char *s, const char *reject);
char *strpbrk(const char *s, const char *accept);
char *strtok(char *str, const char *delim);
#else
// Pull in the platform <string.h> for every mem*/str* function this
// bundled copy tracks, PLUS the __USE_XOPEN2K8/__USE_MISC-guarded
// extras it doesn't (strverscmp, strfry, basename, ...) and, critically,
// the <sys/cdefs.h> chain it transitively includes (__GNUC_PREREQ,
// __THROW, __attribute_pure__/__attribute_const__/__nonnull__, etc.).
// A thin bundled header shadowing the real one — as this used to do
// unconditionally — silently drops all of that: any *other* system
// header (e.g. glibc's own <obstack.h>, which does a plain
// `#include <string.h>` purely to get memcpy/memset before declaring
// `_obstack_memory_used(...) __attribute_pure__`) that happens to
// `#include <string.h>` before relying on __attribute_pure__ etc.
// having already been defined by <sys/cdefs.h> then hits an undefined
// __attribute_pure__ -> undefined _GL_ATTRIBUTE_PURE fallback and a
// bogus "expected ';', ',', or '{'" parse error. Mirrors stdio.h's/
// stdlib.h's own include_next split (WIN32/APPLE keep the hand-rolled
// list above; every function's ABI here is plain enough that native
// Linux and the ARM64-linux cross target can just take the real thing).
#include_next <string.h>
#endif

#endif
