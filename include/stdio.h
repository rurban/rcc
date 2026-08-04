#ifndef RCC_STDIO_H
#define RCC_STDIO_H

#include <stddef.h>

#ifdef _WIN32
typedef struct _rcc_FILE FILE;
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
#endif

#endif
