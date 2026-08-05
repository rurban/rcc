#ifndef RCC_STDLIB_H
#define RCC_STDLIB_H

#include <stddef.h>

/* Standard exit status macros (C89 7.20). */
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

/* Maximum value returned by rand(). Must match the glibc implementation
 * this compiler links against (2**31 - 1), so that code scaling rand()
 * by RAND_MAX (e.g. ((double)rand())/RAND_MAX) stays in range. */
#define RAND_MAX 2147483647

void abort(void);
void exit(int status);
void _Exit(int status);
void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void *aligned_alloc(size_t alignment, size_t size);
void free(void *ptr);
int atexit(void (*fn)(void));
int on_exit(void (*fn)(int, void *), void *arg);

/* String -> number conversions.  These MUST be declared: strtod/strtof/
 * strtold and atof return a floating-point value in %xmm0, so an implicit
 * int declaration would make the caller read the result from %rax instead,
 * corrupting every parsed float (e.g. perl's my_atof, breaking version
 * comparisons like $] > 5.009002). */
double strtod(const char *nptr, char **endptr);
float strtof(const char *nptr, char **endptr);
long double strtold(const char *nptr, char **endptr);
double atof(const char *nptr);
int atoi(const char *nptr);
long atol(const char *nptr);
long long atoll(const char *nptr);
long strtol(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char *nptr, char **endptr, int base);
long long strtoll(const char *nptr, char **endptr, int base);
unsigned long long strtoull(const char *nptr, char **endptr, int base);

int rand(void);
void srand(unsigned int seed);
#ifndef _WIN32
long random(void);
void srandom(unsigned int seed);
#endif

int abs(int j);
long labs(long j);
long long llabs(long long j);

void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *));
void *bsearch(const void *key, void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *));

char *getenv(const char *name);
#ifndef _WIN32
int setenv(const char *name, const char *value, int overwrite);
int unsetenv(const char *name);
#endif
int putenv(char *string);
int system(const char *command);
#ifndef _WIN32
int mkstemp(char *template);
char *mkdtemp(char *template);
char *realpath(const char *path, char *resolved_path);
#endif

#endif
