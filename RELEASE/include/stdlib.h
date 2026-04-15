#ifndef RCC_STDLIB_H
#define RCC_STDLIB_H

#include <stddef.h>

void abort(void);
void exit(int status);
void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);
int atexit(void (*fn)(void));
int on_exit(void (*fn)(int, void *), void *arg);

#endif
