#ifndef RCC_STDIO_H
#define RCC_STDIO_H

#include <stddef.h>

#ifdef _WIN32
/* The Windows FILE tag must match the platform's own <stdio.h>/<wchar.h>
 * (struct _iobuf, MSVC and mingw-w64 alike), not a private _rcc_FILE:
 * mingw's wchar.h redeclares `FILE *__acrt_iob_func(unsigned)` after
 * this bundled copy ran, and the mismatched struct tag made the
 * conflicting-types check (parser.c) flag two identical declarations --
 * and FILE* would be ABI-incompatible with system-compiled objects. */
typedef struct _iobuf FILE;
FILE *__acrt_iob_func(unsigned idx);
#define stdin (__acrt_iob_func(0))
#define stdout (__acrt_iob_func(1))
#define stderr (__acrt_iob_func(2))

#elif defined(__APPLE__)
typedef struct _rcc_FILE FILE;
extern FILE *__stdinp;
extern FILE *__stdoutp;
extern FILE *__stderrp;
#define stdin __stdinp
#define stdout __stdoutp
#define stderr __stderrp

#else
// Pull in the platform <stdio.h> for the full FILE layout, every stdio
// function, and the __USE_POSIX/__USE_XOPEN-guarded constants (L_ctermid,
// L_cuserid, etc.) that this bundled copy doesn't track. Mirrors wchar.h's
// include_next pattern (and the removed ctype.h stub before it): a thin
// bundled header shadowing the SDK one silently drops whatever feature-
// macro-guarded content the real header has, so once include_next is
// available it's strictly worse than resolving straight to the system
// header.
#include_next <stdio.h>
#endif

#if defined(_WIN32) || defined(__APPLE__)
#define EOF (-1)
#define BUFSIZ 1024
#define FILENAME_MAX 1024
#define FOPEN_MAX 20
#define L_tmpnam 1024
#define TMP_MAX 308915776
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

int printf(const char *fmt, ...);
int fprintf(FILE *stream, const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
int snprintf(char *buf, size_t size, const char *fmt, ...);
FILE *fopen(const char *path, const char *mode);
int fclose(FILE *stream);
size_t fread(void *ptr, size_t size, size_t count, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream);
int fgetc(FILE *stream);
int getc(FILE *stream);
char *fgets(char *buf, int size, FILE *stream);
int fflush(FILE *stream);
#ifdef __APPLE__
/* Without a prototype rcc passes arguments in registers, but Apple's arm64
 * ABI puts variadic ones on the stack, and a pointer result would be cut to
 * int. So declare the variadic and non-int functions callers depend on. */
int scanf(const char *fmt, ...);
int fscanf(FILE *stream, const char *fmt, ...);
int sscanf(const char *str, const char *fmt, ...);
int dprintf(int fd, const char *fmt, ...);
int asprintf(char **ret, const char *fmt, ...);
int vprintf(const char *fmt, __builtin_va_list ap);
int vfprintf(FILE *stream, const char *fmt, __builtin_va_list ap);
int vsprintf(char *buf, const char *fmt, __builtin_va_list ap);
int vsnprintf(char *buf, size_t size, const char *fmt, __builtin_va_list ap);
int vasprintf(char **ret, const char *fmt, __builtin_va_list ap);
int vdprintf(int fd, const char *fmt, __builtin_va_list ap);
int vscanf(const char *fmt, __builtin_va_list ap);
int vfscanf(FILE *stream, const char *fmt, __builtin_va_list ap);
int vsscanf(const char *str, const char *fmt, __builtin_va_list ap);
FILE *freopen(const char *path, const char *mode, FILE *stream);
FILE *fdopen(int fd, const char *mode);
FILE *popen(const char *command, const char *mode);
FILE *tmpfile(void);
long ftell(FILE *stream);
int fseek(FILE *stream, long offset, int whence);
int fileno(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);
int fputc(int c, FILE *stream);
int fputs(const char *s, FILE *stream);
int putc(int c, FILE *stream);
int puts(const char *s);
#endif
#endif

#endif
