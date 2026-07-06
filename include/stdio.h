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
#include <features.h>
#ifdef __GLIBC__
// FILE objects come from the installed glibc at runtime, and code
// guarded by __GLIBC__ (e.g. safeclib) pokes members like _fileno
// directly, so pull the layout from glibc's own header instead of
// hand-copying it, to track whatever glibc version is installed.
#include <bits/types/struct_FILE.h>
typedef struct _IO_FILE FILE;
#else
typedef struct _rcc_FILE FILE;
#endif
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;
#endif
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
