#ifndef RCC_STDLIB_H
#define RCC_STDLIB_H

#include <stddef.h>

#if defined(_WIN32) || defined(__APPLE__)

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

/* C89 7.10.6.2/6.2/6.3: div()/ldiv()/lldiv() return a small struct by
 * value ({quot, rem}, same type as the arguments, quot first). Layout is
 * standard-mandated, so these are safe to declare portably here even
 * though this bundled header doesn't track the platform's full <stdlib.h> —
 * omitting them made any call an implicit-int "undeclared variable",
 * which (for a struct-returning function) reads the result out of the
 * wrong registers entirely rather than just losing a prototype warning. */
typedef struct {
    int quot;
    int rem;
} div_t;
typedef struct {
    long quot;
    long rem;
} ldiv_t;
typedef struct {
    long long quot;
    long long rem;
} lldiv_t;
div_t div(int numer, int denom);
ldiv_t ldiv(long numer, long denom);
lldiv_t lldiv(long long numer, long long denom);

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

#else

/* Pull in the platform <stdlib.h> for the full declaration set: every
 * allocation/conversion/process function above plus everything this
 * bundled copy doesn't track (ssize_t and friends are typedef'd here
 * too, since glibc's stdlib.h transitively includes <sys/types.h>).
 * Mirrors stdio.h/wchar.h's include_next pattern: a thin bundled header
 * shadowing the SDK one silently drops feature-macro-guarded content,
 * so once include_next is available it's strictly worse than resolving
 * straight to the system header. */
#include_next <stdlib.h>

#endif

#endif
